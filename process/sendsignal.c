#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("usage: sendsignal <SIGNAL> <PROC ID>");
		return 1;
	}
	int signal = atoi(argv[1]);
	pid_t proc_id = atoi(argv[2]);
	int res = kill(proc_id, signal);
	if (res < 0) {
		char msg[100];
		sprintf(msg, "sendsignal: failed to send signal %d to %d", signal, proc_id);
		perror(msg);
	} else {
		printf("signalled %d to %d\n", signal, proc_id);
	}
    return res;
}
