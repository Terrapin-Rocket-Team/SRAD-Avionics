#ifndef RS_H
#define RS_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdio.h>
#endif

#include "definitions.h"
#include "galois.h"
#include "berlekamp.h"
#include "crcgen.h"

class RS
{
public:
    /* Encoder parity bytes */
    int pBytes[MAXDEG];

    /* Decoder syndrome bytes */
    int synBytes[MAXDEG];

    /* generator polynomial */
    static int genPoly[MAXDEG * 2];

    int DEBUG = FALSE;

    static bool initialized;

    RS();

    /* Reed Solomon encode/decode routines */

    /* Initialize lookup tables, polynomials, etc. */
    static void initialize_ecc(void);
    /* Create a generator polynomial for an n byte RS code.
     * The coefficients are returned in the genPoly arg.
     * Make sure that the genPoly array which is passed in is
     * at least n+1 bytes long.
     */
    static void compute_genpoly(int nbytes, int genpoly[]);
    /* Check if the syndrome is zero */
    int check_syndrome(void);
    /**********************************************************
     * Reed Solomon Decoder
     *
     * Computes the syndrome of a codeword. Puts the results
     * into the synBytes[] array.
     */
    void decode_data(unsigned char data[], int nbytes);
    /* Simulate a LFSR with generator polynomial for n byte RS code.
     * Pass in a pointer to the data array, and amount of data.
     *
     * The parity bytes are deposited into pBytes[], and the whole message
     * and parity are copied to dest to make a codeword.
     *
     */
    void encode_data(unsigned char msg[], int nbytes, unsigned char dst[]);

    /* Error location routines */
    int correct_errors_erasures(unsigned char codeword[], int csize, int nerasures, int erasures[]);

    // debugging
    void print_parity(void);
    void print_syndrome(void);
    void debug_check_syndrome(void);

private:
    /* Append the parity bytes onto the end of the message */
    void build_codeword(unsigned char msg[], int nbytes, unsigned char dst[]);
};

#endif