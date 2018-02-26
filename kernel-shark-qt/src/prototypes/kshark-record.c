#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "KsDeff.h"

int main(int argc, char **argv)
{
	int brass[2];
	if (pipe(brass) == -1) {
		perror("pipe");
		exit(1);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {
		dup2(brass[1], STDERR_FILENO);
		close(brass[1]);
		close(brass[0]);
		char *args[9];
		args[0] = "pkexec";
		asprintf(&args[1], "%s/trace-cmd", TRACECMD_BIN_DIR);
		args[2] = "record";
		args[3] = "-p";
		args[4] = "function";
		args[5] = "-o";
		asprintf(&args[6], "%s/trace_test.dat", KS_DIR);
		args[7] = "ls";
		args[8] = NULL;
		execvp(args[0], args);
		perror("execvp");
		_exit(1);
	}

	close(brass[1]);

	ssize_t count;
	char buffer[BUFSIZ+1];
	while ((count = read(brass[0], buffer, BUFSIZ)) > 0) {
		buffer[count] = 0;
		printf("monitor: >> %s <<\n", buffer);
	}

	close(brass[0]);
	wait(0);
}
