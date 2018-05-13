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

void redirect_input(cmdLine * p_cmd_line, int * in){
  if(p_cmd_line->inputRedirect!=0){
    *(in)=open(p_cmd_line->inputRedirect,O_RDWR);
    close(0); //close stdin
    dup2(*(in),0);//make "in" defualt input
  }
}

void redirect_output(cmdLine * p_cmd_line, int * out){
  if(p_cmd_line->outputRedirect!=0){
    *(out)=open(p_cmd_line->outputRedirect,O_RDWR|O_CREAT,S_IRUSR|S_IXUSR|S_IWUSR);
    close(1); //close stdout
    dup2(*(out),1); //make "out" defualt output
  }
}

void execute_single (cmdLine * p_cmd_line){
  if(strcmp(p_cmd_line->arguments[0],"cd")==0){
    char current_working_dir[PATH_MAX];
    getcwd(current_working_dir,PATH_MAX);
    int i=0;
    while (current_working_dir[i]!='\0') {
      i++;
    }
    current_working_dir[i]='/';
    i++;
    char * fs = p_cmd_line->arguments[1];
    while(*(fs)){
      current_working_dir[i]=*(fs++);
      i++;

    }
    current_working_dir[i]='\0';
    if(chdir(current_working_dir)==-1){
      perror("problem in cd");
    }
  }
  else{
    int pid = fork();
    if(pid==0){
      //procsses wotk
      int out;
      int in;
      redirect_output(p_cmd_line,&out);
      redirect_input(p_cmd_line,&in);
      if(execvp(p_cmd_line->arguments[0],p_cmd_line->arguments)==-1){
        perror("error in command");
        exit(-1);
      }
      else{
        close(in);
        close(out);
      }
    }
    else{
      if(p_cmd_line->blocking){
        waitpid(pid,0,WCONTINUED);
      }
    }
  }
  freeCmdLines(p_cmd_line);
}

//should be called after checking that there is more than one command
void execute_pipe(cmdLine * p_cmd_line){
  int pipe_fd[2];
  pipe(pipe_fd);
  int pid_first_child = fork();

  if(pid_first_child==0){
    //child process work
    int in=0;
    close(1);
    dup(pipe_fd[1]);
    close(pipe_fd[1]);
    redirect_input(p_cmd_line,&in);
    if(execvp(p_cmd_line->arguments[0],p_cmd_line->arguments)==-1){
      perror("error in first command");
    }
  }
  else{
    //parent work
    close(pipe_fd[1]);
    int pid_second_child=fork();
    if(pid_second_child==0){
      //child work process
      int out=1;
      close(0);
      dup(pipe_fd[0]);
      close(pipe_fd[0]);
      redirect_output(p_cmd_line->next,&out);
      if(execvp(p_cmd_line->next->arguments[0],p_cmd_line->next->arguments)==-1){
        perror("error in second command");
      }
    }
    else{
      //parent work process
      close(pipe_fd[0]);
      waitpid(pid_first_child,0,WCONTINUED);
      waitpid(pid_second_child,0,WCONTINUED);

    }
  }
}

void execute(cmdLine * p_cmd_line){
  if(p_cmd_line->next){
    execute_pipe(p_cmd_line);
  }
  else{
    execute_single(p_cmd_line);
  }
}

int main(int argc, char const *argv[]) {
  char input_string[PATH_MAX];
  char current_working_dir[PATH_MAX];
  int is_quit=1;
  do{
    getcwd(current_working_dir,PATH_MAX);
    printf("\033[1;32m");
    printf("~%s$: ",current_working_dir);
    printf("\033[0m");
    fgets(input_string,PATH_MAX,stdin);
    if(strcmp(input_string,"quit\n")==0){
      is_quit=0;
    }
    else if(strcmp(input_string,"\n")==0){
      continue;
    }
    else{
      cmdLine * line = parseCmdLines(input_string);
      execute(line);
    }
  }while (is_quit!=0);
  printf("$\n");
  return 0;
}
