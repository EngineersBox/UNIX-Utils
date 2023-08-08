#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void handler(int sig) {
    printf("Signal %d received\n", sig);
}

int main() {
	struct sigaction handler_action = { 0 };
	handler_action.sa_handler = &handler;
	if (sigaction(SIGINT, &handler_action, NULL) < 0) {
		perror("sighandler: sigaction failed");
		return 1;
	}
	printf("Hello, send me a signal. My proces id is %d\n", getpid());
	printf("Type 'exit' to quit this program at any time.\n");
	char input[4];
	while (1) {
		printf("(waiting for signal) >");
		scanf("%s", input);
		if (strncmp(input, "exit", 4) == 0) {
			return 0;
		}
	}
	return 0; 
}

