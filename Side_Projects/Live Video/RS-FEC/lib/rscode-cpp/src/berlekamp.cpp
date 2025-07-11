/***********************************************************************
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 *
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 * Berlekamp-Peterson and Berlekamp-Massey Algorithms for error-location
 *
 * From Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 205.
 *
 * This finds the coefficients of the error locator polynomial.
 *
 * The roots are then found by looking for the values of a^n
 * where evaluating the polynomial yields zero.
 *
 * Error correction is done using the error-evaluator equation  on pp 207.
 *
 */

#include "berlekamp.h"

int BK::Lambda[MAXDEG] = {};
int BK::Omega[MAXDEG] = {};
int BK::ErrorLocs[256] = {};
int BK::NErrors = {};
int BK::ErasureLocs[256] = {};
int BK::NErasures = {};

/* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
void BK::Modified_Berlekamp_Massey(int synBytes[])
{
  int n, L, L2, k, d, i;
  int psi[MAXDEG], psi2[MAXDEG], D[MAXDEG];
  int gamma[MAXDEG];

  /* initialize Gamma, the erasure locator polynomial */
  BK::init_gamma(gamma);

  /* initialize to z */
  BK::copy_poly(D, gamma);
  BK::mul_z_poly(D);

  BK::copy_poly(psi, gamma);
  k = -1;
  L = NErasures;

  for (n = NErasures; n < NPAR; n++)
  {

    d = compute_discrepancy(psi, synBytes, L, n);

    if (d != 0)
    {

      /* psi2 = psi - d*D */
      for (i = 0; i < MAXDEG; i++)
        psi2[i] = psi[i] ^ GF::gmult(d, D[i]);

      if (L < (n - k))
      {
        L2 = n - k;
        k = n - L;
        /* D = scale_poly(ginv(d), psi); */
        for (i = 0; i < MAXDEG; i++)
          D[i] = GF::gmult(psi[i], GF::ginv(d));
        L = L2;
      }

      /* psi = psi2 */
      for (i = 0; i < MAXDEG; i++)
        psi[i] = psi2[i];
    }

    BK::mul_z_poly(D);
  }

  for (i = 0; i < MAXDEG; i++)
    Lambda[i] = psi[i];
  BK::compute_modified_omega(synBytes);
}

/* given Psi (called Lambda in Modified_Berlekamp_Massey) and synBytes,
   compute the combined erasure/error evaluator polynomial as
   Psi*S mod z^4
  */
void BK::compute_modified_omega(int synBytes[])
{
  int i;
  int product[MAXDEG * 2];

  BK::mult_polys(product, Lambda, synBytes);
  BK::zero_poly(Omega);
  for (i = 0; i < NPAR; i++)
    Omega[i] = product[i];
}

/* gamma = product (1-z*a^Ij) for erasure locs Ij */
void BK::init_gamma(int gamma[])
{
  int e, tmp[MAXDEG];

  BK::zero_poly(gamma);
  BK::zero_poly(tmp);
  gamma[0] = 1;

  for (e = 0; e < BK::NErasures; e++)
  {
    BK::copy_poly(tmp, gamma);
    BK::scale_poly(GF::gexp[BK::ErasureLocs[e]], tmp);
    BK::mul_z_poly(tmp);
    BK::add_polys(gamma, tmp);
  }
}

// void compute_next_omega(int d, int A[], int dst[], int src[])
// {
//   int i;
//   for (i = 0; i < MAXDEG; i++)
//   {
//     dst[i] = src[i] ^ GF::gmult(d, A[i]);
//   }
// }

int BK::compute_discrepancy(int lambda[], int S[], int L, int n)
{
  int i, sum = 0;

  for (i = 0; i <= L; i++)
    sum ^= GF::gmult(lambda[i], S[n - i]);
  return (sum);
}

/********** polynomial arithmetic *******************/

void BK::add_polys(int dst[], int src[])
{
  int i;
  for (i = 0; i < MAXDEG; i++)
    dst[i] ^= src[i];
}

void BK::copy_poly(int dst[], int src[])
{
  int i;
  for (i = 0; i < MAXDEG; i++)
    dst[i] = src[i];
}

void BK::scale_poly(int k, int poly[])
{
  int i;
  for (i = 0; i < MAXDEG; i++)
    poly[i] = GF::gmult(k, poly[i]);
}

void BK::zero_poly(int poly[])
{
  int i;
  for (i = 0; i < MAXDEG; i++)
    poly[i] = 0;
}

/* polynomial multiplication */
void BK::mult_polys(int dst[], int p1[], int p2[])
{
  int i, j;
  int tmp1[MAXDEG * 2];

  for (i = 0; i < (MAXDEG * 2); i++)
    dst[i] = 0;

  for (i = 0; i < MAXDEG; i++)
  {
    for (j = MAXDEG; j < (MAXDEG * 2); j++)
      tmp1[j] = 0;

    /* scale tmp1 by p1[i] */
    for (j = 0; j < MAXDEG; j++)
      tmp1[j] = GF::gmult(p2[j], p1[i]);
    /* and mult (shift) tmp1 right by i */
    for (j = (MAXDEG * 2) - 1; j >= i; j--)
      tmp1[j] = tmp1[j - i];
    for (j = 0; j < i; j++)
      tmp1[j] = 0;

    /* add into partial product */
    for (j = 0; j < (MAXDEG * 2); j++)
      dst[j] ^= tmp1[j];
  }
}

/* multiply by z, i.e., shift right by 1 */
void BK::mul_z_poly(int src[])
{
  int i;
  for (i = MAXDEG - 1; i > 0; i--)
    src[i] = src[i - 1];
  src[0] = 0;
}

/* Finds all the roots of an error-locator polynomial with coefficients
 * Lambda[j] by evaluating Lambda at successive values of alpha.
 *
 * This can be tested with the decoder's equations case.
 */

void BK::Find_Roots(void)
{
  int sum, r, k;
  BK::NErrors = 0;

  for (r = 1; r < 256; r++)
  {
    sum = 0;
    /* evaluate lambda at r */
    for (k = 0; k < NPAR + 1; k++)
    {
      sum ^= GF::gmult(GF::gexp[(k * r) % 255], BK::Lambda[k]);
    }
    if (sum == 0)
    {
      BK::ErrorLocs[BK::NErrors] = (255 - r);
      BK::NErrors++;
      // if (DEBUG)
      //   fprintf(stderr, "Root found at r = %d, (255-r) = %d\n", r, (255 - r));
    }
  }
}

/* Combined Erasure And Error Magnitude Computation
 *
 * Pass in the codeword, its size in bytes, as well as
 * an array of any known erasure locations, along the number
 * of these erasures.
 *
 * Evaluate Omega(actually Psi)/Lambda' at the roots
 * alpha^(-i) for error locs i.
 *
 * Returns 1 if everything ok, or 0 if an out-of-bounds error is found
 *
 */

int BK::correct_errors_erasures(unsigned char codeword[],
                                int csize,
                                int nerasures,
                                int erasures[],
                                int synBytes[])
{
  int r, i, j, err;

  /* If you want to take advantage of erasure correction, be sure to
     set NErasures and ErasureLocs[] with the locations of erasures.
     */
  BK::NErasures = nerasures;
  for (i = 0; i < BK::NErasures; i++)
    BK::ErasureLocs[i] = erasures[i];

  BK::Modified_Berlekamp_Massey(synBytes);
  BK::Find_Roots();

  if ((BK::NErrors <= NPAR) && BK::NErrors > 0)
  {

    /* first check for illegal error locs */
    for (r = 0; r < BK::NErrors; r++)
    {
      if (BK::ErrorLocs[r] >= csize)
      {
        // if (DEBUG)
        //   fprintf(stderr, "Error loc i=%d outside of codeword length %d\n", i, csize);
        return (0);
      }
    }

    for (r = 0; r < NErrors; r++)
    {
      int num, denom;
      i = BK::ErrorLocs[r];
      /* evaluate Omega at alpha^(-i) */

      num = 0;
      for (j = 0; j < MAXDEG; j++)
        num ^= GF::gmult(BK::Omega[j], GF::gexp[((255 - i) * j) % 255]);

      /* evaluate Lambda' (derivative) at alpha^(-i) ; all odd powers disappear */
      denom = 0;
      for (j = 1; j < MAXDEG; j += 2)
      {
        denom ^= GF::gmult(BK::Lambda[j], GF::gexp[((255 - i) * (j - 1)) % 255]);
      }

      err = GF::gmult(num, GF::ginv(denom));
      // if (DEBUG)
      //   fprintf(stderr, "Error magnitude %#x at loc %d\n", err, csize - i);

      codeword[csize - i - 1] ^= err;
    }
    return (1);
  }
  else
  {
    // if (DEBUG && NErrors)
    //   fprintf(stderr, "Uncorrectable codeword\n");
    return (0);
  }
}
