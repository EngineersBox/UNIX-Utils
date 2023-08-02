#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>

typedef struct DataStream *DataStream;

struct DataStream {
	int (*get)(DataStream);
	int (*put)(DataStream, int);
};

typedef struct StringStream {
	int (*get)(DataStream);
	int (*put)(DataStream, int);
	char* string;
	size_t pos;
} StringStream;

typedef struct FileStream {
	int (*get)(DataStream);
	int (*put)(DataStream, int);
	FILE* fp;
} FileStream;

int string_get(DataStream in) {
	int c;
	StringStream* s = (StringStream*) in;
	c = (unsigned char)(s->string[s->pos]);
	if (c == '\0') return -1;
	s->pos++;
	return c;
}

int string_put(DataStream out, int c) {
	StringStream* s = (StringStream*) out;
	s->string[s->pos++] = (c == -1) ? '\0' : c;
	if (c == -1) s->pos = 0;
	return 0;
}

StringStream new_string_stream(char* string, size_t pos) {
	return (StringStream){
		.get = string_get,
		.put = string_put,
		.string = string,
		.pos = pos
	};
}

int file_put(const DataStream out, int c) {
	const FileStream* f = (FileStream*) out;
	return fputc(c, f->fp);
}

int file_get(const DataStream in) {
	const FileStream* f = (FileStream*) in;
	return fgetc(f->fp);
}

FileStream new_file_stream(FILE* fp) {
	return (FileStream) {
		.get = file_get,
		.put = file_put,
		.fp = fp
	};
}

static int run_c = EOF;
static int curr_c = 0;
static int len = 0;

#define write_int(value)\
	dst->put(dst, (value) >> 8 * 0); \
	dst->put(dst, (value) >> 8 * 1); \
	dst->put(dst, (value) >> 8 * 2); \
	dst->put(dst, (value) >> 8 * 3)

void rle_encode(const DataStream restrict src, const DataStream restrict dst, bool terminate_on_eof) {
	while (curr_c != EOF) {
		int next_c = src->get(src);
		/*printf("%x -> %c ", next_c, (char) next_c);*/
		if (!terminate_on_eof && next_c == EOF) {
			break;
		}
		curr_c = next_c;
		/*printf("%d %d\n", run_c != EOF, run_c != curr_c);*/
		if (run_c != EOF && run_c != curr_c) {
			write_int(len);
			dst->put(dst, run_c);
			len = 1;
			run_c = curr_c;
			continue;
		}
		len++;
		run_c = curr_c;
	}
	if (terminate_on_eof) {
		run_c = EOF;
		curr_c = 0;
		len = 0;
	}
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
	FileStream dst = new_file_stream(fdopen(STDOUT_FILENO, "a"));
	if (dst.fp == NULL) {
		printf("wzip: cannot open stdout\n");
		return 1;
	}
	if (stdin_has_data()) {
		FileStream src = new_file_stream(fdopen(STDIN_FILENO, "rb+"));
		if (src.fp == NULL) {
			printf("wzip: cannot open stdin\n");
			fclose(dst.fp);
			return 1;
		}
		rle_encode((DataStream) &src, (DataStream) &dst, false);
		fclose(src.fp);
	} else if (argc < 2) {
		printf("wzip: file1 [file2 ...]\n");
		return 1;
	}
	for (int i = 1; i < argc; i++) {
		FileStream src = new_file_stream(fopen(argv[i], "rb+"));
		if (src.fp == NULL) {
			printf("wzip: cannot open file\n");
			fclose(dst.fp);
			return 1;
		}
		rle_encode((DataStream) &src, (DataStream) &dst, i == argc - 1);
		fclose(src.fp);
	}
	fclose(dst.fp);
    return 0; 
}
