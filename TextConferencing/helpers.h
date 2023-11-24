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

struct client {
	char id[MAX_NAME];
	char pwd[MAX_PWD];
	char ip[15];
	int port_addr;
	bool online;
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
	MESSAGE,
	QUERY,
	QU_ACK
};

/**** functions ****/

/**
 * @brief Parse a string into an array of strings by tokenize using space
*/
char** parse(char* buffer, char delim[]);
#endif
