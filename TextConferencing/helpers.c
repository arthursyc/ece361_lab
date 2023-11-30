#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include "helpers.h"

char** parse(char* buffer, char delim[]) {
	int arr_size = 0;
	char** array = (char**) malloc(sizeof(char*));

	char* ptr = strtok(buffer, delim);
	while (ptr != NULL) {
		array = (char**) reallocarray(array, ++arr_size, sizeof(char*));
		array[arr_size - 1] = ptr;
		ptr = strtok(NULL, delim);
	}

	return array;
}

struct message getMessage(int sockfd, bool timeout) {
	char buffer[1024];
	bzero(buffer, sizeof(buffer));

	if (timeout) {
		fd_set socks;
		FD_ZERO(&socks);
		FD_SET(sockfd, &socks);
		struct timeval t;
		t.tv_sec = 0;
		t.tv_usec = 500000;
		if (!(select((sockfd + 1), &socks, NULL, NULL, &t))) {
			struct message msg;
			msg.type = NONE;
			msg.size = 0;
			memset(msg.source, 0, MAX_NAME);
			memset(msg.data, 0, MAX_DATA);
			return msg;
		}
	}

	read(sockfd, buffer, sizeof(buffer));

	struct message msg;
	char** array = parse(buffer, ":");
	msg.type = atoi(array[0]);
	msg.size = atoi(array[1]);
	memcpy(msg.source, array[2], sizeof(array[2]));
	memcpy(msg.data, array[3], msg.size);
	free(array);
	return msg;
}

struct session* findSess(char query[MAX_NAME], struct session* sess_head) {
	struct session* cur = sess_head;
	while (cur != NULL) {
		if (strcmp(cur->id, query) == 0) {
			return cur;
		} else {
			cur = cur->next;
		}
	}
	return NULL;
}
