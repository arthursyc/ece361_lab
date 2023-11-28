#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "helpers.h"

char** parse(char* buffer, char delim[]) {
	int arr_size = 0;
	char** array;

	char* ptr = strtok(buffer, delim);
	while (ptr != NULL) {
		array = (char**) reallocarray(array, ++arr_size, sizeof(char*));
		array[arr_size - 1] = ptr;
		ptr = strtok(NULL, delim);
	}

	return array;
}

struct message getMessage(int sockfd) {
	char buffer[1024];
	bzero(buffer, sizeof(buffer));
	read(sockfd, buffer, sizeof(buffer));

	struct message msg;
	sscanf(buffer, "%d:%d:%s:%s", &msg.type, &msg.size, &msg.source, &msg.data);
	return msg;
}

int findSess(char query[MAX_NAME], char list[MAX_SESS][MAX_NAME], int num) {
	for (int i = 0; i < num; ++i) {
		if (strcmp(query, list[i]) == 0) {
			return i;
		}
	}
	return -1;
}
