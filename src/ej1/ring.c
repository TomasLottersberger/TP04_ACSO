#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
    int start, n, initial_value;

    if (argc != 4) { // check invalid number of arguments
        fprintf(stderr, "Uso: anillo <n> <c> <s>\n");
        fprintf(stderr, "  n: número de procesos (>= 1)\n");
        fprintf(stderr, "  c: valor inicial\n");
        fprintf(stderr, "  s: proceso inicial (0 <= s < n)\n");
        exit(1);
    }
    
    n = atoi(argv[1]);
    initial_value = atoi(argv[2]);
    start = atoi(argv[3]);
    
    // arguments validation
    if (n <= 0) {
        fprintf(stderr, "ERROR: el número de procesos debe ser >= 1\n");
        exit(1);
    }
    
    if (start < 0 || start >= n) {
        fprintf(stderr, "ERROR: el proceso inicial debe estar entre 0 y %d\n", n-1);
        exit(1);
    }
    
    int pipes[n][2];
    pid_t pids[n];
    
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    
    for (int i = 0; i < n; i++) { // creation of son's processes
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            exit(1);
        }
        
        if (pids[i] == 0) {
            
            int value;
            int read_pipe = i;
            int write_pipe = (i + 1) % n;
            
            for (int j = 0; j < n; j++) { // close unused pipes
                if (j == read_pipe) {
                    close(pipes[j][1]);
                } else if (j == write_pipe) {
                    close(pipes[j][0]);
                } else {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            
            if (read(pipes[read_pipe][0], &value, sizeof(int)) != sizeof(int)) {
                perror("read");
                exit(1);
            }
            close(pipes[read_pipe][0]);
            value++;
            
            if (write(pipes[write_pipe][1], &value, sizeof(int)) != sizeof(int)) {
                perror("write");
                exit(1);
            }
            close(pipes[write_pipe][1]);
            
            exit(0);
        }
    }
    
    int final_read_pipe = start;
    
    for (int i = 0; i < n; i++) {
        if (i != final_read_pipe) {
            close(pipes[i][0]);
        }
        if (i != start) {
            close(pipes[i][1]);
        }
    }
    
    if (write(pipes[start][1], &initial_value, sizeof(int)) != sizeof(int)) {
        perror("write");
        exit(1);
    }

    close(pipes[start][1]);
    
    for (int i = 0; i < n; i++) { // wait for sons
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
        }
    }
    
    int final_result;
    if (read(pipes[final_read_pipe][0], &final_result, sizeof(int)) != sizeof(int)) {
        
        final_result = initial_value + n;
    }
    close(pipes[final_read_pipe][0]);
    
    printf("Resultado final: %d\n", final_result);
    
    return 0;
}
