#include "List.h"

List::List(Node *n)
{
    this->head = n;
    this->tail = n;
}

List::~List()
{
}

void List::shift()
{
    if (this->head != nullptr)
    {
        Node *oldHead = this->head;
        this->head = this->head->next;
        if (this->head == nullptr)
            this->tail = nullptr;
        delete oldHead;
    }
}

bool List::shift(uint8_t *data, uint16_t *size, uint16_t maxSize)
{
    // Serial.println("List.cpp");
    // Serial.println(this->head == nullptr);
    // Serial.flush();
    if (this->head != nullptr && this->head->size < maxSize)
    {
        // Serial.println("List.cpp");
        // Serial.flush();
        memcpy(data, this->head->data, this->head->size);
        *size = this->head->size;
        Node *oldHead = this->head;
        this->head = this->head->next;
        if (this->head == nullptr)
            this->tail = nullptr;
        delete oldHead;

        return true;
    }

    // Serial.println("List.cpp 2");
    // Serial.println(this->head == nullptr);
    // Serial.flush();
    return false;
}

bool List::append(Node *n)
{
    // Serial1.println("List.cpp");
    // Serial1.println(this->head == nullptr);
    // Serial1.println(this->tail == nullptr);
    if (this->head == nullptr && this->tail == nullptr)
    {
        this->head = n;
        this->tail = n;
    }
    else
    {
        this->tail->next = n;
        this->tail = n;
    }

    return true;
}

bool List::append(uint8_t *data, uint16_t size)
{
    Node *n = new Node(data, size);
    if (n->errFull)
        return false;
    // else
    this->append(n);
    return true;
}

bool List::hasData()
{
    return this->head != nullptr;
}