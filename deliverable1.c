#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>

void prompt(){
  char cwd[PATH_MAX+1];
  if(getcwd(cwd,PATH_MAX+1) != NULL){
    printf("[Felix Shell:%s]$ ", cwd);
  }
}

int main(int argc, char *argv[]) {
  int run = 1;

  while(run){
    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    
    prompt();
    char buf[255];
    fgets(buf,255,stdin);
    buf[strlen(buf)-1]='\0';
    char *token = strtok(buf, " ");

    // Non-build-in command
    if(token != NULL){
      if(strcmp(token, "cd") && strcmp(token, "exit")){
        pid_t child_pid;
        setenv("PATH","/bin:/usr/bin:.",1);

        if(!(child_pid = fork())) {
          signal(SIGINT,SIG_DFL);
          signal(SIGQUIT,SIG_DFL);
          signal(SIGTERM,SIG_DFL);
          signal(SIGTSTP,SIG_DFL);

          char **argList = (char**) malloc(sizeof(char*) * 255);
          argList[0] = (char*)malloc(sizeof(char) * 255);
          strcpy(argList[0], token);

          for(int i=1; i<255; i++){
            argList[i] = (char*)malloc(sizeof(char) * 255);
            token = strtok(NULL, " ");
            if(token != NULL){
              strcpy(argList[i], token);
            }
            else {
              argList[i] = NULL;
            }
          }

          int wildcard = 0;
          if(argList[1] != NULL){
            for(int j=0; j<strlen(argList[1]); j++){
              if(argList[1][j]=='*'){
                wildcard = 1;
                break;
              }
            }
          }

          if(wildcard){
            glob_t globbuf;
            globbuf.gl_offs = 1;
            glob(argList[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);
            for(int i=2; i<255; i++){
              if(argList[i] != NULL)
                glob(argList[i], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
            }

            globbuf.gl_pathv[0] = argList[0];
            execvp(globbuf.gl_pathv[0], globbuf.gl_pathv);
          }
          else{
            execvp(*argList, argList);
          }

          if(errno == ENOENT)
            printf("%s:  command not found\n", buf);
          else
            printf("[%s]:  unknown error\n", buf);
        }
        else{
          wait(NULL);
        }
      }

      // buitd-in command
      else{
        // change directory and exit
        if(strcmp(token, "cd") == 0){
          token = strtok(NULL, " ");
          if(strtok(NULL, " ") != NULL){
            printf("cd: wrong number of arguments\n");
          }
          else{
            if(chdir(token) != -1);
            else printf("[%s]: cannot change directory\n",token);
          }
        }

        if(strcmp(token, "exit") == 0){
          token = strtok(NULL, " ");
          if(token != NULL){
            printf("cd: wrong number of arguments\n");
          }
          else run = 0;
        }

      }
    }
  } //end of while

  return 0;
}
