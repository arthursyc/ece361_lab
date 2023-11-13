#ifndef _HELPERS_H_
#define _HELPERS_H_

#define MAX_NAME 16
#define MAX_DATA 1024

struct message {
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
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

#endif
