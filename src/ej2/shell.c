#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 100

int main() {
    char line[1024];
    char *commands[MAX_COMMANDS];
    int command_count;

    while (1) {
        
        printf("Shell> ");
        if (!fgets(line, sizeof(line), stdin)) {
            break; // EOF
        }

        line[strcspn(line, "\n")] = '\0'; // remove newline
        
        command_count = 0; // split by pipe
        char *token = strtok(line, "|");
        while (token && command_count < MAX_COMMANDS) {
            
            while (*token == ' ') token++; // trim leading/trailing spaces
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') *end-- = '\0';
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }
        if (command_count == 0) continue;

        int pipefd[MAX_COMMANDS-1][2]; // pipes creation
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pids[MAX_COMMANDS]; // forks for every command
        for (int i = 0; i < command_count; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                
                if (i > 0) {
                    dup2(pipefd[i-1][0], STDIN_FILENO);
                }
                if (i < command_count - 1) {
                    dup2(pipefd[i][1], STDOUT_FILENO);
                }
                
                for (int j = 0; j < command_count - 1; j++) { // close pipes
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                
                char *args[MAX_ARGS]; // parse args
                int argc = 0;
                char *arg = strtok(commands[i], " ");
                while (arg && argc < MAX_ARGS-1) {
                    args[argc++] = arg;
                    arg = strtok(NULL, " ");
                }
                args[argc] = NULL;
                if (argc == 0) exit(EXIT_SUCCESS);
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            pids[i] = pid;
        }

        
        for (int i = 0; i < command_count - 1; i++) { // father: close every corner side
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        
        for (int i = 0; i < command_count; i++) { // wait for sons
            waitpid(pids[i], NULL, 0);
        }
    }

    printf("Exiting shell.\n");
    return 0;
}