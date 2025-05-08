/*****************************
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
 *
 *
 * Multiplication and Arithmetic on Galois Field GF(256)
 *
 * From Mee, Daniel, "Magnetic Recording, Volume III", Ch. 5 by Patel.
 *
 *
 ******************************/

#include "galois.h"

int GF::gexp[512] = {};
int GF::glog[256] = {};

void GF::init_galois_tables(void)
{
  /* initialize the table of powers of alpha */
  GF::init_exp_table();
}

void GF::init_exp_table(void)
{
  int i, z;
  int pinit, p1, p2, p3, p4, p5, p6, p7, p8;

  pinit = p2 = p3 = p4 = p5 = p6 = p7 = p8 = 0;
  p1 = 1;

  gexp[0] = 1;
  gexp[255] = gexp[0];
  glog[0] = 0; /* shouldn't log[0] be an error? */

  for (i = 1; i < 256; i++)
  {
    pinit = p8;
    p8 = p7;
    p7 = p6;
    p6 = p5;
    p5 = p4 ^ pinit;
    p4 = p3 ^ pinit;
    p3 = p2 ^ pinit;
    p2 = p1;
    p1 = pinit;
    gexp[i] = p1 + p2 * 2 + p3 * 4 + p4 * 8 + p5 * 16 + p6 * 32 + p7 * 64 + p8 * 128;
    gexp[i + 255] = gexp[i];
  }

  for (i = 1; i < 256; i++)
  {
    for (z = 0; z < 256; z++)
    {
      if (gexp[z] == i)
      {
        glog[i] = z;
        break;
      }
    }
  }
}

int GF::gmult(int a, int b)
{
  int i, j;
  if (a == 0 || b == 0)
    return (0);
  i = glog[a];
  j = glog[b];
  return (gexp[i + j]);
}

int GF::ginv(int elt)
{
  return (gexp[255 - glog[elt]]);
}
