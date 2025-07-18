/*
 * Reed Solomon Encoder/Decoder
 *
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
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

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 */

#include "rs.h"

int RS::genPoly[] = {};
bool RS::initialized = false;

RS::RS()
{
  if (!RS::initialized)
  {
    RS::initialize_ecc();
    RS::initialized = true;
  }
}

void RS::initialize_ecc()
{
  /* Initialize the galois field arithmetic tables */
  GF::init_galois_tables();

  /* Compute the encoder generator polynomial */
  RS::compute_genpoly(NPAR, RS::genPoly);
}

void RS::compute_genpoly(int nbytes, int genpoly[])
{
  int i, tp[256], tp1[256];

  /* multiply (x + a^n) for n = 1 to nbytes */

  BK::zero_poly(tp1);
  tp1[0] = 1;

  for (i = 1; i <= nbytes; i++)
  {
    BK::zero_poly(tp);
    tp[0] = GF::gexp[i]; /* set up x+a^n */
    tp[1] = 1;

    BK::mult_polys(genpoly, tp, tp1);
    BK::copy_poly(tp1, genpoly);
  }
}

int RS::check_syndrome(void)
{
  int i, nz = 0;
  for (i = 0; i < NPAR; i++)
  {
    if (synBytes[i] != 0)
    {
      nz = 1;
      break;
    }
  }
  return nz;
}

void RS::encode_data(unsigned char msg[], int nbytes, unsigned char dst[])
{
  int i, LFSR[NPAR + 1], dbyte, j;

  for (i = 0; i < NPAR + 1; i++)
    LFSR[i] = 0;

  for (i = 0; i < nbytes; i++)
  {
    dbyte = msg[i] ^ LFSR[NPAR - 1];
    for (j = NPAR - 1; j > 0; j--)
    {
      LFSR[j] = LFSR[j - 1] ^ GF::gmult(RS::genPoly[j], dbyte);
    }
    LFSR[0] = GF::gmult(RS::genPoly[0], dbyte);
  }

  for (i = 0; i < NPAR; i++)
    pBytes[i] = LFSR[i];

  RS::build_codeword(msg, nbytes, dst);
}

void RS::decode_data(unsigned char data[], int nbytes)
{
  int i, j, sum;
  for (j = 0; j < NPAR; j++)
  {
    sum = 0;
    for (i = 0; i < nbytes; i++)
    {
      sum = data[i] ^ GF::gmult(GF::gexp[j + 1], sum);
    }
    synBytes[j] = sum;
  }
}

int RS::correct_errors_erasures(unsigned char codeword[], int csize, int nerasures, int erasures[])
{
  return BK::correct_errors_erasures(codeword, csize, nerasures, erasures, synBytes);
}

void RS::build_codeword(unsigned char msg[], int nbytes, unsigned char dst[])
{
  int i;

  for (i = 0; i < nbytes; i++)
    dst[i] = msg[i];

  for (i = 0; i < NPAR; i++)
  {
    dst[i + nbytes] = pBytes[NPAR - 1 - i];
  }
}

/* debugging routines */
void RS::print_parity(void)
{
  int i;
  Serial.print("Parity Bytes: ");
  for (i = 0; i < NPAR; i++)
  {
    Serial.print("[");
    Serial.print(i);
    Serial.print("]:");
    Serial.print(pBytes[i], HEX);
  }
  Serial.println();
}

void RS::print_syndrome(void)
{
  int i;
  Serial.print("Syndrome Bytes: ");
  for (i = 0; i < NPAR; i++)
  {
    Serial.print("[");
    Serial.print(i);
    Serial.print("]:");
    Serial.print(synBytes[i], HEX);
  }
  Serial.println();
}

void RS::debug_check_syndrome(void)
{
  int i;

  for (i = 0; i < 3; i++)
  {
    Serial.print(" inv log S[");
    Serial.print(i);
    Serial.print("]/S[");
    Serial.print(i + 1);
    Serial.print("] = ");
    Serial.println(GF::glog[GF::gmult(synBytes[i], GF::ginv(synBytes[i + 1]))]);
  }
}

// void zero_fill_from(unsigned char buf[], int from, int to)
// {
//   int i;
//   for (i = from; i < to; i++)
//     buf[i] = 0;
// }
