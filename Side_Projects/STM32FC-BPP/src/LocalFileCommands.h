#pragma once

#include <Arduino.h>

namespace astra
{
class SerialMessageRouter;
}

namespace stm32fc
{

void registerLocalFileCommands(astra::SerialMessageRouter &router);
void printLocalFileCommandHelp(Print &out);

} // namespace stm32fc
