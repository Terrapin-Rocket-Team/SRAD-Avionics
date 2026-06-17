#include "Node.h"

uint32_t Node::maxSize = LIST_MAX_SIZE;
uint32_t Node::totalSize = 0;

Node::Node(uint16_t size)
{
    if (Node::totalSize + size <= LIST_MAX_SIZE)
    {
        this->data = new uint8_t[size];
        // no data, so don't need to copy anything
        this->size = size;
        Node::totalSize += size;
    }
    else
    {
        this->errFull = true;
    }
}

Node::Node(uint8_t *data, uint16_t size)
{
    if (Node::totalSize + size <= LIST_MAX_SIZE)
    {
        // Serial1.println("Node.cpp");
        // Serial1.println(size);
        // Serial1.println(data == nullptr);
        this->data = new uint8_t[size];
        memcpy(this->data, data, size);
        this->size = size;
        Node::totalSize += size;
    }
    else
    {
        // Serial.println("ERR Node.cpp");
        this->errFull = true;
    }
}

Node::~Node()
{
    if (this->data != nullptr)
        delete[] this->data;
    Node::totalSize -= this->size;
}