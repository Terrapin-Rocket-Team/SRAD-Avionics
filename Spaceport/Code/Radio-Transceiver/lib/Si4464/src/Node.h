#ifndef NODE_H
#define NODE_H

#include "Arduino.h"

#define LIST_MAX_SIZE 10240

class Node
{
public:
    static uint32_t maxSize;
    static uint32_t totalSize;
    uint8_t *data = nullptr;
    uint16_t size = 0;
    bool errFull = false;

    Node *next = nullptr;

    Node(uint16_t size);
    Node(uint8_t *data, uint16_t size);
    ~Node();
};

#endif