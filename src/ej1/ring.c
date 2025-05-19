#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{   
    if (argc != 4) {
        fprintf(stderr, "Use: ring <n> <c> <s>\n");
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    int value = atoi(argv[2]);
    int s = atoi(argv[3]);
    if (n < 3 || s < 1 || s > n) {
        fprintf(stderr, "n would be >=3 and s beetwen 1 and n\n");
        exit(EXIT_FAILURE);
    }
    int start = s - 1;

    int pipes[n][2]; // creation of n pipes for ring
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pids[n]; // child's fork
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
			
            for (int j = 0; j < n; j++) { // close non-used pipes
                if (j != i) close(pipes[j][1]);               
                if (j != (i + n - 1) % n) close(pipes[j][0]);
            }
            int buf;
			
            if (read(pipes[(i + n - 1) % n][0], &buf, sizeof(buf)) != sizeof(buf)) { // read previous
                perror("read child");
                exit(EXIT_FAILURE);
            }
            buf += 1;
			
            if (write(pipes[i][1], &buf, sizeof(buf)) != sizeof(buf)) { // write next
                perror("write child");
                exit(EXIT_FAILURE);
            }
			
            close(pipes[(i + n - 1) % n][0]);
            close(pipes[i][1]);
            exit(EXIT_SUCCESS);
        }
		
        pids[i] = pid;
    }

    for (int j = 0; j < n; j++) {
        if (j != (start + n - 1) % n) close(pipes[j][0]);
        if (j != (start + n - 1) % n) close(pipes[j][1]);
    }

    if (write(pipes[(start + n - 1) % n][1], &value, sizeof(value)) != sizeof(value)) {
        perror("write father");
        exit(EXIT_FAILURE);
    }

    int result; // read final result
    if (read(pipes[(start + n - 1) % n][0], &result, sizeof(result)) != sizeof(result)) {
        perror("read father");
        exit(EXIT_FAILURE);
    }
    printf("Final result: %d\n", result);

    close(pipes[(start + n - 1) % n][0]);
    close(pipes[(start + n - 1) % n][1]);

    for (int i = 0; i < n; i++) { // wait for children
        waitpid(pids[i], NULL, 0);
    }

    return EXIT_SUCCESS;
}