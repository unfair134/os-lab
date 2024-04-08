/*
    COMP3511 Spring 2024
    PA1: Simplified Linux Shell (MyShell)

    Your name:Qiu Tian
    Your ITSC email:   tqiuae@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls

#define MYSHELL_WELCOME_MESSAGE "COMP3511 PA1 Myshell (Spring 2024)"

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_CD_ERROR "Myshell cd command error\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:

#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
void show_prompt(char *prompt, char *path)
{
    printf("%s %s> ", prompt, path);
}

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// parse_arguments function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  parse_arguments(pipe_segments, cmdline, &num_pipe_segments, "|");
//
void parse_arguments(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *cmdline)
{
    // Uncomment this line to show the content of the cmdline
    // printf("cmdline is: %s\n", cmdline);
    char *argv[MAX_ARGUMENTS+1];
    int numTokens;
    // 解析命令行参数
    parse_arguments(argv, cmdline, &numTokens, SPACE_CHARS);
    argv[numTokens]=NULL;

    // 检查是否存在重定向符号
    for (int i = 0; i < numTokens; i++) {
        if (strcmp(argv[i], ">") == 0) {
            // 找到输出重定向符号，打开文件
            int fd = open(argv[i+1],  O_CREAT |O_WRONLY , S_IRUSR | S_IWUSR);

            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            // 重定向标准输出到文件
            close(1);
            dup(fd);
            // 删除重定向符号和文件名
            argv[i] = NULL;
        } else if (strcmp(argv[i], "<") == 0) {
            int fd = open(argv[i+1], O_RDONLY);

            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            close(0);
            dup(fd);
            // 删除重定向符号和文件名
            argv[i] = NULL;
        }
    }
    // 执行命令
  
    
    execvp(argv[0], argv);
    perror("execvp");

    char *pipe_segments[MAX_PIPE_SEGMENTS];
    int num_pipe_segments;
    parse_arguments(pipe_segments,cmdline,&num_pipe_segments,PIPE_CHAR);
    int pipefd[2*(num_pipe_segments-1)];
    for (int i=0;i<num_pipe_segments-1;i++){
        if(pipe(pipefd+i*2)<0){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
     for (int i = 0; i < num_pipe_segments; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // 如果不是最后一个命令，将stdout连接到下一个命令的输入
            if (i < num_pipe_segments - 1) {
                if (dup2(pipefd[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // 如果不是第一个命令，将stdin连接到上一个命令的输出
            if (i > 0) {
                if (dup2(pipefd[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // 关闭所有管道
            for (int j = 0; j < 2 * (num_pipe_segments - 1); j++) {
                close(pipefd[j]);
            }

            char *argv[MAX_ARGUMENTS];
            int numTokens;
            parse_arguments(argv, pipe_segments[i], &numTokens, SPACE_CHARS);

            // 确保参数列表的最后一个元素是NULL
            argv[numTokens] = NULL;

            // 执行命令
            if (execvp(argv[0], argv) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }

    // 在父进程中关闭所有管道
    for (int i = 0; i < 2 * (num_pipe_segments - 1); i++) {
        close(pipefd[i]);
    }

    // 等待所有子进程结束
    for (int i = 0; i < num_pipe_segments; i++) {
        wait(NULL);
    }
    

    exit(0); // ensure the process cmd is finished
}

/* The main function implementation */
int main()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    char *prompt = "tqiuae";
    char cmdline[MAX_CMDLINE_LENGTH];
    char path[256]; // assume path has at most 256 characters

    printf("%s\n\n", MYSHELL_WELCOME_MESSAGE);
    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        getcwd(path, 256);
        show_prompt(prompt, path);

        if (get_cmd_line(cmdline) == -1)
            continue; // empty line handling, continue and do not run process_cmd

        // TODO: Before running process_cmd
        //
        // (1) Handle the exit comman
	if (strcmp(cmdline,"exit")==0){
		printf(TEMPLATE_MYSHELL_END,getpid());
		break;
	}
        // (2) Handle the cd command
        if (strncmp(cmdline,"cd ",3)==0){
		char *dir_path=cmdline+3;
		if (chdir(dir_path)!=0){
			printf(TEMPLATE_MYSHELL_CD_ERROR);
		continue;
		}
	}
        // Note: These 2 commands should not be handled by process_cmd
        // Hint: You may call break or continue when handling (1) and (2) so that process_cmd below won't be executed

        pid_t pid = fork();
        if (pid == 0)
        {
            // the child process handles the command
            process_cmd(cmdline);
        }
        else
        {
            // the parent process simply wait for the child and do nothing
            wait(0);
        }
    }

    return 0;
}
