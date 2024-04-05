// rf69_server.cpp
//
// Example program showing how to use RH_RF69 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM69 module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf69
// make
// sudo ./rf69_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon

// NOTE: must run with sudo or segmentation fault happens

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "RFM69HCW.h"

// define hardware used change to fit your need
// Uncomment the board you have, if not listed
// uncommment custom board and set wiring tin custom section

// LoRasPi board
// see https://github.com/hallard/LoRasPI
// #define BOARD_LORASPI

// iC880A and LinkLab Lora Gateway Shield (if RF module plugged into)
// see https://github.com/ch2i/iC880A-Raspberry-PI
// #define BOARD_IC880A_PLATE

// Raspberri PI Lora Gateway for multiple modules
// see https://github.com/hallard/RPI-Lora-Gateway
// #define BOARD_PI_LORA_GATEWAY

// Dragino Raspberry PI hat
// see https://github.com/dragino/Lora
// #define BOARD_DRAGINO_PIHAT

// Now we include RasPi_Boards.h so this will expose defined
// constants with CS/IRQ/RESET/on board LED pins definition
// #include "../RasPiBoards.h"

uint8_t cs = 27;
uint8_t irq = 17;
uint8_t rst = 22;

APRSConfig config = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
RadioSettings settings = {433.775, true, false, &hardware_spi, cs, irq, rst};
RFM69HCW transmit = {&settings, &config};

// Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
    printf("\n%s Break received, exiting!\n", __BASEFILE__);
    force_exit = true;
}

// Main Function
int main(int argc, const char *argv[])
{

    signal(SIGINT, sig_handler);
    printf("%s\n", __BASEFILE__);

    if (!bcm2835_init())
    {
        fprintf(stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__);
        return 1;
    }

    printf("RF69 CS=GPIO%d", cs);
    printf(", IRQ=GPIO%d\n", irq);
    // IRQ Pin input/pull down
    // pinMode(irq, INPUT);
    // bcm2835_gpio_set_pud(irq, BCM2835_GPIO_PUD_DOWN);
    // Now we can enable Rising edge detection
    // bcm2835_gpio_ren(irq);

    if (!transmit.begin())
        fprintf(stderr, "Transmitter failed to begin\n");

    // Begin the main body of code
    fprintf(stdout, "Starting transmission\n");
    fflush(stdout);
    // FILE *video = fopen("test3.av1", "rb");
    char data[RH_RF69_MAX_MESSAGE_LEN] = {0};
    bool transmission = false;
    int count = 0;
    while (!force_exit)
    {
        if (!transmission)
        {
            // fprintf(stdout, "transmitting\n");
            count += fread(data + count, sizeof(char), RH_RF69_MAX_MESSAGE_LEN, stdin) - 1;
            if (count + 1 == RH_RF69_MAX_MESSAGE_LEN)
            {
                count = 0;
                fprintf(stdout, "transmitting\n");
                if (transmit.send(data, ENCT_NONE, RH_RF69_MAX_MESSAGE_LEN))
                    transmission = true;
                else
                    fprintf(stdout, "failed\n");
            }
            fflush(stdout);
        }
        else
        {
            transmission = !transmit.sendBuffer();
            if (!transmission)
            {
                fprintf(stdout, "transmitting done!\n");
                transmit.endtx();
            }
        }
        // fprintf(stdout, transmission ? "y\n" : "n\n");
        fflush(stdout);

        // Let OS doing other tasks
        // For timed critical appliation you can reduce or delete
        // this delay, but this will charge CPU usage, take care and monitor
        bcm2835_delay(0);
    }
    // fclose(video);
    printf("\n%s Ending\n", __BASEFILE__);
    bcm2835_close();
    return 0;
}