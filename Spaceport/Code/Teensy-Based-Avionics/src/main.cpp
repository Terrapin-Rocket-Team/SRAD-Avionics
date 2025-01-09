#include <wiringPi.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <atomic>

#define CMD_PIN 14  // GPIO pin number (BCM numbering)
#define RESP_PIN 15 // GPIO pin number (BCM numbering)

std::atomic<bool> recording(false);
std::atomic<bool> running(true);

// Function to start recording
void startRecording() {
    if (!recording.load()) {
        std::cout << "Starting recording..." << std::endl;
        int ret = std::system("libcamera-vid -o video.mp4 -t 0 -n --width 1080 --height 720 --hdr auto --framerate 15 &");
        if (ret == 0) {
            recording.store(true);
            digitalWrite(RESP_PIN, HIGH); // Acknowledge recording started
        } else {
            std::cerr << "Failed to start recording." << std::endl;
        }
    }
}

// Function to stop recording
void stopRecording() {
    if (recording.load()) {
        std::cout << "Stopping recording..." << std::endl;
        int ret = std::system("pkill -SIGTERM libcamera-vid");
        if (ret == 0) {
            recording.store(false);
            digitalWrite(RESP_PIN, LOW); // Acknowledge recording stopped
        } else {
            std::cerr << "Failed to stop recording." << std::endl;
        }
    }
}

// Signal handler for clean exit
void signalHandler(int signum) {
    std::cout << "Exiting..." << std::endl;
    running.store(false);
    stopRecording();
    digitalWrite(RESP_PIN, LOW);
    pinMode(RESP_PIN, INPUT);
    pinMode(CMD_PIN, INPUT);
    exit(signum);
}

int main() {
    // WiringPi setup
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Failed to initialize WiringPi." << std::endl;
        return 1;
    }

    pinMode(CMD_PIN, INPUT);
    pinMode(RESP_PIN, OUTPUT);
    pullUpDnControl(CMD_PIN, PUD_DOWN);

    // Register signal handler
    signal(SIGINT, signalHandler);

    // Start recording automatically on boot
    startRecording();

    while (running.load()) {
        if (digitalRead(CMD_PIN) == LOW) {
            stopRecording();
        } else {
            startRecording();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Debounce and CPU usage control
    }

    return 0;
}
