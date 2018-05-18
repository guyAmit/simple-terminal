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
#include<signal.h>

#define EXECFIALD -1
#define NULLCOMMAND 3
#define QUITCOMMAND 2
#define REGUALRCOMMAND 1
#define CDCOMMAD 4

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
  if(p_cmd_line == 0){
    return NULLCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"quit")==0){
    return QUITCOMMAND;
  }
  else if(strcmp(p_cmd_line->arguments[0],"cd")==0){
    return CDCOMMAD;
  }
  else{
    return REGUALRCOMMAND;
  }
 }

void execute_helper(cmdLine * p_cmd_line,int * left_pipe, int * right_pipe){
  int child_pid = fork();
  if(child_pid==0){
    if(right_pipe!=0){
        close(1);
        dup2(right_pipe[1],1);
        close(right_pipe[1]);
    }
    if(left_pipe!=0){
        close(0);
        dup2(left_pipe[0],0);
        close(left_pipe[0]);
    }
    redirect_output(p_cmd_line);
    redirect_input(p_cmd_line);
    if(execvp(p_cmd_line->arguments[0],p_cmd_line->arguments)==-1){
      perror("error in first command");
    }
  }
  else{
    if(right_pipe!=0){
      close(right_pipe[1]);
    }
    if(left_pipe!=0){
      close(left_pipe[0]);
    }
    if(p_cmd_line->next){
      if(p_cmd_line->next->next){
        int new_pipe[2];
        pipe(new_pipe);
        execute_helper(p_cmd_line->next,right_pipe,new_pipe);
      }
      else{
        execute_helper(p_cmd_line->next,right_pipe,0);
      }
    }
    if(p_cmd_line->blocking){
      waitpid(child_pid,0,WCONTINUED);
    }
  }
}



void execute (cmdLine * p_cmd_line){
  if(p_cmd_line->next){
    int right_pipe[2];
    pipe(right_pipe);
    execute_helper(p_cmd_line,0,right_pipe);
  }
  else{
    execute_helper(p_cmd_line,0,0);
  }
  wait(0);
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
void sig_handler(int sign_num){
  printf("\n%s was ignored\n",strsignal(sign_num));
}

int signal_handler(){
  if((signal(SIGQUIT,sig_handler)!=SIG_ERR) | (signal(SIGTSTP,sig_handler)!=SIG_ERR) |
   (signal(SIGCHLD,sig_handler)!=SIG_ERR )){
      return 1;
   }
   else {
     return -1;
   }
}

int main(int argc, char const *argv[]) {
  char input_string[PATH_MAX];
  char current_working_dir[PATH_MAX];
  int is_special=1;
  do{
    getcwd(current_working_dir,PATH_MAX);
    printf("\033[1;32m");
    printf("%s$ ",current_working_dir);
    printf("\033[0m");
    //if(signal_handler()==1){ //uncomment for signal ditecting
      fgets(input_string,PATH_MAX,stdin);
      cmdLine * line = parseCmdLines(input_string);
      is_special = is_special_command(line);
      if(is_special==REGUALRCOMMAND){
        execute(line);
      }
      else if(is_special==CDCOMMAD){
        cd_commad(line);
      }
      freeCmdLines(line);
  //  } //uncomment for signal ditecting
  }while (is_special!=QUITCOMMAND);
  printf("$\n");
  return 0;
}
