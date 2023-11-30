#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdbool.h>

#define MAX_NAME	16
#define MAX_PWD		8
#define MAX_DATA	1024

/**** structs and enums ****/
struct message {
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
};

struct session {
	int usercount;
	char id[MAX_NAME];
	struct session* next;
}

struct client {
	char id[MAX_NAME];
	char pwd[MAX_PWD];
	int fd;
	bool online;
	struct session* sess;
};


enum type {
	LOGIN,
	LO_ACK,
	LO_NAK,
	EXIT,
	JOIN,
	JN_ACK,
	JN_NAK,
	LEAVE_SESS,
	NEW_SESS,
	NS_ACK,
	NS_NAK,
	MESSAGE,
	QUERY,
	QU_ACK,
	REGIS,
};

/**** functions ****/

/**
 * @brief Parse a string into an array of strings by tokenize using space
*/
char** parse(char* buffer, char delim[]);

/**
 * @brief Receive message from sockfd and put it into struct message form
*/
struct message getMessage(int sockfd);

struct session* findSess(char query[MAX_NAME], struct session* sess_head);

// /*
//  puts a message struct into a packet in the form a string with control flags
// */
// void packeting(struct message * msg, char* packet);
// /*
//  reconstruct the message received in the form of a message struct
// */
// void unbox(char* buffer, struct message* msg);
// /*
//  unbox helper
// */
// int getwindow(char* buffer, int start);

#endif
