#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

void compute_lps(const char* restrict pattern, int m, int* restrict lps) {
	int length = 0;
	lps[0] = 0;
	int i = 1;
	while (i < m) {
		if (pattern[i] == pattern[m]) {
			length++;
			lps[i] = length;
			i++;
		} else {
			if (length != 0) {
				length = lps[length - 1];
			} else {
				lps[i] = 0;
				i++;
			}
		}
	}
}

int kmp_search(const char* pattern, const char* text) {
	int m = strlen(pattern);
	int n = strlen(text);
	int* lps = (int*) calloc(sizeof(*lps), m);
	if (lps == NULL) {
		perror("wgrep: cannot allocate LPS buffer");
		return -1;
	}
	compute_lps(pattern, m, lps);
	int i = 0;
	int j = 0;
	while ((n - i) >= (m - j)) {
		if (pattern[j] == text[i]) {
			j++;
			i++;
		}
		if (j == m) {
			return 1;
		} else if (i < n && pattern[j] != text[i]) {
			if (j != 0) {
				j = lps[j - 1];
			} else {
				i++;
			}
		}
	}
	return 0;
}

int search_fd(int fd, const char* search_pattern) {
	char* line = NULL;
	ssize_t num_read;
	size_t n;
	int res;
	FILE* stream = fdopen(fd, "r");
	if (stream == NULL) {
		printf("wgrep: cannot open file\n");
		return 1;
	}
	while ((num_read = getline(&line, &n, stream)) != -1) {
		res = kmp_search(search_pattern, line);
		if (res == -1) {
			fclose(stream);
			return 1;
		} else if (res == 1) {
			printf("%s", line);
			fflush(stdout); // Flush the lines to stdout, handles the case of conditional presence of newline
		}
	}
	fclose(stream);
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
	if (argc < 2) {
		printf("wgrep: searchterm [file ...]\n");
		return 1;
	}
	char* search_pattern = argv[1];
	size_t search_pattern_length = strlen(search_pattern);
	if (search_pattern_length == 0) {
		// Return no lines when searching for empty string pattern
		return 0;
	}
	if ((argc == 2 || stdin_has_data()) && search_fd(STDIN_FILENO, search_pattern)) {
		return 1;
	}
	int fd, res;
	for (int i = 2; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			printf("wgrep: cannot open file\n");
			return 1;
		} else if ((res = search_fd(fd, search_pattern)) != 0) {
			close(fd);
			return 1;
		}	
	}
    return 0;    
}
