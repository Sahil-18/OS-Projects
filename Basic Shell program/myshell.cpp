#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>

#define Max_cmd 20
#define Max_cmd_len 200
#define Size 80


//Function to parse the input from shell into multiple commands according to symbol type like '&', '#' or '>'
char parseInput(char* input,char command[Max_cmd][Max_cmd_len],int* no_cmds){   
    int cmd = 0;
    char *token,*empty,*cpyInput;
    char type;
    empty = strdup("");
    
    cpyInput=(char*)malloc(strlen(input)*sizeof(char));
    strcpy(cpyInput,input);
    
    //Parsing with respect to '&' 
    while((token=strsep(&cpyInput,"&&"))!=NULL){
        if(strcmp(token,empty)!=0){
            strcpy(command[cmd],token);
            cmd++;
        }
    }
    type='&';
    
    //If the previous parsing gives output as single command then we can consider that the input was not having '&&' in input
    //Therefore we can now parse with respect to '#'
    if(cmd==1){
        cmd=0;
        cpyInput=(char*)malloc(strlen(input)*sizeof(char));
        strcpy(cpyInput,input);
        while((token=strsep(&cpyInput,"##"))!=NULL){
            if(strcmp(token,empty)!=0){
                strcpy(command[cmd],token);
                cmd++;
            }
        }
        type='#';
    }
    
    //If the previous parsing gives output as single command then we can consider that the input was not having '&&' and '##'in input
    //Therefore we can now parse with respect to '>'
    if(cmd==1){
        cmd=0;
        cpyInput=(char*)malloc(strlen(input)*sizeof(char));
        strcpy(cpyInput,input);
        while((token=strsep(&cpyInput,">"))!=NULL){
            if(strcmp(token,empty)!=0){
                strcpy(command[cmd],token);
                cmd++;
            }
        }
        type='>';
    }
    
    //If the previous parsing gives output as single command then we can consider that the input was not having '&&', '##' and '>' in input
    //Therefore we can conclude that input is a single command
    if(cmd==1){
        type=' ';
    }
    *(no_cmds)=cmd;
    return type;
}

//Handler for CTRL+Z signal
void handlerTSTP(int num){
    int pid = getpid();
    kill(pid,SIGINT);
}

//Exexuting a single command
void executeCommand(char* command){
    char** cmd;
    int i=0;
    char *token,*str,*empty;
    str=(char*)malloc(strlen(command)*sizeof(char));
    strcpy(str,command);
    empty = strdup("");
    
    //Parsing the single command with respect to " " so that we can get command and its arguments as different string saved using double pointer
    while((token=strsep(&str," "))!=NULL){
        if(strcmp(token,empty)!=0){
            i++;
            if(i==1){
                cmd=(char**)malloc(i*sizeof(char*));
            }else{
                cmd=(char**)realloc(cmd,i*sizeof(char*));
            }
            cmd[i-1]=(char*)malloc(strlen(token)*sizeof(char));
            strcpy(cmd[i-1],token);
        }
    }
    
    //Checking if command is to change the directory
    //Since chdir() method has to be executed in parent process
    if(strcmp(cmd[0],"cd")==0){      
        int msg = chdir(cmd[1]);
        if(msg==-1){
            printf("Shell: Incorrect command\n");
            exit(0);
        }
    }else{
        //Executing commands other than change directory by forking a new child process
        int pid=fork();
        if(pid==0){  
            //Child process can accept signal like CTRL+C for termination
            //Therefore activating signal with default handler   
            signal(SIGINT, SIG_DFL);
            
            //Executing command using execvp and also catching error while executing commands      
            if(execvp(cmd[0],cmd)<0){
                printf("Shell: Incorrect command\n");
                exit(0);
            }
        }else if(pid<0){
            exit(0);
        }else{
            //Parent will wait till child with process ID pid to complete it's execution
            wait(&pid);
        }
    }
}

//Executing multiple commands sequentially
//cmds is the count of commands to execute
//We can execute change directory command in sequential execution as we can ensure which process will be scheduled at what time
void executeSequentialCommands(char command[Max_cmd][Max_cmd_len],int cmds){
    //Looping to execute commands in sequential order
    for(int i=0;i<cmds;i++){
        //Executing each command using executeCommand() function
        //Next command will only get executed when control from executeCommand() function comes back to this point
        //And executeCommand() function will terminate only when it's child has executed the command
        //Therefore we ensured that next command will get executed only after complete execution of previos command
        executeCommand(command[i]);
    }
}

//Executing multiple commands parallely
//cmds is the count of command to execute and startCmd is the command to be executed in this function call
//We will use recursive calls with incremented startCmd value to execute mutliple commands parallely
//We cannot execute change directory command in parallel execution as we don't know which process will be scheduled when.
void executeParallelCommands(char command[Max_cmd][Max_cmd_len],int cmds,int startCmd){
    //Execute only if sartCmd is less than cmds
    if(startCmd<cmds){
        //This part of the code is similar to executeCommand() function with one change in parent process.
        int pid=fork();
        if(pid==0){
            signal(SIGINT, SIG_DFL);
            char** cmd;
            int i=0;
            char *token,*str,*empty;
            str=(char*)malloc(strlen(command[startCmd])*sizeof(char));
            strcpy(str,command[startCmd]);
            empty = strdup("");
            while((token=strsep(&str," "))!=NULL){
                if(strcmp(token,empty)!=0){
                    i++;
                    if(i==1){
                        cmd=(char**)malloc(i*sizeof(char*));
                    }else{
                        cmd=(char**)realloc(cmd,i*sizeof(char*));
                    }
                    cmd[i-1]=(char*)malloc(strlen(token)*sizeof(char));
                    strcpy(cmd[i-1],token);
                }
            }
            if(execvp(cmd[0],cmd)<0){
                printf("Shell: Incorrect command\n");
                exit(0);
            }
        }else if(pid<0){
            exit(0);
        }else{
            //Parent process will recursively call function which will execute next command
            executeParallelCommands(command,cmds,startCmd+1);
            //When control again comes this point, parent will wait fir it's child process to completely execute command
            //Parent is ensured that command execution by its recursive next call is completed
            //If it's child process has completely executed the command before control comes at this point, parent no longer has to wait.
            wait(&pid);
        }
    }
}

//Execute the command with output redirected to different file
//This function is also similar to executeCommand() function with a slight change
void executeCommandRedirection(char command[Max_cmd][Max_cmd_len]){
    int pid=fork();
    if(pid<0){
        exit(0);
    }else if(pid==0){
        signal(SIGINT, SIG_DFL);
        char** outFile;
        int i=0;
        char *token,*str,*empty;
        empty = strdup("");
        str=(char*)malloc(strlen(command[1])*sizeof(char));
        strcpy(str,command[1]);
        
        //Parsing the file name which is the redirected output file
        while((token=strsep(&str," "))!=NULL){
            if(strcmp(token,empty)!=0){
                i++;
                if(i==1){
                    outFile=(char**)malloc(i*sizeof(char*));
                }else{
                    outFile=(char**)realloc(outFile,i*sizeof(char*));
                }
                outFile[i-1]=token;
            }
        }
        //For child process we are closing standard output file
        close(STDOUT_FILENO);
        //Opening the output file if it already exist/ create a new file and open
        open(outFile[0],O_CREAT | O_WRONLY | O_APPEND);
        char** cmd;
        i=0;
        str=(char*)realloc(str,strlen(command[0])*sizeof(char));
        strcpy(str,command[0]);
        while((token=strsep(&str," "))!=NULL){
            if(strcmp(token,empty)!=0){
                i++;
                if(i==1){
                    cmd=(char**)malloc(i*sizeof(char*));
                }else{
                    cmd=(char**)realloc(cmd,i*sizeof(char*));
                }
                cmd[i-1]=token;
            }
        }
        if(execvp(cmd[0],cmd)<0){
            printf("Shell: Incorrect command\n");
            exit(0);
        }
    }else{
        wait(&pid);
    }    
}

int main(){
    char *input=NULL;
    ssize_t input_len;
    ssize_t len=0;
    char command[Max_cmd][Max_cmd_len];
    char currentWorkingDirectory[Size];
    char type;
    int cmds;
    
    //Ignoring the process temination signal for parent process as we don't want our shell process to terminate due to it
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    while(1){
        //Fetching the current wirking directory
        getcwd(currentWorkingDirectory,Size);
        printf("%s$",currentWorkingDirectory);
        //Fetching the input command
        input_len=getline(&input,&len,stdin);
        input[strlen(input)-1]='\0';
        cmds=0;
        //Parsing the command
        type = parseInput(input,command,&cmds);
        //Terminate the shell if command is "exit".
        if(strcmp(command[0],"exit")==0){
            printf("Exiting shell...\n");
            break;
        }
        //Redirecting to various if else block according to type of execution returned by parseInput() functio
        if(type=='&'){
            executeParallelCommands(command,cmds,0);
        }else if(type=='#'){
            executeSequentialCommands(command,cmds);
        }else if(type=='>'){
            executeCommandRedirection(command);
        }else{
            executeCommand(command[0]);
        }
    }
}