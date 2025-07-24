#ifndef GALOIS_H
#define GALOIS_H

/* This is one of 14 irreducible polynomials
 * of degree 8 and cycle length 255. (Ch 5, pp. 275, Magnetic Recording)
 * The high order 1 bit is implicit */
/* x^8 + x^4 + x^3 + x^2 + 1 */
#define PPOLY 0x1D

class GF
{
public:
    static int gexp[512];
    static int glog[256];

    static void init_galois_tables(void);
    static void init_exp_table(void);

    /* multiplication using logarithms */

    static int gmult(int a, int b);
    static int ginv(int elt);
};

#endif