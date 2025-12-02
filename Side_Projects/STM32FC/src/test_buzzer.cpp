#include "../tests/test_buzzer.h"
#include "../tests/test_menu.h"

// Buzzer pin configuration
#define BUZZER_PIN PD12

// Musical note frequencies for one octave (C4 to C5)
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

namespace BuzzerTest
{

    void setup()
    {
        Console.println("[BUZZER] Initializing buzzer test...");

        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);

        Console.println("[BUZZER] Buzzer configured on PD12");
        Console.println("[BUZZER] Setup complete");
    }

    void run()
    {
        Console.println("\n[BUZZER] Running buzzer test...");
        Console.println("[BUZZER] 'Do-Re-Mi' style melody for first four scale notes\n");

        // === Rhythm setup ===
        // Tempo in beats per minute
        const int TEMPO = 150;
        const int QUARTER = 60000 / TEMPO; // ms per quarter note
        const int EIGHTH = QUARTER / 2;    // ms per eighth note
        const int HALF = QUARTER * 2;      // ms per half note
        const int THREE_EIGHTH = EIGHTH * 3;
        const int WHOLE = QUARTER * 4;

        // We’ll do 4 phrases (Do, Re, Mi, Fa), 7 notes each.
        // Each phrase: 6 eighth notes + 1 half note to "land" the line.

        // Phrase 1: "Do, a deer, a female deer" – centered on C (Do)
        int notes[] = {
            NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_C4, NOTE_E4, // Do phrase
            // Phrase 2: "Re, a drop of golden sun" – centered on D (Re)
            NOTE_D4, NOTE_E4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_F4,
            // Phrase 3: "Mi, a name I call myself" – centered on E (Mi)
            NOTE_E4, NOTE_F4, NOTE_G4, NOTE_E4, NOTE_G4, NOTE_E4, NOTE_G4,
            // Phrase 4: "Fa, a long, long way to run" – centered on F (Fa)
            NOTE_F4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_A4};

        int durations[] = {
            // Phrase 1
            THREE_EIGHTH, EIGHTH, THREE_EIGHTH, EIGHTH, QUARTER, QUARTER, HALF,
            // Phrase 2
            THREE_EIGHTH, EIGHTH, EIGHTH, EIGHTH, EIGHTH, EIGHTH, WHOLE,
            // Phrase 3
            THREE_EIGHTH, EIGHTH, THREE_EIGHTH, EIGHTH, QUARTER, QUARTER, HALF,
            // Phrase 4
            THREE_EIGHTH, EIGHTH, EIGHTH, EIGHTH, EIGHTH, EIGHTH, WHOLE,};

        const char *solfegeForPhrase[] = {"Do", "Re", "Mi", "Fa"};
        const char *lyricForPhrase[] = {
            "Do, a deer, a female deer",
            "Re, a drop of golden sun",
            "Mi, a name I call myself",
            "Fa, a long, long way to run"};

        const int NUM_NOTES = sizeof(notes) / sizeof(notes[0]);
        const int NOTES_PER_PHRASE = 7;

        int currentPhrase = -1;

        for (int i = 0; i < NUM_NOTES; i++)
        {
            int phraseIndex = i / NOTES_PER_PHRASE;

            // When we enter a new phrase, print the lyric line
            if (phraseIndex != currentPhrase)
            {
                currentPhrase = phraseIndex;
                Console.println();
                Console.print("[BUZZER] ");
                Console.println(lyricForPhrase[currentPhrase]);
            }

            // Print which scale syllable we’re on (Do/Re/Mi/Fa)
            Console.print("  ♪ ");
            Console.println(solfegeForPhrase[currentPhrase]);

            // Play the note
            tone(BUZZER_PIN, notes[i]);
            delay(durations[i]);
            noTone(BUZZER_PIN);

            // Short gap between notes
            delay(50);
        }

        Console.println("\n[BUZZER] Test complete. Press '0' for menu.\n");
    }
}
