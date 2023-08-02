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

#define read_int(value, initial) \
	(value) |= (initial) << (8 * 0); \
	(value) |= src->get(src) << (8 * 1); \
	(value) |= src->get(src) << (8 * 2); \
	(value) |= src->get(src) << (8 * 3)

void rle_decode(const DataStream src, const DataStream dst) {
	int c, len;
	while ((c = src->get(src)) != EOF) {
		len = 0;
		read_int(len, c);
		if ((c = src->get(src)) == EOF) {
			printf("wunzip: invalid archive\n");
			exit(1);
		}
		for (int i = 0; i < len; i++) {
			dst->put(dst, c);
		}
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

int main(int argc, char *argv[]) { 
	FileStream dst = new_file_stream(fdopen(STDOUT_FILENO, "a"));
	if (dst.fp == NULL) {
		printf("wunzip: cannot open stdout\n");
		return 1;
	}
	if (stdin_has_data()) {
		FileStream src = new_file_stream(fdopen(STDIN_FILENO, "rb+"));
		if (src.fp == NULL) {
			printf("wunzip: cannot open stdin\n");
			fclose(dst.fp);
			return 1;
		}
		rle_decode((DataStream) &src, (DataStream) &dst);
		fclose(src.fp);
	} else if (argc < 2) {
		printf("wunzip: file1 [file2 ...]\n");
		return 1;
	}
	for (int i = 1; i < argc; i++) {
		FileStream src = new_file_stream(fopen(argv[i], "rb+"));
		if (src.fp == NULL) {
			printf("wunzip: cannot open file\n");
			fclose(dst.fp);
			return 1;
		}
		rle_decode((DataStream) &src, (DataStream) &dst);
		fclose(src.fp);
	}
	fclose(dst.fp);
    return 0; 
}
