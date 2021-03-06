#include "LineParser.h"
#include <unistd.h>
#include "linux/limits.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "job_control.h"
#include <signal.h>


#define EXECFIALD -1
#define NULLCOMMAND 3
#define QUITCOMMAND 2
#define REGUALRCOMMAND 1
#define CDCOMMAD 4
#define JOBSCOMMAND 5
#define FGCOMMAND 6
#define BGCOMMAND 7


int redirect_input(cmdLine * p_cmd_line){
  if(p_cmd_line->inputRedirect){
    int in = open(p_cmd_line->inputRedirect,O_RDWR);
    close(0);
    dup2(in,0);
    return in;
  }
  return 0;
}

int redirect_output(cmdLine * p_cmd_line){
  if(p_cmd_line->outputRedirect){
    int out = open(p_cmd_line->outputRedirect,O_RDWR|O_CREAT,S_IRUSR|S_IXUSR|S_IWUSR);
    close(1);
    dup2(out,1);
    return out;
  }
  return 1;
}

int is_special_command(cmdLine * p_cmd_line){
  if((p_cmd_line == 0) || (strcmp(p_cmd_line->arguments[0],"\n")==0)){
    return NULLCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"quit")==0){
    return QUITCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"cd")==0){
    return CDCOMMAD;
  }
  else if(strcmp(p_cmd_line->arguments[0],"jobs")==0){
    return JOBSCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"fg")==0){
    return FGCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"bg")==0){
    return BGCOMMAND;
  }
  else{
    return REGUALRCOMMAND;
  }
 }


void execute_helper(cmdLine * p_cmd_line,int * left_pipe, int * right_pipe, job * current_job){
  int child_pid = fork();
  if(child_pid==0){
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT,SIG_DFL);
    signal(SIGCHLD,SIG_DFL);

    if(0 == current_job->pgid){
        current_job->pgid = getpid();
    }
    setpgid(0,current_job->pgid);

    if(left_pipe!=0){
        close(0);
        dup2(left_pipe[0],0);
        close(left_pipe[0]);
    }

    if(right_pipe!=0){
        close(1);
        dup2(right_pipe[1],1);
        close(right_pipe[1]);
    }

    redirect_input(p_cmd_line);
    redirect_output(p_cmd_line);

    if(execvp(p_cmd_line->arguments[0],p_cmd_line->arguments)==-1){
      perror("error in execute");
      exit(-1);
    }
  }
  else{
    if(current_job->pgid == 0){
      current_job->pgid = child_pid;
    }
    setpgid(child_pid, current_job->pgid);

    if(left_pipe!=0){
      close(left_pipe[0]);
    }

    if(right_pipe!=0){
      close(right_pipe[1]);
    }

    if(p_cmd_line->next){
      if(p_cmd_line->next->next){
        int new_right[2];
        pipe(new_right);
        execute_helper(p_cmd_line->next,right_pipe,new_right,current_job);
      }
      else{
        execute_helper(p_cmd_line->next,right_pipe,0,current_job);
      }
    }
  }
}


void execute (cmdLine * p_cmd_line,job ** job_list,int job_number, struct termios * terminal_settings, pid_t shell_pgid ){
  job * current_job = find_job_by_index(*(job_list),job_number);
  int * start_pipe= 0;
  if(p_cmd_line->next){
    start_pipe= (int*)malloc(sizeof(int)*2);
    pipe(start_pipe);
  }
  execute_helper(p_cmd_line,0,start_pipe,current_job);
  free(start_pipe);

  int blocking=0;
  cmdLine * line = p_cmd_line;
  while((line!=0) && (blocking==0)){
    if(line->blocking){
      blocking=1;
     }
    line=line->next;
  }

  if(blocking){
    run_job_in_foreground(job_list,current_job,1,terminal_settings,shell_pgid);
  }
  else{
    run_job_in_background(current_job,1);
  }

}

void cd_commad(cmdLine* p_cmd_line){
  char current_working_dir[PATH_MAX];
  getcwd(current_working_dir,PATH_MAX);
  int i = 0;
  while(current_working_dir[i]!='\0'){
    i++;
  }
  current_working_dir[i]='/';
  i++;
  char *fs = p_cmd_line->arguments[1];
  while(*(fs)){
    current_working_dir[i]= *(fs++);
    i++;
  }
  current_working_dir[i]='\0';
  if(chdir(current_working_dir)==-1){
    perror("error in cd");
  }
}


void sig_ignore(int signo){
  printf("sig ctr_z was not ignored\n");
}

int main(int argc, char const *argv[]) {

  //shell initialization
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
  signal(SIGCHLD,SIG_IGN);

  int shell_pgid = getpgid(0);
  setpgid(0,shell_pgid);
  struct termios * terminal_settings = (struct termios*) malloc(sizeof(struct termios));
  tcgetattr(STDIN_FILENO,terminal_settings);  //save and terminal attributes

  //jobs initialization
  job * init =0;
  job ** job_list=&init;
  int job_number = 0;

  //input system initialize
  char input[PATH_MAX];
  char current_working_dir[PATH_MAX];
  int command_type=1;

  //
  do{
    getcwd(current_working_dir,PATH_MAX);
    printf("~%s$: ",current_working_dir);
    fgets(input,PATH_MAX,stdin);
    cmdLine * p_cmd_line = parseCmdLines(input);
    command_type = is_special_command(p_cmd_line);
    if(command_type==REGUALRCOMMAND){
      add_job(job_list,input);
       job_number++;
       execute(p_cmd_line,job_list,job_number,terminal_settings,shell_pgid);
     }
     else if( command_type==CDCOMMAD){
       cd_commad(p_cmd_line);
     }
     else if(command_type==JOBSCOMMAND){
       print_jobs(job_list);
     }
     else if((command_type==FGCOMMAND) && (p_cmd_line->arguments[1]!=0)) {
       int index_of_job = atoi(p_cmd_line->arguments[1]);
       job * j = find_job_by_index(*(job_list),index_of_job);
       run_job_in_foreground(job_list,j,1,terminal_settings,shell_pgid);
     }
     else if((command_type==BGCOMMAND) && (p_cmd_line->arguments[1]!=0)){
       int index_of_job = atoi(p_cmd_line->arguments[1]);
       job * j = find_job_by_index(*(job_list),index_of_job);
       run_job_in_background(j,1);
     }
     if(*(job_list)==0){
       job_number=0;
     }

  }while(command_type!=QUITCOMMAND);
  free_job_list(job_list);
  return 0;
}
