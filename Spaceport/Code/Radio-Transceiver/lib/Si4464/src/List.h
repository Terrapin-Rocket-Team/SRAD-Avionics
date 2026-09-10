#ifndef LIST_H
#define LIST_H

#include "Node.h"

class List
{
public:
    Node *head = nullptr;
    Node *tail = nullptr;

    List() {}
    List(Node *head);
    ~List();

    void shift();
    bool shift(uint8_t *data, uint16_t *size, uint16_t maxSize);

    bool append(Node *n);
    bool append(uint8_t *data, uint16_t size);

    bool hasData();
};

#endif