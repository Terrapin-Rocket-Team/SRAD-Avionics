#include <stdio.h>
#include <exception>
#include <wiringPi.h>
#include <atomic>
#include <thread>

// libcamera
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "lv_output.hpp"

using namespace std::placeholders;

#define CMD_PIN 14  // GPIO pin for input from the FC
#define RESP_PIN 15 // GPIO pin for acknowledgment to the FC

std::atomic<bool> recording(false);
std::atomic<bool> running(true);

// Function to start recording
void startRecording(LibcameraEncoder &app) {
    if (!recording.load()) {
        printf("Starting recording...\n");

        // Configure and start the camera
        app.OpenCamera();
        app.ConfigureVideo(LibcameraEncoder::FLAG_VIDEO_JPEG_COLOURSPACE);
        app.StartEncoder();
        app.StartCamera();

        recording.store(true);
        digitalWrite(RESP_PIN, HIGH); // Acknowledge recording started
    }
}

// Function to stop recording
void stopRecording(LibcameraEncoder &app) {
    if (recording.load()) {
        printf("Stopping recording...\n");

        // Stop and close the camera
        app.StopCamera();
        app.StopEncoder();
        app.CloseCamera();

        recording.store(false);
        digitalWrite(RESP_PIN, LOW); // Acknowledge recording stopped
    }
}

// GPIO monitoring loop
void gpioLoop(LibcameraEncoder &app) {
    while (running.load()) {
        if (digitalRead(CMD_PIN) == HIGH) {
            startRecording(app);
        } else {
            stopRecording(app);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Debounce and CPU usage control
    }
}

int main(int argc, char *argv[]) {
    try {
        // Initialize GPIO
        if (wiringPiSetupGpio() == -1) {
            printf("Failed to initialize WiringPi.\n");
            return -1;
        }

        pinMode(CMD_PIN, INPUT);
        pinMode(RESP_PIN, OUTPUT);
        pullUpDnControl(CMD_PIN, PUD_DOWN);
        digitalWrite(RESP_PIN, LOW);

        LibcameraEncoder app;
        VideoOptions *options = app.GetOptions();

        if (options->Parse(argc, argv)) {
            // Start GPIO monitoring loop in a separate thread
            std::thread gpioThread(gpioLoop, std::ref(app));

            // Main camera event loop
            eventLoop(app);

            // Cleanup
            running.store(false);
            gpioThread.join();
        }
    } catch (std::exception const &e) {
        printf("Error: %s\n", e.what());
        return -1;
    }

    // Ensure GPIO pins are reset
    digitalWrite(RESP_PIN, LOW);
    pinMode(RESP_PIN, INPUT);
    pinMode(CMD_PIN, INPUT);

    return 0;
}
