#ifndef RSCODE_DEFS_H
#define RSCODE_DEFS_H
/****************************************************************

  Below is NPAR, the only compile-time parameter you should have to
  modify.

  It is the number of parity bytes which will be appended to
  your data to create a codeword.

  Note that the maximum codeword size is 255, so the
  sum of your message length plus parity should be less than
  or equal to this maximum limit.

  In practice, you will get slooow error correction and decoding
  if you use more than a reasonably small number of parity bytes.
  (say, 10 or 20)

  ****************************************************************/
#define NPAR 4
/****************************************************************/

#define TRUE 1
#define FALSE 0

typedef unsigned long BIT32;
typedef unsigned short BIT16;

/* Maximum degree of various polynomials. */
#define MAXDEG (NPAR * 2)

#endif