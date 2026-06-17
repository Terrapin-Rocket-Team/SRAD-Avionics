#ifndef RADIO_H
#define RADIO_H

#include "RadioMessage.h"
#include "RadioDriver.h"
#include "List.h"

class Radio
{
public:
    RadioDriver *rad;
    List txMsgs;
    List rxMsgs;

    bool isActive = false;
    uint32_t rxTimeout = 100; // ms

    Radio(RadioDriver *rad);
    Radio(RadioDriver *rad, bool isActive, uint32_t rxTimeout = 0);
    ~Radio();

    void setRole(bool isActive, uint32_t rxTimeout = 0);
    /*
    Send a message using the data in ```data```
    - data : the ```Data``` object containing the message to be sent
    Returns: whether the transmission was successfully started
    */
    bool send(Data *data);
    bool send(Message *msg);
    /*
    Receive a message and place it in ```data```, should only be called after avail() returns true
    - data : the ```Data``` object to place the message data in
    Returns: whether a message was successfully received
    */
    bool receive(Data *data);
    bool receive(Message *msg);

    void update();

private:
    uint32_t rxTimer = millis() - 100;
};

#endif