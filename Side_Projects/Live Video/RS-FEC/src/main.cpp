#include <Arduino.h>

#include "rs.h"

unsigned char msg[251] = {};
unsigned char codeword[256];
/* Initialization the ECC library */
RS rs;

uint32_t pTimer, pTime;

/* Some debugging routines to introduce errors or erasures
into a codeword.
*/

/* Introduce a byte error at LOC */
void byte_err(int err, int loc, unsigned char *dst)
{
  Serial.print("Adding Error at loc ");
  Serial.print(loc);
  Serial.print(", data ");
  Serial.println(dst[loc - 1], HEX);
  dst[loc - 1] ^= err;
}

/* Pass in location of error (first byte position is
labeled starting at 1, not 0), and the codeword.
*/
void byte_erasure(int loc, unsigned char dst[], int cwsize, int erasures[])
{
  Serial.print("Erasure at loc ");
  Serial.print(loc);
  Serial.print(", data ");
  Serial.println(dst[loc - 1], HEX);
  dst[loc - 1] = 0;
}

void print_word(int n, unsigned char *data)
{
  int i;
  for (i = 0; i < n; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void setup()
{

  Serial.begin(9600);

  if (CrashReport)
    Serial.println(CrashReport);

  int erasures[16];
  int nerasures = 0;

  /* ************** */

#define ML (sizeof(msg) + NPAR)

  /* Add one error and two erasures */

  /* We need to indicate the position of the erasures.  Eraseure
     positions are indexed (1 based) from the end of the message... */

  /*
     erasures[nerasures++] = ML-17;
     erasures[nerasures++] = ML-19;
  */

  /* Now decode -- encoded codeword size must be passed */
  rs.decode_data(codeword, ML);

  int fails = 0;
  for (int i = 0; i < 10; i++)
  {
    int j;
    // make random msg data
    for (j = 0; j < sizeof(msg); j++)
    {
      msg[j] = random(256) % 256;
    }
    // Serial.print("msg = ");
    // print_word(28, msg);
    pTimer = micros();
    rs.encode_data(msg, sizeof(msg), codeword);
    pTime = micros() - pTimer;
    Serial.print("Encoding took: ");
    Serial.print(pTime);
    Serial.println("us");
    // Serial.println("encoded data:");
    // print_word(ML, codeword);

    // add two random errors to codeword
    unsigned char r = random(256) % 256;
    int rloc = random(256) % ML;
    codeword[rloc] = r;

    unsigned char r2 = random(256) % 256;
    int rloc2 = random(256) % ML;
    codeword[rloc2] = r2;

    Serial.print("encoded data with two errors: ");
    Serial.print(r);
    Serial.print(" @ loc ");
    Serial.print(rloc);
    Serial.print(", ");
    Serial.print(r2);
    Serial.print(" @ loc ");
    Serial.println(rloc2);
    // print_word(ML, codeword);

    /* Now decode -- encoded codeword size must be passed */
    Serial.println("attempting decode");
    pTimer = micros();
    rs.decode_data(codeword, ML);
    pTime = micros() - pTimer;
    Serial.print("Decoding took: ");
    Serial.print(pTime);
    Serial.println("us");

    {
      int syndrome = rs.check_syndrome();
      Serial.print("syndrome = ");
      Serial.println(syndrome);
      /* check if syndrome is all zeros */
      if (syndrome == 0)
      {
        // no errs detected, codeword payload should match message
        for (int k = 0; k < sizeof(msg); k++)
        {
          if (msg[k] != codeword[k])
          {
            Serial.print("#### FAILURE TO DETECT ERROR @ ");
            Serial.print(k);
            Serial.print(": ");
            Serial.print(msg[k]);
            Serial.print(" != ");
            Serial.println(codeword[k]);
            fails++;
          }
        }
      }
      else
      {
        Serial.println("nonzero syndrome, attempting correcting errors");
        int result = 0;
        pTimer = micros();
        result = rs.correct_errors_erasures(codeword,
                                            ML,
                                            nerasures,
                                            erasures);
        pTime = micros() - pTimer;
        Serial.print("Correcting errors took: ");
        Serial.print(pTime);
        Serial.println("us");
        Serial.print("correct_errors_erasures = ");
        Serial.println(result);
        print_word(28, codeword);
        int k;
        for (k = 0; k < sizeof(msg); k++)
        {
          if (msg[k] != codeword[k])
          {
            Serial.print("##### FAILURE TO CORRECT ERROR @ ");
            Serial.print(k);
            Serial.print(": ");
            Serial.print(msg[k]);
            Serial.print(" != ");
            Serial.println(codeword[k]);
            fails++;
          }
        }
      }
    }
  }

  if (fails == 0)
  {
    Serial.println("\n\n All Tests Passed: No failures to correct codeword");
  }
  else
  {
    Serial.print("### ERROR Algorithm failed to correct codeword");
    Serial.print(fails);
    Serial.println(" times!!!");
  }
}

void loop()
{
}