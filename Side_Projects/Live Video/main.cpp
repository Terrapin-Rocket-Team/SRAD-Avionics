#include <stdio.h>
#include <exception>
#include <wiringPi.h>
#include <atomic>
#include <csignal>
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "lv_output.hpp"

using namespace std::placeholders;

// GPIO configuration
#define CMD_PIN 14  // GPIO pin for command input
#define RESP_PIN 15 // GPIO pin for response output

std::atomic<bool> recording(false);
std::atomic<bool> running(true);
LibcameraEncoder *appPtr = nullptr;

// Function to start recording
void startRecording(LibcameraEncoder &app) {
    if (!recording.load()) {
        printf("Starting recording...\n");
        app.StartCamera();
        recording.store(true);
        digitalWrite(RESP_PIN, HIGH); // Signal that recording started
    }
}

// Function to stop recording
void stopRecording(LibcameraEncoder &app) {
    if (recording.load()) {
        printf("Stopping recording...\n");
        app.StopCamera();
        recording.store(false);
        digitalWrite(RESP_PIN, LOW); // Signal that recording stopped
    }
}

// Signal handler for clean exit
void signalHandler(int signum) {
    printf("Exiting...\n");
    running.store(false);
    if (appPtr) {
        stopRecording(*appPtr);
    }
    digitalWrite(RESP_PIN, LOW);
    pinMode(RESP_PIN, INPUT);
    pinMode(CMD_PIN, INPUT);
    exit(signum);
}

static void eventLoop(LibcameraEncoder &app) {
    appPtr = &app;

    VideoOptions const *options = app.GetOptions();
    std::unique_ptr<Output> output = std::unique_ptr<Output>((Output *)(new LVOutput(options)));
    app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
    app.SetMetadataReadyCallback(std::bind(&Output::MetadataReady, output.get(), _1));

    app.OpenCamera();
    app.ConfigureVideo(LibcameraEncoder::FLAG_VIDEO_JPEG_COLOURSPACE);

    // GPIO setup
    if (wiringPiSetupGpio() == -1) {
        throw std::runtime_error("Failed to initialize WiringPi.");
    }
    pinMode(CMD_PIN, INPUT);
    pinMode(RESP_PIN, OUTPUT);
    pullUpDnControl(CMD_PIN, PUD_DOWN);

    // Start recording initially
    startRecording(app);

    // Main loop
    while (running.load()) {
        if (digitalRead(CMD_PIN) == LOW) {
            stopRecording(app);
        } else {
            startRecording(app);
        }

        // Handle libcamera events
        LibcameraEncoder::Msg msg = app.Wait();
        if (msg.type == LibcameraApp::MsgType::Timeout) {
            printf("Camera timeout... attempting to restart\n");
            app.StopCamera();
            app.StartCamera();
            continue;
        }
        if (msg.type == LibcameraApp::MsgType::Quit)
            return;
        else if (msg.type != LibcameraApp::MsgType::RequestComplete)
            throw std::runtime_error("Msg type not recognized!");

        CompletedRequestPtr &completedRequest = std::get<CompletedRequestPtr>(msg.payload);
        app.EncodeBuffer(completedRequest, app.VideoStream());

        delay(100); // Debounce and CPU usage control
    }
}

int main(int argc, char *argv[]) {
    try {
        // Register signal handler
        signal(SIGINT, signalHandler);

        LibcameraEncoder app;
        VideoOptions *options = app.GetOptions();
        if (options->Parse(argc, argv)) {
            eventLoop(app);
        }
    } catch (std::exception const &e) {
        printf("Error: %s\n", e.what());
        return -1;
    }

    return 0;
}