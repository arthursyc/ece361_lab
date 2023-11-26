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

// void packeting(struct message * msg, char* packet){
// 	char* flag = ":";
// 	bool doppel = 0;
// 	if((strstr(msg->source,":") != NULL)|(strstr(msg->data,":") != NULL)){
// 		doppel = 1;
// 	}
// 	if(doppel) strcat(flag, flag);
// 	char* typeb = malloc(sizeof(unsigned int));
// 	char* sizeb = malloc(sizeof(unsigned int));
// 	sprintf(typeb, "%d", msg->type);
// 	sprintf(sizeb, "%d", msg->size);
// 	strcat(packet, flag);
// 	strcat(packet, typeb);
// 	strcat(packet, flag);
// 	strcat(packet, sizeb);
// 	strcat(packet, flag);
// 	strcat(packet, msg->source);
// 	strcat(packet, flag);
// 	strcat(packet, msg->data);
// 	strcat(packet, flag);
// 	free(typeb);
// 	free(sizeb);
// }

// void unbox(char* buffer, struct message* msg){
// 	// should probably call a packet failure error here
// 	if(strlen(buffer) < 5) return;

// 	char* flag = "::";
// 	if(strstr(buffer, flag) == NULL){ // singular flag, no occurences of naive flag
// 		flag = ":";
// 	}
// 	unsigned int window = 0;
// 	int field = 0;
// 	for(int i = 0; i < strlen(buffer);i++){
// 		if(buffer[i] == ':') continue;
// 		window = getwindow(buffer ,i);
// 		char* intermed = malloc(MAX_DATA*sizeof(char));
// 		strncpy(intermed, buffer+i, window);
// 		switch(field){
// 			case 0:
// 				msg->type = atoi(intermed);
// 			case 1:
// 				msg->size = atoi(intermed);
// 			case 2:
// 				strcpy(msg->source, intermed);
// 			case 3:
// 				strcpy(msg->data, intermed);
// 		}
// 		field ++;
// 		i += window - 1;
// 		free(intermed);
// 	}
// }

// int getwindow(char* buffer, int start){
// 	int i = start;
// 	while(buffer[i] != ':'){
// 		i++;
// 	}
// 	return i - start + 1;
// }
