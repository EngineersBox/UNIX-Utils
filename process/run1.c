#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/wait.h> 

int main(int argc, char* argv[]) {
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork failed\n");
		exit(1);
	} else if (pid == 0) {
		// Child
		char** args = (char**) calloc(sizeof(*args), argc);
		for (int i = 0; i < argc - 1; i++) {
			args[i] = strdup(argv[i + 1]);
		}
		args[argc] = NULL;
		int res = execvp(argv[1], args);
		if (res) {
			printf("\n****\nrun1 (child): execvp error\n");
		}
		exit(res);
	} else {
		// Parent
		int stat_loc = 0;
		waitpid(pid, &stat_loc, 0);
		if (WIFEXITED(stat_loc)) {
			printf("\n****\nrun1 (parent): child terminated with status: %d\n", WEXITSTATUS(stat_loc));
		}
	}
	return 0; 
}
