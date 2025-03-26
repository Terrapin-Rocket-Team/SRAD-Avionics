#ifndef BERLEKAMP_H
#define BERLEKAMP_H

#include "definitions.h"
#include "galois.h"

class BK
{
public:
    /* The Error Locator Polynomial, also known as Lambda or Sigma. Lambda[0] == 1 */
    static int Lambda[MAXDEG];

    /* The Error Evaluator Polynomial */
    static int Omega[MAXDEG];

    /* error locations found using Chien's search*/
    static int ErrorLocs[256];
    static int NErrors;

    /* erasure flags */
    static int ErasureLocs[256];
    static int NErasures;

    static int compute_discrepancy(int lambda[], int S[], int L, int n);
    static void init_gamma(int gamma[]);
    static void compute_modified_omega(int synBytes[]);
    static void mul_z_poly(int src[]);

    static void add_polys(int dst[], int src[]);
    static void copy_poly(int dst[], int src[]);
    static void scale_poly(int k, int poly[]);
    static void zero_poly(int poly[]);
    static void mult_polys(int dst[], int p1[], int p2[]);

    static int correct_errors_erasures(unsigned char codeword[],
                                       int csize,
                                       int nerasures,
                                       int erasures[],
                                       int synBytes[]);

private:
    static void Find_Roots(void);
    static void Modified_Berlekamp_Massey(int synBytes[]);
};

#endif