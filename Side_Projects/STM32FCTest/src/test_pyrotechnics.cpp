#include "../tests/test_pyrotechnics.h"
#include "../tests/test_menu.h"

namespace {
    constexpr uint32_t ARM_PIN = PA3;
    constexpr uint32_t ARM_SENSE_PIN = PA2;
    constexpr uint32_t VSENSE_PIN = PC2_C;

    struct PyroChannel {
        const char* label;
        uint32_t firePin;
        uint32_t sensePin;
        bool senseHasAdc;
    };

    struct DigitalSample {
        int highCount;
        int totalSamples;
    };

    constexpr PyroChannel PYRO_CHANNELS[] = {
        {"Channel 1", PB1, PB0, true},
        {"Channel 2", PA7, PA6, true},
    };
    constexpr size_t PYRO_CHANNEL_COUNT = sizeof(PYRO_CHANNELS) / sizeof(PYRO_CHANNELS[0]);

    constexpr unsigned long ARM_SETTLE_MS = 150;
    constexpr unsigned long HEARTBEAT_MS = 10000;
    constexpr int DIGITAL_SAMPLES = 8;
    constexpr int ANALOG_SAMPLES = 8;
    constexpr float ADC_REFERENCE_VOLTS = 3.3f;
    constexpr int ADC_MAX_COUNTS = 4095;
    constexpr float SENSE_DIVIDER_RATIO = 24.9f / (100.0f + 24.9f);
    constexpr float VSENSE_DIVIDER_RATIO = 102.0f / (422.0f + 102.0f);
    constexpr float CONTINUITY_THRESHOLD_RATIO = 0.50f;

    void flushConsoleInput() {
        while (Console.available() > 0) {
            Console.read();
        }
    }

    void setFireOutputsSafe() {
        for (const auto& channel : PYRO_CHANNELS) {
            digitalWrite(channel.firePin, LOW);
        }
    }

    void setArmState(bool armed) {
        digitalWrite(ARM_PIN, armed ? HIGH : LOW);
        delay(ARM_SETTLE_MS);
    }

    void setSafeState() {
        setArmState(false);
        setFireOutputsSafe();
    }

    bool outputIsHigh(uint32_t pin) {
        return digitalRead(pin) == HIGH;
    }

    void setChannelFireState(size_t channelIndex, bool enabled) {
        if (channelIndex >= PYRO_CHANNEL_COUNT) {
            return;
        }

        digitalWrite(PYRO_CHANNELS[channelIndex].firePin, enabled ? HIGH : LOW);
    }

    bool toggleArmState() {
        const bool newState = !outputIsHigh(ARM_PIN);
        setArmState(newState);
        return newState;
    }

    bool toggleChannelFireState(size_t channelIndex) {
        const bool newState = !outputIsHigh(PYRO_CHANNELS[channelIndex].firePin);
        setChannelFireState(channelIndex, newState);
        return newState;
    }

    DigitalSample sampleDigitalPin(uint32_t pin, int samples = DIGITAL_SAMPLES) {
        DigitalSample result{0, samples};

        for (int i = 0; i < samples; i++) {
            if (digitalRead(pin) == HIGH) {
                result.highCount++;
            }
            delay(2);
        }

        return result;
    }

    bool sampleIsHigh(const DigitalSample& sample) {
        return sample.highCount >= ((sample.totalSamples / 2) + 1);
    }

    bool sampleIsStable(const DigitalSample& sample) {
        return sample.highCount == 0 || sample.highCount == sample.totalSamples;
    }

    int sampleAnalogPin(uint32_t pin, int samples = ANALOG_SAMPLES, bool restoreDigitalInput = true) {
        long total = 0;

        for (int i = 0; i < samples; i++) {
            total += analogRead(pin);
            delay(2);
        }

        if (restoreDigitalInput) {
            pinMode(pin, INPUT);
        }

        return static_cast<int>(total / samples);
    }

    float adcCountsToVolts(int counts) {
        return (static_cast<float>(counts) * ADC_REFERENCE_VOLTS) / static_cast<float>(ADC_MAX_COUNTS);
    }

    float readSupplyVoltage() {
        const int counts = sampleAnalogPin(VSENSE_PIN, ANALOG_SAMPLES, false);
        const float pinVolts = adcCountsToVolts(counts);
        return pinVolts / VSENSE_DIVIDER_RATIO;
    }

    float readSupplyPinVoltage() {
        const int counts = sampleAnalogPin(VSENSE_PIN, ANALOG_SAMPLES, false);
        return adcCountsToVolts(counts);
    }

    float readEstimatedPyroLineVoltage(uint32_t pin) {
        const int counts = sampleAnalogPin(pin);
        const float pinVolts = adcCountsToVolts(counts);
        return pinVolts / SENSE_DIVIDER_RATIO;
    }

    float readPyroSensePinVoltage(uint32_t pin) {
        const int counts = sampleAnalogPin(pin);
        return adcCountsToVolts(counts);
    }

    void printAnalogDetails(uint32_t pin, bool estimatePyroSide) {
        const int counts = sampleAnalogPin(pin);
        const float pinVolts = adcCountsToVolts(counts);

        Console.print(", ADC=");
        Console.print(counts);
        Console.print(" (~");
        Console.print(pinVolts, 2);
        Console.print("V");

        if (estimatePyroSide) {
            Console.print(", est=");
            Console.print(pinVolts / SENSE_DIVIDER_RATIO, 2);
            Console.print("V");
        }

        Console.print(")");
    }

    void printSupplyState(float supplyVolts) {
        const int counts = sampleAnalogPin(VSENSE_PIN, ANALOG_SAMPLES, false);
        const float pinVolts = adcCountsToVolts(counts);

        Console.print("[PYRO] Supply : V_SENSE=");
        Console.print(supplyVolts, 2);
        Console.print("V (ADC=");
        Console.print(counts);
        Console.print(", pin=");
        Console.print(pinVolts, 2);
        Console.println("V)");
    }

    void printOutputStates() {
        Console.print("[PYRO] Outputs: ARM=");
        Console.print(outputIsHigh(ARM_PIN) ? "HIGH" : "LOW");

        for (size_t i = 0; i < PYRO_CHANNEL_COUNT; i++) {
            Console.print(", CH");
            Console.print(i + 1);
            Console.print("_FIRE=");
            Console.print(outputIsHigh(PYRO_CHANNELS[i].firePin) ? "HIGH" : "LOW");
        }

        Console.println();
    }

    void printSampleInline(const char* label, const DigitalSample& sample) {
        Console.print(label);
        Console.print("=");
        Console.print(sampleIsHigh(sample) ? "HIGH" : "LOW");
        Console.print(" (");
        Console.print(sample.highCount);
        Console.print("/");
        Console.print(sample.totalSamples);

        if (!sampleIsStable(sample)) {
            Console.print(", unstable");
        }

        Console.print(")");
    }

    void printSenseStates(float supplyVolts) {
        Console.print("[PYRO] Senses : ");
        const DigitalSample armSample = sampleDigitalPin(ARM_SENSE_PIN);
        printSampleInline("ARM_SENS", armSample);
        printAnalogDetails(ARM_SENSE_PIN, false);

        const float supplyPinVolts = readSupplyPinVoltage();
        const float expectedPyroSensePinVolts = supplyPinVolts * (SENSE_DIVIDER_RATIO / VSENSE_DIVIDER_RATIO);

        for (size_t i = 0; i < PYRO_CHANNEL_COUNT; i++) {
            Console.print(", ");
            char label[16] = {};
            snprintf(label, sizeof(label), "CH%u_SENS", static_cast<unsigned>(i + 1));

            if (PYRO_CHANNELS[i].senseHasAdc) {
                const float pyroSensePinVolts = readPyroSensePinVoltage(PYRO_CHANNELS[i].sensePin);
                const float ratio = (expectedPyroSensePinVolts > 0.01f) ? (pyroSensePinVolts / expectedPyroSensePinVolts) : 0.0f;
                const bool connected = ratio >= CONTINUITY_THRESHOLD_RATIO;

                Console.print(label);
                Console.print("=");
                Console.print(connected ? "CONNECTED" : "OPEN");
                Console.print(" (");
                Console.print(pyroSensePinVolts, 2);
                Console.print("V, ");
                Console.print(ratio * 100.0f, 0);
                Console.print("% of expected");
                Console.print(", exp=");
                Console.print(expectedPyroSensePinVolts, 2);
                Console.print("V, threshold=");
                Console.print(CONTINUITY_THRESHOLD_RATIO * 100.0f, 0);
                Console.print("%)");
            } else {
                const DigitalSample sample = sampleDigitalPin(PYRO_CHANNELS[i].sensePin);
                printSampleInline(label, sample);
                Console.print(" [digital-only]");
            }
        }

        Console.println();
    }

    void printHeartbeat() {
        const float supplyVolts = readSupplyVoltage();

        Console.println();
        Console.println("[PYRO] Heartbeat");
        printOutputStates();
        printSupplyState(supplyVolts);
        printSenseStates(supplyVolts);
    }

    void printControls() {
        Console.println("[PYRO] Manual control mode. Single-key commands; Enter is not required.");
        Console.println("[PYRO] ARM output: PA3 (through series resistor), ARM sense: PA2");
        Console.println("[PYRO] CH1 FIRE/SENS: PB1 / PB0");
        Console.println("[PYRO] CH2 FIRE/SENS: PA7 / PA6");
        Console.println("[PYRO] Commands: a=toggle ARM, 1-2=toggle FIRE output, s=all LOW, p=print now, h/?=help, q/0=exit");
        Console.print("[PYRO] Both continuity channels are ADC-based and compare against ");
        Console.print(CONTINUITY_THRESHOLD_RATIO * 100.0f, 0);
        Console.println("% of V_SENSE.");
    }

    bool handleCommand(char input, unsigned long& lastHeartbeatMs) {
        if (input >= '1' && input < ('1' + static_cast<char>(PYRO_CHANNEL_COUNT))) {
            const size_t channelIndex = static_cast<size_t>(input - '1');
            Console.print("[PYRO] ");
            Console.print(PYRO_CHANNELS[channelIndex].label);
            Console.print(" FIRE -> ");
            Console.println(toggleChannelFireState(channelIndex) ? "HIGH" : "LOW");
            printHeartbeat();
            lastHeartbeatMs = millis();
            return false;
        }

        switch (input) {
            case 'a':
            case 'A':
                Console.print("[PYRO] ARM -> ");
                Console.println(toggleArmState() ? "HIGH" : "LOW");
                printHeartbeat();
                lastHeartbeatMs = millis();
                return false;

            case 's':
            case 'S':
                Console.println("[PYRO] Returning ARM and all FIRE outputs LOW.");
                setSafeState();
                printHeartbeat();
                lastHeartbeatMs = millis();
                return false;

            case 'p':
            case 'P':
                printHeartbeat();
                lastHeartbeatMs = millis();
                return false;

            case 'h':
            case 'H':
            case '?':
                printControls();
                lastHeartbeatMs = millis();
                return false;

            case 'q':
            case 'Q':
            case '0':
                return true;

            case '\r':
            case '\n':
                return false;

            default:
                Console.print("[PYRO] Unknown command: ");
                Console.println(input);
                Console.println("[PYRO] Press 'h' for help.");
                lastHeartbeatMs = millis();
                return false;
        }
    }
}

namespace PyrotechnicsTest {
    void setup() {
        Console.println("[PYRO] Initializing pyrotechnics test...");

        analogReadResolution(12);

        pinMode(ARM_PIN, OUTPUT);
        pinMode(ARM_SENSE_PIN, INPUT);
        pinMode(VSENSE_PIN, INPUT_ANALOG);

        for (const auto& channel : PYRO_CHANNELS) {
            pinMode(channel.firePin, OUTPUT);
            pinMode(channel.sensePin, INPUT);
        }

        setSafeState();

        Console.println("[PYRO] Setup complete");
    }

    void run() {
        Console.println("\n[PYRO] Running manual pyrotechnics IO test...");

        flushConsoleInput();
        setSafeState();
        printControls();
        printHeartbeat();

        unsigned long lastHeartbeatMs = millis();

        while (true) {
            while (Console.available() > 0) {
                const char input = static_cast<char>(Console.read());

                if (handleCommand(input, lastHeartbeatMs)) {
                    setSafeState();
                    Console.println("\n[PYRO] Exiting pyrotechnics test in a safe state.\n");
                    TestMenu::displayMenu();
                    return;
                }
            }

            if (millis() - lastHeartbeatMs >= HEARTBEAT_MS) {
                printHeartbeat();
                lastHeartbeatMs = millis();
            }

            delay(10);
        }
    }
}
