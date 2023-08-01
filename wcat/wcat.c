#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <poll.h>

// Splice flags:
// * SPLICE_F_MOVE: Move pages instead of copy
// * SPLICE_F_NONBLOCK: Don't block on IO (dependent on file behaviour an O_NONBLOCK)
// * SPLICE_F_MORE: Notify that potentially more to stream in next round
#define handled_splice(src, dst, error_var, error_msg) \
	error_var = splice( \
		(src), NULL, \
		(dst), NULL, \
		buf_size, \
		SPLICE_F_MOVE | SPLICE_F_NONBLOCK | SPLICE_F_MORE \
	); \
	if ((error_var) < 0) { \
		close(descriptors[0]); \
		close(descriptors[1]); \
		perror((error_msg)); \
		return (error_var); \
	}

int kernel_bufcpy(int file, int file_len) {
	off_t len = file_len;
	int buf_size = 4096;
	int descriptors[2];
	int error;
	if ((error = pipe(descriptors)) < 0) {
		perror("wcat: nable to create pipe");
		return error;
	}
	// Use kernel only stream copy between file and stdout descriptors to avoid copy into user space
	// Man: https://man7.org/linux/man-pages/man2/splice.2.html
	ssize_t cp_to_pipe = 1;
	ssize_t cp_from_pipe = 1;
	while ((file_len == 0 && cp_to_pipe > 0 && cp_from_pipe > 0) || len > 0) {
		if (buf_size > len) {
			buf_size = len;	
		}
		handled_splice(file, descriptors[1], cp_to_pipe, "wcat: spliced stream from source to pipe failed");
		handled_splice(descriptors[0], STDOUT_FILENO, cp_from_pipe, "wcat: spliced stream from pipe to stdout failed");
		// Termination should be based on splice saturation, whether a full buffer was piped or not
		len = cp_from_pipe < buf_size ? 0 : len - buf_size;
	}
	close(descriptors[0]);
	close(descriptors[1]);
	return 0;
}

int stdin_has_data() {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0; // Return immediately
	// Effective poll stdin to determine if buffer has data
	return select(sizeof(fds) * 8, &fds, NULL, NULL, &timeout);
}

int main(int argc, char* argv[]) {
	// Determine if we are in a terminal context, i.e. /dev/tty
	if (isatty(STDOUT_FILENO)) {
		// Ensure that stdout (file descriptor 1) is not opened in append only mode,
		// this is fatal for splice(...)
		fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) & ~O_APPEND);
	}
	int error;
	// Ignore stdin when the buffer is empty (note that there is no fully portable implementation for this)
	if (stdin_has_data() > 0) {
		if ((error = kernel_bufcpy(STDIN_FILENO, INT_MAX)) < 0) {
			return error;
		}
	}
	for (int i = 1; i < argc; i++) {
		// Retrieve FS object stats to get file size
		struct stat file_stats;
		if (stat(argv[i], &file_stats) < 0) {
			printf("wcat: cannot open file\n");
			return 1;
		}
		// Open our file to stream to stdout
		int file = open(argv[i], O_RDONLY);
		if (file < 0) {
			perror("wcat: nable to open file");
			return file;
		}
		if ((error = kernel_bufcpy(file, file_stats.st_size)) < 0) {
			close(file);
			return error;
		}
		close(file);
	}
    return 0; 
}
