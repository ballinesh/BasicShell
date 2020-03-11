//Hesiquio Ballines
//This is the reference I used to be able to pipe multiple commands using | https://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell
//I did not copy code straight up but I read and applied it to my functions



#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
//Function Prototypes
void eval (char *cmdLine, pid_t parentID,int shellTerminal);
int parseLine(char *buffer, char **argv);
int builtInCommand(char **argv);
int containsPipingCommand(char **argv);
void pipingCommand(char **argv, int noPipes);

//Signal Handlers
void sigintHandler(int signum)
{
    write(1, "Caught: Interrupt\n" ,19);
    exit(0);
}
void sigchldHandler(int signum)
{
     int status;
     pid_t processID = 0;

        
        while((processID = waitpid(-1, &status, WNOHANG) ) > 0) 
        {
            char buf[40];
            int size = snprintf(buf, 40, "pid:%d status:%d\n", (int)processID,status);
            write(1,buf,strlen(buf));
        }
       
        
        //wait(NULL);
}

void sigtermHandler(int signum)
{
    write(1,"Caught: Terminated",19);
    kill(-getpid(), SIGQUIT);
    exit(0);
}

void sigcontHandler(int signum)
{
    write(1,"CS361 >",8);
}
/*void sigtstpHandler(int signum)
{
        pid_t masterID = getppid();
        setpgid(masterID,masterID);
        tcsetpgrp(STDIN_FILENO, masterID);
        tcsetpgrp(STDOUT_FILENO, masterID);
    
}*/
int main()
{
    char cmdLine[128];                          //Command line input
    char moddedCmdLine[128];
    pid_t parentID = getpid();
   
    //Signals
    signal(SIGINT , sigintHandler);            //Happens when Ctrl+C
    signal(SIGCHLD,sigchldHandler);            //Happens when a child terminates
    signal(SIGTERM,sigtermHandler);            //Terminates a process
    //signal(SIGTSTP,sigtstpHandler);          //Could not get to work
    signal(SIGCONT,sigcontHandler);
    signal(SIGSTOP,SIG_DFL);
    signal(SIGQUIT, SIG_IGN);
    

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    int terminalNum = STDIN_FILENO;
    
    setpgid(parentID, parentID);
    
    while(1)
    {
        write(1,"CS361 >",8);
        fgets(cmdLine, 128,stdin);
        
        //Find and add spaces to riderection flags/symbols
        int i = 0, j = 0;

        /*Formatting the input to work with the parser given 
        * This gets rid of the issue that something like ls|wc would not work
        * becuase there aren't spaces in between
        */
        while(cmdLine[i] != '\0') 
        {
            if(cmdLine[i] == '|')
            {
                moddedCmdLine[j] = ' ';
                moddedCmdLine[j+1] = '|';
                moddedCmdLine[j+2] = ' ';
                j+=3;
            }
            else if(cmdLine[i] == '>' && cmdLine[i+1] == '>')
            {
                moddedCmdLine[j] = ' ';
                moddedCmdLine[j+1] = '>';
                moddedCmdLine[j+2] = '>';
                moddedCmdLine[j+3] = ' ';
                j+=4;
                i++;
            }
            else if(cmdLine[i] == '>')
            {
                moddedCmdLine[j] = ' ';
                moddedCmdLine[j+1] = '>';
                moddedCmdLine[j+2] = ' ';
                j+=3;
            }
            else
            {
                moddedCmdLine[j] = cmdLine[i];
                j++;
                
            }
            i++;
        
        }

        moddedCmdLine[j] == '\0';

        if(feof(stdin))
        {
            exit(0);
        }

        eval(moddedCmdLine,parentID,terminalNum);                          //Evaluating the command line
        
        //Clearing the modded command array
        i = 0;
        while(moddedCmdLine[i] != '\0')
        {
            moddedCmdLine[i] = '\0';
            i++;
        }
    }
}



void eval (char * cmdLine, pid_t parentID, int shellTerminal)
{
    char *argv[128];
    char buf[128];
    char * environ [2];
    int bg;                                     //Determines whether the parent is in the Background
    int requiresPipe = 0;
    pid_t processID;

    strcpy(buf, cmdLine);
    bg = parseLine(buf,argv);

    if(argv[0] == NULL)
    {
        return;                                 //No argument so skip
    }

    requiresPipe = containsPipingCommand(argv);

    if (requiresPipe != 0)
    {
        pipingCommand(argv,requiresPipe);
        
    }
    if(!builtInCommand(argv) && requiresPipe == 0)   //Runs any command that cannot be run with execvp, background implementation used from lab and help from TA
    {
        
        int status;
        processID = fork();                     
       
        if(processID == 0)                      //If the process is a child then run the command
        {
           if(!bg)
           {
               setpgid(processID,processID);
               tcsetpgrp(STDIN_FILENO,processID);
           }
           

            if(execvp(argv[0] , argv) < 0)      //If exec can't run it then print an error
            {
                printf("%s: command not found.\n"  , argv[0]);
                exit(0);
            }
             
        }
        else if (processID < 0)
        {
            exit(1);
        }
        else
        {
                setpgid(processID,processID);
                if(!bg)
                {
                    
                    pid_t pid = waitpid(-1, &status, WNOHANG);
                    
                    
                }
    
                sleep(1); //Sleep used throughout to sync the prompt
        }
        tcsetpgrp(STDIN_FILENO,parentID);
    }
    return;
}

int builtInCommand(char **argv)     //These are the commands that are not covered by execvp
{
    if(!strcmp(argv[0], "exit"))
    {
        exit(0);
    }
    if(!strcmp(argv[0],"&"))
    {
        return 1;
    }
   if(!strcmp(argv[0], "cd"))       //To change directory the second argument of input is used
   {
       chdir(argv[1]);              //The function chdir takes in a directory
       return 1;                    //Returns 1 to show that cd is not a builtin function
   }
   if(!strcmp(argv[0], "/bin/cd"))  //To change directory the second argument of input is used
   {
       chdir(argv[1]);              //The function chdir takes in a directory
       return 1;                    //Returns 1 to show that cd is not a builtin function
   }
   
    return 0;
}

int containsPipingCommand(char **argv)      //This functions checks to see if the command line contains any processes that require piping or redirection
{
    int i = 0;

    
    while(argv[i] != NULL)
    {
            if (!strcmp(argv[i],"|"))
            { 
                
                return 1;
            }
            i++;
    }
    i = 0;
    while(argv[i] != NULL)
    {
            if (!strcmp(argv[i],">>") || !strcmp(argv[i],">") || !strcmp(argv[i], "<"))
            { 
                
                return 2;
            }
            i++;
    }
    return 0;
}

void pipingCommand(char **argv, int noPipes) //The function used to execute any processes requiring redirection or piping          
{                                           
    //printf("In the piping command function\n");
    int i =0, z = 0, endIndex = 0,previousI = 0, k = 0, executedFlag = 0;
    int pipeEnds[2];
    char* command[5] = {NULL,NULL,NULL,NULL,NULL};
    int inputPipe = fileno(stdin);              //The first command should read from stdin, will change as num processes increases
    int stdoutOriginal = fileno(stdout);
    int redirectFlag = 0; 
    int fileIndex = 0;
   
    while(argv[i] != NULL)
    {
        
        executedFlag = 0;
        k=0;
        if (!strcmp(argv[i], "|"))  //Should add support for > and >> later
        {
            pipe(pipeEnds);
            //Parse the command
            for(int j = 0; (j+previousI) <i; j++)
            {
                command[j] = argv[previousI + j];
                endIndex = j + 1;
            }
            command[endIndex] = NULL;    //Make sure the last index is NULL (Ensures that the exec functions know where input ends)

            //This loop checks for input file if necessary, if found then redirect input from that file
            while ( command[k] != NULL)
            {
                if (!strcmp(command[k], "<"))
                {
                        fileIndex = k +1;
                        
                        int fd = open(command[fileIndex], O_RDONLY);
                        if (fork() == 0)
                        {
                            dup2(fd,0);
                            if(pipeEnds[1] != 1)
                            {
                                dup2(pipeEnds[1],1);
                                close(pipeEnds[1]);
                            }
                            command[fileIndex - 1] = NULL;          //Lets exec know when to stop
                            execvp(command[0],command);
                            exit(0);
                        }

                        else
                        {
                            sleep(1);
                            //wait(NULL);
                            executedFlag = 1;
                        }
                        
                    
                    //Command executed with file input so we 
                    inputPipe = pipeEnds[0];
                }
                k++;
            }
            //End of check

            //Execute the commands and setup pipes, if it was not executed already
            //At the same time redirecting output and input
            if (!executedFlag)
            {
                if(fork() == 0)
                {
                    if(inputPipe != 0)
                    {
                        dup2(inputPipe,0);
                    }
                    if(pipeEnds[1] != 1)
                    {
                        dup2(pipeEnds[1],1);
                        close(pipeEnds[1]);
                    }
                    
                    execvp(command[0],command);
                    exit(0);
                }
                else
                {
                    sleep(1);
                    
                }
                inputPipe = pipeEnds[0]; //The output of this pipe should be the input of the next pipe 
            }
            
            

            close(pipeEnds[1]);

            

            //Resseting the command array
            for(int k = 0; k<5;k++)
            {
                command[k] = NULL;
            }
            //Updating index for the next command
            previousI = i +1;
        }

        //Parseing of input and setting a flag, redirection done later
        if(!strcmp(argv[i], ">>"))
        {
            redirectFlag = 1;
            fileIndex = i+1;
            int trackSize = 0;

           // printf("Previous: %d\n" , previousI);
            //printf("I: %d", i);

            for(int k = 0; (previousI+k) < i; k++)
            {
                command[k] = argv[previousI+k];
                //printf("Test: %s\n", command[k]);
                
            }
            break;
        }

        if(!strcmp(argv[i], ">"))
        {
            redirectFlag = 2;
            fileIndex = i+1;
            int trackSize = 0;

           // printf("Previous: %d\n" , previousI);
            //printf("I: %d", i);

            for(int k = 0; (previousI+k) < i; k++)
            {
                command[k] = argv[previousI+k];
                //printf("Test: %s\n", command[k]);
                
            }
            break;
        }

        if(!strcmp(argv[i], "<") && noPipes == 2)  //Since this can come earlier in than '|' we don't parse if a pipe is necessary
        {
            redirectFlag = 3;
            fileIndex = i+1;
            int trackSize = 0;

            for(int k = 0; (previousI+k) < i; k++)
            {
                command[k] = argv[previousI+k];
                
            }
            break;
        }

       

        i++;
    }
    //printf("Testing flag is %d\n", redirectFlag);
    //The last command must be made
    i = previousI;

    if(redirectFlag == 0)   //The final command execution
    {
        for (int j = 0; argv[i] != NULL; j++)
        {
            command[j] = argv[i];
            //printf("%s\n",command[j]);
            i++;
        }

   
        
        if (fork() == 0)
        {
            if(inputPipe!= 0)
            {
                dup2(inputPipe,0);
            }

            dup2(1,stdoutOriginal);
            execvp(command[0],command);
            exit(0);
        }
        else
        {
            //wait(NULL);
            sleep(1);
            return;
        }
    }
   
    else if(redirectFlag == 1) //If a >> is found then append output to the file name
    {

        //Redirect and execute command
        int fd = open(argv[fileIndex], O_CREAT |  O_APPEND| O_WRONLY, 0666);
        

        if (fork() == 0)
        {
            if(inputPipe!= 0)
            {
                dup2(inputPipe,0);
            }
            dup2(fd,stdoutOriginal);
            execvp(command[0],command);
            exit(0);
        }

        else
        {
            wait(NULL);
            return;
        }


    }
    else if (redirectFlag == 2) //If a > is found then overwrite with the output to that file name
    {
        int fd = open(argv[fileIndex], O_CREAT | O_TRUNC | O_WRONLY,0666);
        

        if (fork() == 0)
        {
            if(inputPipe!= 0)
            {
                dup2(inputPipe,0);
            }
            dup2(fd,stdoutOriginal);
            execvp(command[0],command);
            exit(0);
        }

        else
        {
            wait(NULL);
            return;
        }
    }

     else if(redirectFlag == 3) //If a < is found then redirect that file to be input 
    {
        //printf("%s",argv[fileIndex]);

        int fd = open(argv[fileIndex], O_RDONLY);

        if (fork() == 0)
        {
            dup2(fd,0);
            dup2(1,stdoutOriginal);
            execvp(command[0],command);
            exit(0);
        }

        else
        {
            wait(NULL);
            return;
        }
    }
    
    
    
    
    
}

int parseLine(char *buf,char **argv)        //Parses the input, separated by spaces (Function given by the book)
{
    char *delim;
    int argc;
    int bg;

    buf[strlen(buf) - 1] = ' ';
    
    while(*buf && (*buf == ' '))
    {
        buf++;
    }

    argc = 0;

    while((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while(*buf && (*buf == ' ')) 
        {
            buf++;
        }
    }

    argv[argc] = NULL;

    if(argc == 0)
    {
        return 1;
    }

    if((bg = (*argv[argc-1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }

    return bg;
}