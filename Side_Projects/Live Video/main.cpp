#include <stdio.h>
#include <exception>
#include <wiringPi.h>
#include <atomic>
#include <csignal>
#include <libgpsmm.h>  // gpsd library
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "lv_output.hpp"

using namespace std::placeholders;

// GPIO configuration
#define CMD_PIN 14  // GPIO pin for command input
#define RESP_PIN 15 // GPIO pin for response output
#define FIX_PIN 16  // GPIO pin for fix indication (LED or Buzzer)

// GPS fix threshold
#define FIX_THRESHOLD 3 // below this fix level means no fix

std::atomic<bool> recording(false);
std::atomic<bool> running(true);
LibcameraEncoder *appPtr = nullptr;
bool hasFirstFix = false; // Indicates if the system has a valid GPS fix

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

// Function to blink the LED (or beep the buzzer) twice to indicate loss of fix
void indicateFixLoss() {
    for (int i = 0; i < 2; ++i) {
        digitalWrite(FIX_PIN, HIGH); // Turn LED on
        delay(500);                   // Wait for 500ms
        digitalWrite(FIX_PIN, LOW);  // Turn LED off
        delay(500);                   // Wait for 500ms
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
    pinMode(FIX_PIN, INPUT);
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
    pinMode(FIX_PIN, OUTPUT);
    pullUpDnControl(CMD_PIN, PUD_DOWN);

    // Start recording initially
    startRecording(app);

    // Main loop
    while (running.load()) {
        // Simulate GPS fix status (replace with actual GPS fix check)
        int gpsFix = getGPSFix(); // Assume this function gets the GPS fix level (0-5)
        
        if (gpsFix < FIX_THRESHOLD) {
            // GPS fix lost, indicate loss and reset status
            if (hasFirstFix) {
                hasFirstFix = false;
                indicateFixLoss(); // Trigger LED blink or beep
            }
        } else {
            // GPS fix acquired
            if (!hasFirstFix) {
                hasFirstFix = true;
                digitalWrite(FIX_PIN, HIGH); // Indicate GPS fix acquired (LED on)
            }
        }

        // Handle recording control based on CMD_PIN state
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

// Simulated GPS fix function (replace with actual GPS status check)
int getGPSFix() {
  gpsmm gps_rec("/dev/ttyAMA0", 9600); // Replace with correct serial device

    if (!gps_rec.stream(WATCH_ENABLE | WATCH_NEWSTYLE)) {
        std::cerr << "No GPS device found!" << std::endl;
        return -1;
    }

    struct gps_data_t data;

    if (gps_rec.waiting()) {
        gps_rec.next(&data);
        if (data.fix.mode == 0) {
            return 0;  // No Fix
        } else if (data.fix.mode == 1) {
            return 2;  // 2D Fix
        } else if (data.fix.mode == 2) {
            return 3;  // 3D Fix
        } else {
            return -1; // Unknown Fix
        }
    }

    return -1; // Error case
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