// xor_list.h

#ifndef XOR_LIST_H
#define XOR_LIST_H

#include <stdint.h>

typedef struct Xor_List_elm {
	int data;
	uint8_t xored;
} XList_elm;

typedef struct Xor_List {
	struct Xor_List_elm *head;
	struct Xor_List_elm *tail;
} XList;

void XList_append(XList *list, int data);
void XList_next(XList_elm curr, XList_elm next);

#endif  // XOR_LIST_H
