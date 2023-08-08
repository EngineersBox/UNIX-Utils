#include <stdio.h>
#include <stdlib.h>
#include <proc/readproc.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	PROCTAB* proc_table = openproc(PROC_FILLSTATUS);
	proc_t* proc = NULL;
	while ((proc = readproc(proc_table, NULL)) != NULL) {
		if (proc->state != 'Z') {
			freeproc(proc);
			continue;
		}
		int ppid = proc->ppid;
		printf("\nFound zombie process: %d\n", ppid);
		printf("Sending SIGERM to parent process %d\n", ppid);
		int res = kill(ppid, SIGTERM);
		if (res < 0) {
			char msg[100];
			sprintf(msg, "reaper: unable to kill %d, skipping\n", ppid);
			perror(msg);
		} else {
			printf("Sent SIGTERM to parent process %d\n", ppid);
		}
		freeproc(proc);
	}
	closeproc(proc_table);
	return 0; 
}
