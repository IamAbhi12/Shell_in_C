
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXCOM 1024 //maximum length of command line
#define MAXLIST 100 //maximum number of arguments

void init_shell()
{
    printf("\033[H\033[J"); //to clear the screen at the time of initialisation

    printf("\n\n\n\t****CS242 ASSIGNMENT****");
    printf("\n\n\tMINI SHELL USING C");
    printf("\n\n\n\n=======================================================");
    printf("\n");

    sleep(1);

    printf("\033[H\033[J");
}

//to store input command line as string
int takeInput(char *str)
{
    int bufsize = MAXCOM, position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;
    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
        return 1;
    }

    while (1)
    {
        c = getchar();
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            strcpy(str, buffer);
            return 0;
        }
        else
        {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize)
        {
            bufsize += MAXCOM;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
                return 1;
            }
        }
    }
}

//to print the prompt before entering the command
void printPrompt()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd)); //gets the current working directory
    printf("\n%s", cwd);
    printf("$ ");
}

//uses fork system call to create a child process and execute commands using execvp
void execArgs(char **parsed)
{

    pid_t pid = fork();

    if (pid == -1)
    {
        printf("\nFailed forking child..");
        return;
    }
    else if (pid == 0)
    {
        if (execvp(parsed[0], parsed) < 0)
        {
            printf("\nCould not execute command..");
        }
        exit(0);
    }
    else
    {

        wait(NULL);
        return;
    }
}

//executing piped commands
void execArgsPiped(char **parsed, char **parsedpipe)
{

    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0)
    {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0)
    {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0)
    {

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0)
        {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    }
    else
    {

        p2 = fork();

        if (p2 < 0)
        {
            printf("\nCould not fork");
            return;
        }

        if (p2 == 0)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0)
            {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        }
        else
        {

            wait(NULL);
            wait(NULL);
        }
    }
}

//a short guide for the shell
void openHelp()
{
    puts("\n***HELP***"
         "\nList of Commands supported:"
         "\n>cd"
         "\n>ls"
         "\n>exit"
         "\n>all other general commands available in UNIX shell"
         "\n>pipe handling");

    return;
}

//to run the builtin commands of shell
int BuiltinCommandHandler(char **parsed)
{
    int NumOfBuiltinCommands = 4, i, switchOwnArg = 0;
    char *BuiltinCommands[NumOfBuiltinCommands];
    char *username;

    BuiltinCommands[0] = "exit";
    BuiltinCommands[1] = "cd";
    BuiltinCommands[2] = "help";
    BuiltinCommands[3] = "hello";

    for (i = 0; i < NumOfBuiltinCommands; i++)
    {
        if (strcmp(parsed[0], BuiltinCommands[i]) == 0)
        {
            switchOwnArg = i + 1;
            break;
        }
    }

    switch (switchOwnArg)
    {
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        chdir(parsed[1]);
        return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
        username = getenv("USER");
        printf("\nHello %s.\nThis is a basic shell designed in C programming language."
               "\nUse the command help to know more...\n",
               username);
        return 1;
    default:
        break;
    }

    return 0;
}

//parsing pipes in the command line
int pipeParser(char *str, char **strpiped)
{

    strpiped[0] = strtok(str, "|");
    strpiped[1] = strtok(NULL, "|");
    if (strpiped[0] == NULL || strpiped[1] == NULL)
        return 0;
    else
        return 1;
}

//parsing spaces in the command line
void spaceParser(char *str, char **parsed)
{
    int i;
    char *delim = " \t";
    parsed[0] = strtok(str, delim);
    for (i = 1; i < MAXLIST; i++)
    {
        parsed[i] = strtok(NULL, delim);

        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

//after taking the command as input, it is processed here
int processString(char *str, char **parsed, char **parsedpipe)
{

    char *strpiped[2];
    int piped = 0;

    piped = pipeParser(str, strpiped);

    if (piped)
    {
        spaceParser(strpiped[0], parsed);
        spaceParser(strpiped[1], parsedpipe);
    }
    else
    {

        spaceParser(str, parsed);
    }

    if (BuiltinCommandHandler(parsed))
        return 0;
    else
        return 1 + piped;
}

int main()
{
    char inputString[MAXCOM], *parsedArgs[MAXLIST];
    char *parsedArgsPiped[MAXLIST];
    int execFlag = 0;
    init_shell();

    while (1)
    {

        printPrompt();
        FILE *history_ptr;
        history_ptr = fopen("/tmp/history.txt", "a+");

        if (takeInput(inputString))
            continue;

        fprintf(history_ptr, "%s", inputString);

        execFlag = processString(inputString,
                                 parsedArgs, parsedArgsPiped);

        if (execFlag == 1)
            execArgs(parsedArgs);

        if (execFlag == 2)
            execArgsPiped(parsedArgs, parsedArgsPiped);
    }
    return 0;
}