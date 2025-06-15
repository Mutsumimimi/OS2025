#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_CMD_ARG_NUM 32

int main(){
    // char *argv[MAX_CMD_ARG_NUM];
    // char *commands[128];
    // int argc;
    // int fd[2];

    // char string[] = "ls  -a  b c";
    // *commands = string;

    // // malloc?
    // for(int i = 0; i<MAX_CMD_ARG_NUM; i++)
    //     *(argv + i) = (char *)malloc(MAX_CMD_ARG_NUM * sizeof(char));
    // //
    // argc = 0;
    // int commands_i, argv_n;
    // commands_i = 0; argv_n = 0;
    // while(1) {
    //     if(commands[0][commands_i] == ' ' ){
    //         if(argv_n == 0){
    //             commands_i++;
    //             continue;
    //         }
    //         argv[argc][argv_n+1] = '\0';
    //         argc++;
    //         argv_n = 0;
    //     } else if(commands[0][commands_i] == '\0'){
    //         if(argv_n == 0){
    //             break;
    //         }
    //         argv[argc][argv_n+1] = '\0';
    //         argc++;
    //         argv_n = 0;
    //         break;
    //     } else {
    //         argv[argc][argv_n] = commands[0][commands_i];
    //         argv_n++;
    //     }
    //     commands_i++;
    // }
    // for(int i = 0; i<MAX_CMD_ARG_NUM; i++){
    //     if(argv[i][0] != '\0')
    //         printf("%s\n", argv[i]);
    //     else
    //         break;
    // }
    // printf("%d\n", argc);

    char str[] = "1024";
    int pid = 0;
    for(int i = 0; ; i++){
        if(*(str+i) == '\0')
            break;
        pid = pid *10 + (str[i]-48);
    }
    printf("%d\n", pid);

    return 0;
}

int execute(int argc, char ** argv){
    /* TODO:运行命令与结束 */
    int pid = fork();
    if(pid == -1){
        fprintf(stderr, "Usage: fork()\n");
        exit(-1);
    } else if(pid == 0){
        // 子进程
        if(execvp(argv[0], argv) == -1){
            // fprintf(stderr, "Usage: execvp()\n");
            return -1;
        }
    } else {
        // 父进程
        wait(NULL);
        return 0;
    }
}