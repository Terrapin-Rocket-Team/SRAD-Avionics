#include <RF4463.h>
#include <SPI.h>
// Generated with wireless development suite by silicon labs
#include "radio_config_Si4463.h"

// Configuration parameters from "radio_config_Si4463.h"
const uint8_t RF4463_CONFIGURATION_DATA[] = RADIO_CONFIGURATION_DATA_ARRAY;

RF4463::RF4463(uint8_t nIRQPin, uint8_t sdnPin, uint8_t nSELPin)
{
	_nIRQPin = nIRQPin;
	_sdnPin = sdnPin;
	_nSELPin = nSELPin;
}
void RF4463::spiInit()
{
	SPI.begin();
	// init slave select pin
	pinMode(_nSELPin, OUTPUT);
	digitalWrite(_nSELPin, HIGH);

	// depends on RF4463 spi timing
	SPI.setBitOrder(MSBFIRST);
	// too fast may cause error
	SPI.setClockDivider(SPI_CLOCK_DIV16);
	SPI.setDataMode(SPI_MODE0);
}
void RF4463::pinInit()
{
	pinMode(_sdnPin, OUTPUT);
	digitalWrite(_sdnPin, HIGH);
	pinMode(_nIRQPin, INPUT);
}
bool RF4463::init()
{
	Serial.println("1");
	Serial.flush();
	pinInit();
	spiInit();

	Serial.println("2");
	Serial.flush();

	uint8_t buf[20];

	// reset RF4463
	powerOnReset();
	// Set RF parameter,like frequency,data rate etc
	setConfig(RF4463_CONFIGURATION_DATA, sizeof(RF4463_CONFIGURATION_DATA));

	Serial.println("3");
	Serial.flush();

	// set antenna switch,in RF4463 is GPIO2 and GPIO3
	// don't change setting of GPIO2,GPIO3,NIRQ,SDO
	buf[0] = RF4463_GPIO_NO_CHANGE;
	buf[1] = RF4463_GPIO_NO_CHANGE;
	buf[2] = RF4463_GPIO_RX_STATE;
	buf[3] = RF4463_GPIO_TX_STATE;
	buf[4] = RF4463_NIRQ_INTERRUPT_SIGNAL;
	buf[5] = RF4463_GPIO_SPI_DATA_OUT;
	setCommand(6, RF4463_CMD_GPIO_PIN_CFG, buf);

	Serial.println("4");
	Serial.flush();

	// frequency adjust
	// frequency will inaccurate if change this parameter
	buf[0] = 98;
	setProperties(RF4463_PROPERTY_GLOBAL_XO_TUNE, 1, buf);

	Serial.println("5");
	Serial.flush();

	// tx = rx = 64 byte,PH mode ,high performance mode
	buf[0] = 0x40;
	setProperties(RF4463_PROPERTY_GLOBAL_CONFIG, 1, buf);

	Serial.println("6");
	Serial.flush();

	// set preamble
	buf[0] = 0x08; //  8 bytes Preamble
	buf[1] = 0x14; //  detect 20 bits
	buf[2] = 0x00;
	buf[3] = 0x0f;
	buf[4] = RF4463_PREAMBLE_FIRST_1 | RF4463_PREAMBLE_LENGTH_BYTES | RF4463_PREAMBLE_STANDARD_1010;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = 0x00;
	buf[8] = 0x00;
	setProperties(RF4463_PROPERTY_PREAMBLE_TX_LENGTH, 9, buf);

	Serial.println("7");
	Serial.flush();

	// set sync words
	buf[0] = 0x2d;
	buf[1] = 0xd4;
	setSyncWords(buf, 2);

	Serial.println("8");
	Serial.flush();

	// set CRC
	buf[0] = RF4463_CRC_SEED_ALL_1S | RF4463_CRC_ITU_T;
	setProperties(RF4463_PROPERTY_PKT_CRC_CONFIG, 1, buf);

	Serial.println("9");
	Serial.flush();

	buf[0] = RF4463_CRC_ENDIAN;
	setProperties(RF4463_PROPERTY_PKT_CONFIG1, 1, buf);

	Serial.println("10");
	Serial.flush();

	buf[0] = RF4463_IN_FIFO | RF4463_DST_FIELD_ENUM_2;
	buf[1] = RF4463_SRC_FIELD_ENUM_1;
	buf[2] = 0x00;
	setProperties(RF4463_PROPERTY_PKT_LEN, 3, buf);

	// set length of Field 1 -- 4
	// variable len,field as length field,field 2 as data field
	// didn't use field 3 -- 4
	buf[0] = 0x00;
	buf[1] = 0x01;
	buf[2] = RF4463_FIELD_CONFIG_PN_START;
	buf[3] = RF4463_FIELD_CONFIG_CRC_START | RF4463_FIELD_CONFIG_SEND_CRC | RF4463_FIELD_CONFIG_CHECK_CRC | RF4463_FIELD_CONFIG_CRC_ENABLE;
	buf[4] = 0x00;
	buf[5] = 50;
	buf[6] = RF4463_FIELD_CONFIG_PN_START;
	buf[7] = RF4463_FIELD_CONFIG_CRC_START | RF4463_FIELD_CONFIG_SEND_CRC | RF4463_FIELD_CONFIG_CHECK_CRC | RF4463_FIELD_CONFIG_CRC_ENABLE;
	;
	buf[8] = 0x00;
	buf[9] = 0x00;
	buf[10] = 0x00;
	buf[11] = 0x00;
	setProperties(RF4463_PROPERTY_PKT_FIELD_1_LENGTH_12_8, 12, buf);

	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = 0x00;
	setProperties(RF4463_PROPERTY_PKT_FIELD_4_LENGTH_12_8, 8, buf);

	// set max tx power
	setTxPower(127);

	// check if RF4463 works
	if (!checkDevice())
	{
		return false;
	}

	return true;
}
void RF4463::powerOnReset()
{
	uint8_t buf[] = {RF_POWER_UP};

	digitalWrite(_sdnPin, HIGH);
	delay(100);
	digitalWrite(_sdnPin, LOW);
	delay(20); // wait for RF4463 stable

	// send power up command
	digitalWrite(_nSELPin, LOW);
	spiWriteBuf(sizeof(buf), buf);
	digitalWrite(_nSELPin, HIGH);

	delay(200);
}
void RF4463::setConfig(const uint8_t *parameters, uint16_t paraLen)
{
	// command buf starts with length of command in RADIO_CONFIGURATION_DATA_ARRAY
	uint8_t cmdLen;
	uint8_t command;
	uint16_t pos;
	uint8_t buf[30];

	// power up command had already send
	paraLen = paraLen - 1;
	cmdLen = parameters[0];
	pos = cmdLen + 1;

	while (pos < paraLen)
	{
		cmdLen = parameters[pos++] - 1;		   // get command lend
		command = parameters[pos++];		   // get command
		memcpy(buf, parameters + pos, cmdLen); // get parameters

		setCommand(cmdLen, command, buf);
		pos = pos + cmdLen;
	}
}
bool RF4463::checkDevice()
{
	uint8_t buf[9];
	uint16_t partInfo;
	if (!getCommand(9, RF4463_CMD_PART_INFO, buf)) // read part info to check if 4463 works
		return false;

	partInfo = buf[2] << 8 | buf[3];
	if (partInfo != 0x4463)
	{
		return false;
	}
}
bool RF4463::txPacket(uint8_t *sendbuf, uint8_t sendLen)
{
	uint16_t txTimer;

	fifoReset();				   // clr fifo
	writeTxFifo(sendbuf, sendLen); // load data to fifo
	setTxInterrupt();
	clrInterrupts(); // clr int factor
	enterTxMode();	 // enter TX mode

	txTimer = RF4463_TX_TIMEOUT;
	while (txTimer--)
	{
		if (waitnIRQ()) // wait INT
		{
			return true;
		}
		delay(1);
	}
	init(); // reset RF4463 if tx time out

	return false;
}
uint8_t RF4463::rxPacket(uint8_t *recvbuf)
{
	uint8_t rxLen;
	rxLen = ReadRxFifo(recvbuf); // read data from fifo
	fifoReset();				 // clr fifo

	return rxLen;
}

bool RF4463::rxInit()
{
	uint8_t length;
	length = 50;
	setProperties(RF4463_PROPERTY_PKT_FIELD_2_LENGTH_7_0, sizeof(length), &length); // reload rx fifo size
	fifoReset();																	// clr fifo
	setRxInterrupt();
	clrInterrupts(); // clr int factor
	enterRxMode();	 // enter RX mode
	return true;
}
bool RF4463::waitnIRQ()
{
	return !digitalRead(_nIRQPin); // inquire interrupt
}
void RF4463::enterTxMode()
{
	uint8_t buf[] = {0x00, 0x30, 0x00, 0x00};
	buf[0] = RF4463_FREQ_CHANNEL;
	setCommand(4, RF4463_CMD_START_TX, buf);
}
void RF4463::enterRxMode()
{
	uint8_t buf[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08};
	buf[0] = RF4463_FREQ_CHANNEL;
	setCommand(7, RF4463_CMD_START_RX, buf);
}
bool RF4463::enterStandbyMode()
{
	uint8_t data = 0x01;
	return setCommand(1, RF4463_CMD_CHANGE_STATE, &data);
}
bool RF4463::setTxInterrupt()
{
	uint8_t buf[3] = {0x01, 0x20, 0x00}; // enable PACKET_SENT interrupt
	return setProperties(RF4463_PROPERTY_INT_CTL_ENABLE, 3, buf);
}
bool RF4463::setRxInterrupt()
{
	uint8_t buf[3] = {0x03, 0x18, 0x00}; // enable PACKET_RX interrupt
	return setProperties(RF4463_PROPERTY_INT_CTL_ENABLE, 3, buf);
}
bool RF4463::clrInterrupts()
{
	uint8_t buf[] = {0x00, 0x00, 0x00};
	return setCommand(sizeof(buf), RF4463_CMD_GET_INT_STATUS, buf);
}
void RF4463::writeTxFifo(uint8_t *databuf, uint8_t length)
{
	setProperties(RF4463_PROPERTY_PKT_FIELD_2_LENGTH_7_0, sizeof(length), &length);
	uint8_t buf[length + 1];
	buf[0] = length;
	memcpy(buf + 1, databuf, length);
	setCommand(length + 1, RF4463_CMD_TX_FIFO_WRITE, buf);
}
uint8_t RF4463::ReadRxFifo(uint8_t *databuf)
{
	if (!checkCTS())
		return 0;
	uint8_t readLen;
	digitalWrite(_nSELPin, LOW);
	spiByte(RF4463_CMD_RX_FIFO_READ);
	spiReadBuf(1, &readLen);
	spiReadBuf(readLen, databuf);
	digitalWrite(_nSELPin, HIGH);
	return readLen;
}
void RF4463::fifoReset()
{
	uint8_t data = 0x03;
	setCommand(sizeof(data), RF4463_CMD_FIFO_INFO, &data);
}
bool RF4463::setGPIOMode(uint8_t GPIO0Mode, uint8_t GPIO1Mode)
{
	uint8_t buf[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	buf[0] = GPIO0Mode;
	buf[1] = GPIO1Mode;

	return setCommand(sizeof(buf), RF4463_CMD_GPIO_PIN_CFG, buf);
}
bool RF4463::setPreambleLen(uint8_t len)
{
	return setProperties(RF4463_PROPERTY_PREAMBLE_TX_LENGTH, 1, &len);
}
bool RF4463::setSyncWords(uint8_t *syncWords, uint8_t len)
{
	if ((len == 0) || (len > 3))
		return false;
	uint8_t buf[5];
	buf[0] = len - 1;
	memcpy(buf + 1, syncWords, len);
	return setProperties(RF4463_PROPERTY_SYNC_CONFIG, sizeof(buf), buf);
}
bool RF4463::setTxPower(uint8_t power)
{
	if (power > 127) // max is 127
		return false;

	uint8_t buf[4] = {0x08, 0x00, 0x00, 0x3d};
	buf[1] = power;

	return setProperties(RF4463_PROPERTY_PA_MODE, sizeof(buf), buf);
}
bool RF4463::setCommand(uint8_t length, uint8_t command, uint8_t *paraBuf)
{
	if (!checkCTS())
		return false;

	digitalWrite(_nSELPin, LOW);
	spiByte(command);			  // send command
	spiWriteBuf(length, paraBuf); // send parameters
	digitalWrite(_nSELPin, HIGH);

	return true;
}
bool RF4463::getCommand(uint8_t length, uint8_t command, uint8_t *paraBuf)
{
	if (!checkCTS())
		return false;

	digitalWrite(_nSELPin, LOW);
	spiByte(command); // set command to read
	digitalWrite(_nSELPin, HIGH);

	if (!checkCTS()) // check if RF4463 is ready
		return false;

	digitalWrite(_nSELPin, LOW);
	spiByte(RF4463_CMD_READ_BUF); // turn to read command mode
	spiReadBuf(length, paraBuf);  // read parameters
	digitalWrite(_nSELPin, HIGH);
}

bool RF4463::setProperties(uint16_t startProperty, uint8_t length, uint8_t *paraBuf)
{
	uint8_t buf[4];

	if (!checkCTS())
		return false;

	buf[0] = RF4463_CMD_SET_PROPERTY;
	buf[1] = startProperty >> 8;   // GROUP
	buf[2] = length;			   // NUM_PROPS
	buf[3] = startProperty & 0xff; // START_PROP

	digitalWrite(_nSELPin, LOW);
	spiWriteBuf(4, buf);		  // set start property and read length
	spiWriteBuf(length, paraBuf); // set parameters
	digitalWrite(_nSELPin, HIGH);

	return true;
}
bool RF4463::getProperties(uint16_t startProperty, uint8_t length, uint8_t *paraBuf)
{
	if (!checkCTS())
		return false;

	uint8_t buf[4];
	buf[0] = RF4463_CMD_GET_PROPERTY;
	buf[1] = startProperty >> 8;   // GROUP
	buf[2] = length;			   // NUM_PROPS
	buf[3] = startProperty & 0xff; // START_PROP

	digitalWrite(_nSELPin, LOW);
	spiWriteBuf(4, buf); // set start property and read length
	digitalWrite(_nSELPin, HIGH);

	if (!checkCTS())
		return false;

	digitalWrite(_nSELPin, LOW);
	spiByte(RF4463_CMD_READ_BUF); // turn to read command mode
	spiReadBuf(length, paraBuf);  // read parameters
	digitalWrite(_nSELPin, HIGH);
	return true;
}
bool RF4463::checkCTS()
{
	uint16_t timeOutCnt;
	timeOutCnt = RF4463_CTS_TIMEOUT;
	while (timeOutCnt--) // cts counter
	{
		digitalWrite(_nSELPin, LOW);
		spiByte(RF4463_CMD_READ_BUF);		// send READ_CMD_BUFF command
		if (spiByte(0) == RF4463_CTS_REPLY) // read CTS
		{
			digitalWrite(_nSELPin, HIGH);
			return true;
		}
		digitalWrite(_nSELPin, HIGH);
	}
	return false;
}
void RF4463::spiWriteBuf(uint8_t writeLen, uint8_t *writeBuf)
{
	while (writeLen--)
		spiByte(*writeBuf++);
}
void RF4463::spiReadBuf(uint8_t readLen, uint8_t *readBuf)
{
	while (readLen--)
		*readBuf++ = spiByte(0);
}
uint8_t RF4463::spiByte(uint8_t writeData)
{
	uint8_t readData;
	readData = SPI.transfer(writeData);
	return readData;
}
