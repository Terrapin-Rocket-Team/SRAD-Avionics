#include "Radio.h"

Radio::Radio(RadioDriver *rad) : rad(rad)
{
}

Radio::Radio(RadioDriver *rad, bool isActive, uint32_t rxTimeout) : rad(rad), isActive(isActive), rxTimeout(rxTimeout)
{
}

Radio::~Radio()
{
}

void Radio::setRole(bool isActive, uint32_t rxTimeout)
{
    this->isActive = isActive;
    this->rxTimeout = rxTimeout;
}

bool Radio::send(Data *data)
{
    // encode the data
    Message m;
    m.encode(data);

    // add the data to be sent
    return this->send(&m);
}

bool Radio::send(Message *msg)
{
    return this->txMsgs.append(msg->buf, msg->size);
}

bool Radio::receive(Data *data)
{
    Message m;
    bool result = this->receive(&m);
    if (result)
        m.decode(data);
    // TODO: error check?

    return result;
}

bool Radio::receive(Message *msg)
{
    // check if we have a message and put it in msg
    return this->rxMsgs.shift(msg->buf, &(msg->size), Message::maxSize);
}

void Radio::update()
{
    rad->update();

    // if this is an active node (primarily sending)
    if (isActive)
    {
        // check if there is a message to send and the rxTimeout is expired
        if (this->txMsgs.hasData() && millis() - this->rxTimer > this->rxTimeout)
        {
            Serial1.println("sending msg");
            Serial1.write(this->txMsgs.head->data, this->txMsgs.head->size);
            Serial1.println();
            // reset rxTimeout
            this->rxTimer = millis();

            // send the message
            this->rad->tx(this->txMsgs.head->data, this->txMsgs.head->size);
            this->txMsgs.shift();
        }

        // check if rxTimeout is not expired and there is a message received
        if (millis() - this->rxTimer <= rxTimeout && this->rad->avail())
        {
            // store the message
            Node *n = new Node(this->rad->avail());
            bool resultRX = this->rad->rx(n->data, &(n->size), n->size); // size shouldn't change here
            bool resultAppend = false;
            if (resultRX)
            {
                resultAppend = rxMsgs.append(n);
            }

            bool success = resultRX && resultAppend;

            // set rxTimer to allow for tx on the next loop
            this->rxTimer = millis() - this->rxTimeout;
        }
    }
    // if this is a passive node (primarily receiving)
    else
    {
        // check if there is a message received
        if (this->rad->avail() > 0)
        {
            // store the message
            Node *n = new Node(this->rad->avail());
            bool resultRX = this->rad->rx(n->data, &(n->size), n->size); // size shouldn't change here
            bool resultAppend = false;
            if (resultRX)
            {
                resultAppend = rxMsgs.append(n);
            }

            bool success = resultRX && resultAppend;

            // check if there is a message to send
            if (this->txMsgs.hasData())
            {
                // send the message
                this->rad->tx(this->txMsgs.head->data, this->txMsgs.head->size);
                this->txMsgs.shift();
            }
        }
    }
}