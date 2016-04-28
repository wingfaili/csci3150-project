/*
  Name: Li Wing Fai
  SID: 1155053870
  Last modify date: 2016-03-16
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/times.h>
#include <signal.h>

pid_t pid;
void alrmHandler(int signal){
    kill(pid, SIGTERM);
}

pid_t pids[10][2];
void alrmHandler2(int signal){
    pid_t myPid = getpid();
    // Searching for monitor pid
    int i;
    for(i=0; i<10; i++){
        if(pids[i][0] == myPid){
            kill(pids[i][1], SIGTERM);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    char* input1 = argv[1];
    char* input2 = argv[2];
    if(strcmp(input1, "FIFO") == 0){
        signal(SIGALRM, alrmHandler);
        clock_t startTime, endTime;
        struct tms cpuTime;
        double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
        int i=0, j=0;

        FILE *fp;
        char buff[255];
        fp = fopen(input2, "r");

        while(fgets(buff, 255, (FILE*)fp)!=NULL){
            buff[strlen(buff)-1]='\0';
            char *line = strtok(buff, "\t");
            char *command = line;
            printf("%s\n", command);
            line = strtok(NULL, "\t");
            int duration=0;
            if(strcmp(line, "-1") == 0){
                duration = -1;
            }
            for(i=0; i<strlen(line); i++){
                // translate the time from string to intger
                duration = duration * 10 + (line[i] - '0');
            }

            char *token = strtok(command, " ");

            setenv("PATH","/bin:/usr/bin:.", 1);

            startTime = times(&cpuTime);
            if(!(pid = fork())) {
                char **argList = (char**) malloc(sizeof(char*) * 255);
                argList[0] = (char*)malloc(sizeof(char) * 255);
                strcpy(argList[0], token);

                for(i=1; i<255; i++){
                    argList[i] = (char*)malloc(sizeof(char) * 255);
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        strcpy(argList[i], token);
                    }
                    else {
                        argList[i] = NULL;
                    }
                }

                if(argList[1] != NULL){
                    glob_t globbuf;
                    globbuf.gl_offs = 1;
                    glob(argList[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);
                    for(i=2; i<255; i++){
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
                    printf("%s:  command not found\n", command);
                else
                    printf("[%s]:  unknown error\n", command);
                exit(0);
            }

            else{
                alarm(duration);
                wait(NULL);
            }
            endTime = times(&cpuTime);

            printf("<<Process %d>>\n", pid);
            printf("Time Elapsed: %.4f\n", (endTime-startTime)/ticks_per_sec);
            printf("user time: %.4f\n", cpuTime.tms_cutime/ticks_per_sec);
            printf("system time: %.4f\n", cpuTime.tms_cstime/ticks_per_sec);
            printf("\n\n");
        }
        fclose(fp);
    }


    if(strcmp(input1, "PARA") == 0){
        //signal(SIGALRM, alrmHandler2);
        clock_t startTime, endTime;
        struct tms cpuTime;
        double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
        int i=0, j=0;

        FILE *fp;
        char buff[255];
        fp = fopen(input2, "r");
        char **line=(char**) malloc(sizeof(char*) * 255);
        char **command=(char**) malloc(sizeof(char*) * 255);
        char **token=(char**) malloc(sizeof(char*) * 255);
        int duration[10]={0,0,0,0,0,0,0,0,0,0};
        int cnt = 0;

        while(fgets(buff, 255, (FILE*)fp)!=NULL){
            buff[strlen(buff)-1]='\0';
            line[i] = (char*)malloc(sizeof(char) * 255);
            strcpy(line[i], strtok(buff, "\t"));
            command[i] = (char*)malloc(sizeof(char) * 255);
            strcpy(command[i], line[i]);
            line[i] = strtok(NULL, "\t");

            if(strcmp(line[i], "-1") == 0){
                duration[i] = -1;
            }
            else{
                int j;
                for(j=0; j<strlen(line[i]); j++){
                    // translate the time from string to intger
                    duration[i] = duration[i] * 10 + (line[i][j] - '0');
                }
            }
            i++, cnt++;
        }

        /*
        printf("%d\n", cnt);
        printf("%s\n", input2);
        for(i=0;i<cnt;i++){
            printf("command[%d]: %s\n", i, command[i]);
            //printf("token[%d]: %s\n", i, token[i]);
            printf("duration[%d]: %d\n", i, duration[i]);
        }
        */

        for(j=0; j<cnt; j++){
            // Fork 3 monitors
            if(!(pids[j][0] = fork())){
                /* Monitor Process*/
                signal(SIGALRM, alrmHandler2);
                pids[j][0] = getpid();
                //printf("in fork(),pids[%d][0] is %d\n", j, pids[j][0]);
                //printf("%s\n", command[j]);
                token[j] = (char*)malloc(sizeof(char) * 255);
                strcpy(token[j], strtok(command[j], " "));
                setenv("PATH","/bin:/usr/bin:.", 1);
                startTime = times(&cpuTime);
                if(! (pids[j][1] = fork())){
                    /* Job Process */
                    pids[j][1] = getpid();
                    // Doing Jobs
                    char **argList = (char**) malloc(sizeof(char*) * 255);
                    argList[0] = (char*)malloc(sizeof(char) * 255);
                    strcpy(argList[0], token[j]);

                    for(i=1; i<255; i++){
                        argList[i] = (char*)malloc(sizeof(char) * 255);
                        token[j] = strtok(NULL, " ");
                        if(token[j] != NULL){
                            strcpy(argList[i], token[j]);
                        }
                        else {
                            argList[i] = NULL;
                        }
                    }

                    if(argList[1] != NULL){
                        glob_t globbuf;
                        globbuf.gl_offs = 1;
                        glob(argList[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);
                        for(i=2; i<255; i++){
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
                        printf("%s:  command not found\n", command[j]);
                    else
                        printf("[%s]:  unknown error\n", command[j]);
                    exit(0);
                }
                else{
                    alarm(duration[j]);
                    wait(NULL);
                    // Print Time
                    endTime = times(&cpuTime);
                    printf("<<Process %d>>\n", pids[j][1]);
                    printf("Time Elapsed: %.4f\n", (endTime-startTime)/ticks_per_sec);
                    printf("user time: %.4f\n", cpuTime.tms_cutime/ticks_per_sec);
                    printf("system time: %.4f\n", cpuTime.tms_cstime/ticks_per_sec);
                    printf("\n\n");
                }
                exit(0);

            }

            //else{
              //for (j = 0; j<cnt; j++){
                //waitpid(pids[j][0], NULL, 0);
              //}
            //}

        }

        for (j = 0; j<cnt; j++){
          waitpid(pids[j][0], NULL, 0);
        }
        fclose(fp);
    }
    return 0;
}
