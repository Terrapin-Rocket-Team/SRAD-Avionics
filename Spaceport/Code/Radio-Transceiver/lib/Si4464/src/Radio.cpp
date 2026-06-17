#include "Radio.h"

Radio::Radio(RadioDriver *rad) : rad(rad)
{
}

Radio::Radio(RadioDriver *rad, bool isActive, uint32_t rxTimeout) : rad(rad), isActive(isActive), rxTimeout(rxTimeout)
{
    this->rxTimer = millis() - rxTimeout;
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

    // make minimum message size the minimum size to trigger RX threshold (si4464)
    while (msg->size < 40)
        msg->append(0);
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
    this->rad->update();

    // if this is an active node (primarily sending)
    if (isActive)
    {
        // Serial.println("Radio.cpp before rx");
        // Serial.flush();
        // Serial.println(this->rad->state);
        // delay(100);
        // check if rxTimeout is not expired and there is a message received
        if (millis() - this->rxTimer <= rxTimeout && this->rad->avail())
        {
            // Serial1.println("Radio.cpp rx");
            // Serial1.println(this->rad->avail());
            // Serial1.flush();

            // store the message
            Node *n = new Node(this->rad->avail());
            bool resultRX = this->rad->rx(n->data, &(n->size), n->size); // size shouldn't change here
            // Serial.write(n->data, n->size);
            bool resultAppend = false;
            if (resultRX)
            {
                uint8_t newChannel = 0;
                bool hasQSY = false;
                // check if message is QSY
                if (n->size >= 6)
                {
                    char cmd[4] = {0, 0, 0, 0};
                    uint16_t len = 3;
                    memcpy(cmd, n->data, len);
                    // mRX.get((uint8_t *)cmd, len);
                    // Serial1.println(cmd);
                    // Serial1.println(len);
                    // check if this command is telling us to change frequency
                    if (len == 3 && strcmp(cmd, "QSY") == 0)
                    {
                        // convert channel number string to number
                        char newChannelStr[3] = {0, 0, 0};
                        len = 2;
                        memcpy(newChannelStr, n->data + 4, len);
                        // mRX.get((uint8_t *)newChannelStr, len, 4);
                        if (len == 2)
                        {
                            // Serial1.println();
                            newChannel = atoi(newChannelStr);
                            hasQSY = true;
                        }
                    }
                }
                // change frequency
                if (hasQSY)
                {
                    this->rad->channel = newChannel;
                    // Serial1.print("New Channel Received RX: ");
                    // Serial1.println(newChannel);
                }
                else
                {
                    // otherwise add message
                    resultAppend = rxMsgs.append(n);
                }
            }

            bool success = resultRX && resultAppend;

            // set rxTimer to allow for tx on the next loop
            this->rxTimer = millis() - this->rxTimeout;
            // delay(1000);
        }

        // Serial.println("Radio.cpp");
        // Serial.println(this->txMsgs.hasData());
        // Serial.println(this->txMsgs.head == nullptr);
        // Serial.flush();
        // check if there is a message to send and the rxTimeout is expired
        if (this->txMsgs.hasData() && millis() - this->rxTimer > this->rxTimeout)
        {
            // Serial1.println("Radio.cpp sending msg");
            // Serial1.write(this->txMsgs.head->data, this->txMsgs.head->size);
            // Serial1.println();
            // Serial1.flush();

            // send the message
            // Serial.println(this->rad->avail());
            this->rad->tx(this->txMsgs.head->data, this->txMsgs.head->size);
            // Serial1.println("Radio.cpp here");
            // Serial1.flush();
            this->txMsgs.shift();
            // delay(10);
            // Serial.println(this->rad->avail());
            // reset rxTimeout
            this->rxTimer = millis();
        }

        // Serial.println("Radio.cpp after rx");
        // Serial.flush();
    }
    // if this is a passive node (primarily receiving)
    else
    {
        // check if there is a message received
        if (millis() - this->rxTimer > rxTimeout && this->rad->avail() > 0)
        {
            // store the message
            Node *n = new Node(this->rad->avail());
            bool resultRX = this->rad->rx(n->data, &(n->size), n->size); // size shouldn't change here
            // Serial.println("Radio.cpp rx inactive");
            // Serial.println(resultRX);
            // Serial.flush();
            bool resultAppend = false;
            if (resultRX)
            {
                resultAppend = rxMsgs.append(n);
            }

            bool success = resultRX && resultAppend;
            // Serial.println(success);
            // Serial1.println(this->txMsgs.hasData());
            // Serial1.flush();

            this->rxTimer = millis();
        }
        else if (millis() - this->rxTimer > rxTimeout && this->rad->state != STATE_RX)
        {
            this->rad->startRx();
        }

        // Serial1.println(this->rad->state);
        // delay(10);
        // check if there is a message to send
        if (millis() - this->rxTimer > 50 && millis() - this->rxTimer <= rxTimeout && this->txMsgs.hasData())
        {
            // check if message is QSY
            uint8_t newChannel = 0;
            bool hasQSY = false;
            // Serial1.println("Radio.cpp size");
            // Serial1.println(this->txMsgs.head->size);
            if (this->txMsgs.head->size >= 6)
            {
                char cmd[4] = {0, 0, 0, 0};
                uint16_t len = 3;
                memcpy(cmd, this->txMsgs.head->data, len);
                // mTX.get((uint8_t *)cmd, len);
                // Serial1.println(cmd);
                // Serial1.println(len);
                // check if this command is telling us to change frequency
                if (len == 3 && strcmp(cmd, "QSY") == 0)
                {
                    // convert channel number string to number
                    char newChannelStr[3] = {0, 0, 0};
                    len = 2;
                    memcpy(newChannelStr, this->txMsgs.head->data + 4, len);
                    // mTX.get((uint8_t *)newChannelStr, len, 4);
                    if (len == 2)
                    {
                        // Serial1.println("Radio.cpp here");
                        // Serial1.println(newChannelStr);
                        newChannel = atoi(newChannelStr);
                        hasQSY = true;
                    }
                }
            }
            // Serial.println(this->rad->state);
            // send the message
            this->rad->tx(this->txMsgs.head->data, this->txMsgs.head->size);
            this->txMsgs.shift();
            // change the channel
            if (hasQSY)
            {
                this->rad->channel = newChannel;
                this->rad->state = STATE_IDLE; // NOTE: THIS IS SUPER SCUFFED, but set the state to idle to force re-entering rx mode and switch to the new channel
                delay(50);                     // also wait a bit to let the message finish sending
                // Serial1.print("New Channel Received TX: ");
                // Serial1.println(newChannel);
            }
            // Serial.println("Radio.cpp inactive sending msg");

            this->rxTimer = millis() - this->rxTimeout;
        }
    }
}