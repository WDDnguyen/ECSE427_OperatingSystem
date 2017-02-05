// William Nguyen 260638465 Assignement 1: Shell
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MaxArg 20
pid_t child_pid;

/*Seperate input and set each value into args array
Determine if special characters such as >,& or | are included and raise flags
input : inpuxt in the command prompt
args:string array to place tokens inside
background : flag to run command in background
redirection:flag to redirect output to another location
pipeStatus : flag to pipe output to another command
 */

void getcmd (char* input, char * args[], int *background, int *redirection,int *pipeStatus){
        int i;
        for (i = 0; i < MaxArg; i++){
                args[i] = strsep(&input, " ");
                if(args[i] == NULL){
                break;
                }
                if (strcmp(args[i],">") ==0) {
                        *redirection = i;
                }
                else if (strcmp(args[i], "|") == 0) {
                        *pipeStatus = i;
                }
        }
        if (strcmp(args[i-1],"&") == 0){
                *background = 1;
                args[i-1] = NULL;
        }
}

/*
Whenever a signal occurs, this function handles the signal input.
When CTRL + C is pressed, kill the program.
*/

static void sigHandler(int sig){

        if (sig == SIGINT){
                kill(child_pid,SIGKILL);
        }
}

/*
Place Pid of background job into a list
bgProcessList : array of background Pids
pid : pid to add to background list
bgIndex: current index of list
*/

void addProcessToBg(pid_t *bgProcessList, int pid, int* bgIndex){
        //MODIFY TO DYNAMICALLY GIVE INDEX/REMOVE INDEX
        printf("Adding PID = %d to Background at location %d : \n", pid, *bgIndex);
        bgProcessList[*bgIndex] = pid;
        *bgIndex = *bgIndex + 1;
}

/*
When command is jobs, print the list of background jobs
bgProcessList : list of background jobs
bgIndex : current index of storage
*/

void printBgJobs(pid_t *bgProcessList, int* bgIndex){
        int i;
        printf("current jobs in background :\n\n");
        for (i = *bgIndex; i >0  ; i--){
                printf("Job ID %d at : %d\n", bgProcessList[i],i);
                }

}

/*
Remove arguments in the array that are not needed to execute external programs when piping or redirection.
args :array of arguments
index : index to remove argument value
*/

void removeArgs(char *args[], int index){
        int i = index;
        for (i ; i < MaxArg ; i++){
                if (args[i] == NULL) {
                break;
                }else {
                args[i] = NULL;
                }
        }
}

//Shell Console

int main(void){
char *args[MaxArg];
int bg;
int redirection;
int pipeStatus;
int statusOfProcess;
const int maxPath = 4096;
char input[maxPath];

//Background variables
const int maxBgProcesses = 20;
int bgIndex = 0;
int *ptrbgIndex = &bgIndex;
pid_t bgProcessList[maxBgProcesses];

while(1)
{
bg = 0;
redirection = -1;
pipeStatus = -1;
fflush(stdout);
printf(">> ");
fgets(input, sizeof(input),stdin);

        if(input[strlen(input)-1] == '\n'){
                input[strlen(input)-1] = '\0';
        }

getcmd(input,args, &bg, &redirection,&pipeStatus);

        if(strcmp(args[0],"cd") == 0) {
                int cd = chdir(args[1]);
                if (cd == -1){
                        perror("cannot access that directory\n");
                }
        }
        else if (strcmp(args[0],"\0") == 0) {
                printf("Command invalid pleae type a new one\n");
        }

        else if (strcmp(args[0],"exit") == 0) {
                printf("exiting shell\n");
                exit(1);
        }
        else {

        int pid = fork();
                if (pid < 0){
                        perror("child process error");
                }
                else if (pid == 0){

                         if (strcmp(args[0],"pwd") == 0){
                         char buffer[maxPath];
                         printf("current working directory : %s\n", getcwd(buffer,maxPath));
                         }

                         else if (strcmp(args[0],"fg") == 0){
                         //ERROR handling
                         if (args[1] == NULL || args[1] ==""){
                                 perror("there is no argument for fg");
                        }
                        printf("switching to the process at index %s\n : ", args[1]);
                        pid_t newPid = bgProcessList[atoi(args[1])];
                        printf("NEW PID : %d\n",newPid);
                        if (newPid > 0){
                                printf("new Foreground Process\n");
                                kill(newPid,SIGCONT);
                                waitpid(newPid, &statusOfProcess, 0);
                        }

                        }

                        else if (strcmp(args[0],"jobs") == 0){
                                printBgJobs(bgProcessList,ptrbgIndex);
                        }

                        else if (redirection >= 0){

                                close(1);
                                int  fd  = open(args[redirection + 1],O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                                removeArgs(args,redirection);
                                execvp(args[0],args);
                        }
                        else if(pipeStatus >= 0){

                                int pfd[2];
                                if(pipe(pfd) == -1) {
                                        perror("Pipe error");
                                        exit(1);
                                }
                                int pipedPid = fork();
                                if (pipedPid == 0){

                                        close(0);
                                        dup(pfd[0]);

                                        close(pfd[1]);
                                        close(pfd[0]);

                                        //Arguments for second command when piping
                                        int length = sizeof(args)/sizeof(args[0]);
                                        int secondLength = length - pipeStatus;
                                        char* pipeArgs[secondLength];
                                        for(int i =0 ; i< secondLength - 1 ; i++){
                                                pipeArgs[i] = args[pipeStatus + 1 + i];
                                        }
                                        pipeArgs[secondLength] = 0;
                                        execvp(pipeArgs[0],pipeArgs);
                                }else{
                                close(1);
                                dup(pfd[1]);

                                close(pfd[0]);
                                close(pfd[1]);

                                removeArgs(args,pipeStatus);
                                execvp(args[0],args);
                                }

                        }
                        else {
                         execvp(args[0],args);
                        }

                        _exit(EXIT_FAILURE);

                }
        else{

                if (bg == 0){

                        child_pid = pid;
                        if (signal(SIGINT, sigHandler) == SIG_ERR){
                                        printf("ERROR! could not bind the signal handler\n");
                                        exit(1);
                        }

                        if (signal(SIGTSTP,SIG_IGN) == SIG_ERR){
                                printf("ERROR! could not ignore signal\n");
                        }
                        waitpid(pid,&statusOfProcess,0);

        }
        else {
                addProcessToBg(bgProcessList,pid,ptrbgIndex);

        }
        }
  }
 }
}
