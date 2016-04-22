#include <ctype.h>
#include <errno.h> // Needed by errno
#include <glob.h> // Needed by glob()
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t pid;
void alrmHandler(int signal) {
    kill(pid,SIGTERM);
}

pid_t pids[10][2];
void alrmHandler_p(int signal){
    pid_t myPid = getpid();
    // printf("myPid: %d\n", myPid);

    // Searching for monitor pid
    int i;
    for(i = 0; i < 10; i++) {
        // printf("pids[%d][1]: %d\n", i, pids[i][0]);
        if(pids[i][0] == myPid) {
            // printf("kill myPid: %d\n", pids[i][0]);
            kill(pids[i][1],SIGTERM);
            break;
        }
    }
}

int main(int argc, char *argv[]) {

    // printf("input: %s %s\n", argv[1], argv[2]);

    glob_t globbuf;
    globbuf.gl_offs = 1;
    clock_t startTime, endTime;
    struct tms cpuTime;
    double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
    char line[256];
    char const* const fileName = argv[2];
    FILE* file = fopen(fileName, "r");
    char deli[] = "\t";
    char deli2[] = " ";

    if (strcmp(argv[1], "FIFO") == 0 ) {
        // printf("In FIFO mode\n\n");
        signal(SIGALRM,alrmHandler);

        int process_number = 0;
        while (fgets(line, sizeof(line), file)) {
            process_number++;
        }
        fclose(file);
        // printf("Process Number: %d\n\n", process_number);
        file = fopen(fileName, "r");

        while ((fgets(line, sizeof(line), file)) != NULL) {

            line[strlen(line)-1] = '\0';
            char *token2 = strtok(line, deli);
            // printf("%s\n", token2);
            char *dur = strtok(NULL, deli);
            // printf("dur: %s\n", dur);
            char **argList = (char**) malloc(sizeof(char*) * 255);
            argList[0] = (char*)malloc(sizeof(char));
            argList[0] = strtok(token2, deli2);



            unsigned int duration;
            if ( isdigit(dur[0]) ) {
                duration = atoi(dur);
                // printf("+ve Duration: %u\n", duration);
            } else if (strcmp(dur, "-1") == 0) {
                // printf("Negative token: %s\n", dur);
                duration = -1;
                // printf("-ve Duration: %u\n", duration);
            }

            int i = 1;
            int only_one = 0;
            for(i = 1; i < 255; i++) {
                argList[i] = (char*) malloc(sizeof(char*) * 255);
                char *tmp = strtok(NULL,deli2);
                // printf("tmp: %s\n", tmp);
                if (tmp == NULL) {
                    // printf("break\n");
                    argList[i] = NULL;
                    break;
                }
                token2 = tmp;
                if (token2 != NULL) {
                    strcpy(argList[i], token2);
                } else {
                    argList[i] = NULL;
                }
            }

            int globCount = 0;
            int loopCount = 1;

            setenv("PATH","/bin:/usr/bin:.", 1);

            startTime = times(&cpuTime);

            if( !(pid = fork()) ) {
                if(argList[1] != NULL) {
                    glob(argList[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);
                    for (i = 2; i < 255; i++) {
                            glob(argList[i], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
                        }
                    // printf("%s\n", argList[0]);
                    globbuf.gl_pathv[0] = argList[0];
                    execvp(globbuf.gl_pathv[0],globbuf.gl_pathv);
                    perror("error:");
                } else {
                    execvp(*argList, argList);
                    // printf("[%s]\n",*argList);
                    perror("error:");
                    exit(0);
                }
            } else {
                // printf("Duration:%u\n", duration);
                if (duration != -1) {
                    alarm(duration);
                }
                wait(NULL);
            }

            endTime = times(&cpuTime);
            printf("<<Process %d>>\n", pid);
            printf("time elapsed: %.4f\n",(endTime- startTime)/ticks_per_sec);
            printf("user time: %.4f\n",cpuTime.tms_cutime/ticks_per_sec);
            printf("system time: %.4f\n\n",cpuTime.tms_cstime/ticks_per_sec);
        }
    } else if(strcmp(argv[1], "PARA") == 0) {

        // printf("In Parallel mode\n");
        int process_number = 0;

        while (fgets(line, sizeof(line), file)) {
            process_number++;
        }
        fclose(file);
        // printf("Process Number: %d\n\n", process_number);
        file = fopen(fileName, "r");

        int duration[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        char **argList = (char**) malloc(sizeof(char*) * 255);

        int i = 0;
        int j = 0;
        while ((fgets(line, sizeof(line), file)) != NULL) {
            line[strlen(line)-1] = '\0';

            // printf("Line: %s\n", line);
            char *token2 = strtok(line, deli);
            // printf("token2: %s\n", token2);
            char *dur = strtok(NULL, deli);
            // printf("dur: %s\n", dur);

            argList[i] = (char*)malloc(sizeof(char));
            strcpy(argList[i], token2);
            // printf("argList[%d]: %s\n", i, argList[i]);
            // printf("argList[%d]: %s\n", i, argList[i]);

            // for(j = 0; j < strlen(argList[i]); j++) {
            //     printf("argList[%d][%d]: %c\n", i, j, argList[i][j]);
            // }
            // printf("\n");

            if ( isdigit(dur[0]) ) {
                duration[i] = atoi(dur);
                // printf("+ve Duration: %u\n", duration);
            } else if (strcmp(dur, "-1") == 0) {
                // printf("Negative token: %s\n", dur);
                duration[i] = -1;
                // printf("-ve Duration: %u\n", duration);
            }
            i++;
        }

        for(j = 0; j < process_number; j++) {
        // Fork 3 monitors

            if(!(pids[j][0] = fork())) {
                /* Monitor Process*/
                // printf("In Monitor process\n");
                signal(SIGALRM,alrmHandler_p);
                pids[j][0] = getpid();

                signal(SIGALRM,alrmHandler_p);
                setenv("PATH","/bin:/usr/bin:.", 1);

                startTime = times(&cpuTime);
                if(! (pids[j][1] = fork())) {
                /* Job Process */
                    // printf("In job process\n");
                    pids[j][1] = getpid();
                    // Doing Jobs
                    int k;
                    int h = 0;
                    // printf("leng: %lu\n", strlen(argList[j]));
                    for(k = 0; k < strlen(argList[j]); k++) {
                        // printf("1-argList[%d]: %s\n",j, argList[j]);
                        char **argList2 = (char**) malloc(sizeof(char*) * 255);
                        argList2[0] = (char*)malloc(sizeof(char));
                        argList2[0] = strtok(argList[j], deli2);

                        for(i = 1; i < 255; i++) {
                            argList2[i] = (char*) malloc(sizeof(char*) * 255);
                            char *tmp = strtok(NULL,deli2);
                            // printf("tmp: %s\n", tmp);
                            if (tmp == NULL) {
                                // printf("break\n");
                                argList2[i] = NULL;
                                break;
                            }
                            argList[j] = tmp;
                            // printf("i=%d - j=%d\n", i, j);

                            if (argList[j] != NULL) {
                                strcpy(argList2[i], argList[j]);
                                // printf("2-argList[%d]: %s\n", j, argList[j]);
                                // printf("2-argList2[%d]: %s\n", i, argList2[i]);
                                // argList2[i] = argList[j];
                            } else {
                                argList2[i] = NULL;
                            }
                            // printf("2-argList2[%d]: %s\n", i, argList2[i]);
                        }
                        // for(i = 0; i < 255; i++) {
                        //     printf("3--------argList2[%d]: %s\n", i, argList2[i]);
                        // }
                        if(argList2[1] != NULL) {
                            glob(argList2[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);

                            for (i = 2; i < 255; i++) {
                                glob(argList2[i], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
                            }
                        // printf("%s\n", argList[0]);
                            globbuf.gl_pathv[0] = argList2[0];

                            execvp(globbuf.gl_pathv[0],globbuf.gl_pathv);

                        } else {
                            execvp(*argList2, argList2);
                            // printf("[%s]\n",*argList);
                        }

                        perror("error:");
                        exit(0);
                    }
                } else {
                    alarm(duration[j]);
                    wait(NULL);
                    // Print Time
                    endTime = times(&cpuTime);
                    printf("<<Process %d>>\n", pids[j][1]);
                    printf("time elapsed: %.4f\n",(endTime- startTime)/ticks_per_sec);
                    printf("user time: %.4f\n",cpuTime.tms_cutime/ticks_per_sec);
                    printf("system time: %.4f\n\n",cpuTime.tms_cstime/ticks_per_sec);
                }
                exit(0);
            }
        }
        for (j = 0; j < process_number; j++) {
            // printf("~~~~pids[%d][0]: %d\n", j, pids[j][0]);
            waitpid(pids[j][0], NULL, 0);
        }
    } else {
        printf("Invalid argument\n");
    }

    fclose(file);
    return 0;
}
