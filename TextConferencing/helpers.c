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
	char** array = parse(buffer, "`");
	msg.type = atoi(array[0]);
	msg.size = atoi(array[1]);
	memset(msg.source, 0, sizeof(msg.source));
	memcpy(msg.source, array[2], sizeof(array[2]));
	msg.source[strcspn(msg.source, "\n")] = '\0';
	memset(msg.data, 0, sizeof(msg.data));
	memcpy(msg.data, array[3], msg.size);
	msg.data[msg.size] = '\0';
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


client_list * load_client_list(){
	FILE* fptr;
	fptr = fopen("clientlist.txt", "r");
	if(fptr == NULL){
		; // throw some missing file warning?
	}
	client_list * list = malloc(sizeof(client_list));
	list->length = 0;
	client_node * tail = malloc(sizeof(client_node));
	while(1){
		char buffer[MAX_LINE];
		if(fgets(buffer, MAX_LINE, fptr) == NULL){
			break;
		}
		buffer[strcspn(buffer, "\n")] = '\0';
		char** array = parse(buffer, "`");
		struct client * new_client = malloc(sizeof(struct client));
		memcpy(new_client->id, array[0], sizeof(array[0]));
		memcpy(new_client->pwd, array[1], sizeof(array[1]));
		// df, online, sess take default values, never written to the actual file
		new_client->fd = -1;
		new_client->online = false;
		new_client->sess = NULL;
		client_node * new_node = malloc(sizeof(client_node));
		new_node->client = new_client;
		new_node->next = NULL;
		if(list->head == NULL){
			list->head = new_node;
			tail = new_node;
			list->tail = new_node;
		}else{
			tail->next = new_node;
			list->tail = tail->next;
			tail = tail->next;
		}
		list->length++;
	}
	fclose(fptr);
	return list;
}

client_node * find_client(client_list * list, char id[MAX_NAME]){
	if((list == NULL)|(list->head == NULL)){
		return NULL;
	}
	client_node * curr = list->head;
	while(curr != NULL){
		if(strstr(curr->client->id, id) != NULL){
			return curr;
		}else{
			curr = curr->next;
		}
	}
	return NULL;
}
