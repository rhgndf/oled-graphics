/* Pi computation using Chudnovsky's algortithm.

 * Copyright 2002, 2005 Hanhong Xue (macroxue at yahoo dot com)

 * Slightly modified 2005 by Torbjorn Granlund to allow more than 2G
   digits to be computed.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gmp.h"
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#define A   13591409
#define B   545140134
#define C   640320
#define D   12

ArduiPi_OLED display;

#define BITS_PER_DIGIT   3.32192809488736234787
#define DIGITS_PER_ITER  14.1816474627254776555
#define DOUBLE_PREC      53

#ifdef __GNUC__
#define inline __inline__
#endif

char *prog_name;

#if CHECK_MEMUSAGE
#undef CHECK_MEMUSAGE
#define CHECK_MEMUSAGE							\
  do {									\
    char buf[100];							\
    snprintf (buf, 100,							\
	      "ps aguxw | grep '[%c]%s'", prog_name[0], prog_name+1);	\
    system (buf);							\
  } while (0)
#else
#undef CHECK_MEMUSAGE
#define CHECK_MEMUSAGE
#endif


/* Return user CPU time measured in milliseconds.  */

#if !defined (__sun) \
    && (defined (USG) || defined (__SVR4) || defined (_UNICOS) \
	|| defined (__hpux))
int
cputime ()
{
  return (int) ((double) clock () * 1000 / CLOCKS_PER_SEC);
}
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

int
cputime ()
{
  struct rusage rus;

  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}
#endif

/*///////////////////////////////////////////////////////////////////////////*/

mpf_t t1, t2;

/* r = sqrt(x) */
void
my_sqrt_ui(mpf_t r, unsigned long x)
{
  unsigned long prec, bits, prec0;

  prec0 = mpf_get_prec(r);

  if (prec0<=DOUBLE_PREC) {
    mpf_set_d(r, sqrt(x));
    return;
  }

  bits = 0;
  for (prec=prec0; prec>DOUBLE_PREC;)
    {
      int bit = prec&1;
      prec = (prec+bit)/2;
      bits = bits*2+bit;
    }

  mpf_set_prec_raw(t1, DOUBLE_PREC);
  mpf_set_d(t1, 1/sqrt(x));

  while (prec<prec0)
    {
      prec *=2;
      if (prec<prec0)
	{
	  /* t1 = t1+t1*(1-x*t1*t1)/2; */
	  mpf_set_prec_raw(t2, prec);
	  mpf_mul(t2, t1, t1);         /* half x half -> full */
	  mpf_mul_ui(t2, t2, x);
	  mpf_ui_sub(t2, 1, t2);
	  mpf_set_prec_raw(t2, prec/2);
	  mpf_div_2exp(t2, t2, 1);
	  mpf_mul(t2, t2, t1);         /* half x half -> half */
	  mpf_set_prec_raw(t1, prec);
	  mpf_add(t1, t1, t2);
	}
      else
	{
	  break;
	}
      prec -= (bits&1);
      bits /=2;
    }
  /* t2=x*t1, t1 = t2+t1*(x-t2*t2)/2; */
  mpf_set_prec_raw(t2, prec0/2);
  mpf_mul_ui(t2, t1, x);
  mpf_mul(r, t2, t2);          /* half x half -> full */
  mpf_ui_sub(r, x, r);
  mpf_mul(t1, t1, r);          /* half x half -> half */
  mpf_div_2exp(t1, t1, 1);
  mpf_add(r, t1, t2);
}

/* r = y/x   WARNING: r cannot be the same as y. */
#if __GMP_MP_RELEASE >= 50001
#define my_div mpf_div
#else
void
my_div(mpf_t r, mpf_t y, mpf_t x)
{
  unsigned long prec, bits, prec0;

  prec0 = mpf_get_prec(r);

  if (prec0<=DOUBLE_PREC) {
    mpf_set_d(r, mpf_get_d(y)/mpf_get_d(x));
    return;
  }

  bits = 0;
  for (prec=prec0; prec>DOUBLE_PREC;) {
    int bit = prec&1;
    prec = (prec+bit)/2;
    bits = bits*2+bit;
  }

  mpf_set_prec_raw(t1, DOUBLE_PREC);
  mpf_ui_div(t1, 1, x);

  while (prec<prec0) {
    prec *=2;
    if (prec<prec0) {
      /* t1 = t1+t1*(1-x*t1); */
      mpf_set_prec_raw(t2, prec);
      mpf_mul(t2, x, t1);          /* full x half -> full */
      mpf_ui_sub(t2, 1, t2);
      mpf_set_prec_raw(t2, prec/2);
      mpf_mul(t2, t2, t1);         /* half x half -> half */
      mpf_set_prec_raw(t1, prec);
      mpf_add(t1, t1, t2);
    } else {
      prec = prec0;
      /* t2=y*t1, t1 = t2+t1*(y-x*t2); */
      mpf_set_prec_raw(t2, prec/2);
      mpf_mul(t2, t1, y);          /* half x half -> half */
      mpf_mul(r, x, t2);           /* full x half -> full */
      mpf_sub(r, y, r);
      mpf_mul(t1, t1, r);          /* half x half -> half */
      mpf_add(r, t1, t2);
      break;
    }
    prec -= (bits&1);
    bits /=2;
  }
}
#endif

/*///////////////////////////////////////////////////////////////////////////*/

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct {
  unsigned long max_facs;
  unsigned long num_facs;
  unsigned long *fac;
  unsigned long *pow;
} fac_t[1];

typedef struct {
  long int fac;
  long int pow;
  long int nxt;
} sieve_t;

sieve_t *sieve;
long int sieve_size;
fac_t   ftmp, fmul;

#define INIT_FACS 32

void
fac_show(fac_t f)
{
  long int i;
  for (i=0; i<f[0].num_facs; i++)
    if (f[0].pow[i]==1)
      printf("%ld ", f[0].fac[i]);
    else
      printf("%ld^%ld ", f[0].fac[i], f[0].pow[i]);
  printf("\n");
}

inline void
fac_reset(fac_t f)
{
  f[0].num_facs = 0;
}

inline void
fac_init_size(fac_t f, long int s)
{
  if (s<INIT_FACS)
    s=INIT_FACS;

  f[0].fac  = malloc(s*sizeof(unsigned long)*2);
  f[0].pow  = f[0].fac + s;
  f[0].max_facs = s;

  fac_reset(f);
}

inline void
fac_init(fac_t f)
{
  fac_init_size(f, INIT_FACS);
}

inline void
fac_clear(fac_t f)
{
  free(f[0].fac);
}

inline void
fac_resize(fac_t f, long int s)
{
  if (f[0].max_facs < s) {
    fac_clear(f);
    fac_init_size(f, s);
  }
}

/* f = base^pow */
inline void
fac_set_bp(fac_t f, unsigned long base, long int pow)
{
  long int i;
  assert(base<sieve_size);
  for (i=0, base/=2; base>0; i++, base = sieve[base].nxt) {
    f[0].fac[i] = sieve[base].fac;
    f[0].pow[i] = sieve[base].pow*pow;
  }
  f[0].num_facs = i;
  assert(i<=f[0].max_facs);
}

/* r = f*g */
inline void
fac_mul2(fac_t r, fac_t f, fac_t g)
{
  long int i, j, k;

  for (i=j=k=0; i<f[0].num_facs && j<g[0].num_facs; k++) {
    if (f[0].fac[i] == g[0].fac[j]) {
      r[0].fac[k] = f[0].fac[i];
      r[0].pow[k] = f[0].pow[i] + g[0].pow[j];
      i++; j++;
    } else if (f[0].fac[i] < g[0].fac[j]) {
      r[0].fac[k] = f[0].fac[i];
      r[0].pow[k] = f[0].pow[i];
      i++;
    } else {
      r[0].fac[k] = g[0].fac[j];
      r[0].pow[k] = g[0].pow[j];
      j++;
    }
  }
  for (; i<f[0].num_facs; i++, k++) {
    r[0].fac[k] = f[0].fac[i];
    r[0].pow[k] = f[0].pow[i];
  }
  for (; j<g[0].num_facs; j++, k++) {
    r[0].fac[k] = g[0].fac[j];
    r[0].pow[k] = g[0].pow[j];
  }
  r[0].num_facs = k;
  assert(k<=r[0].max_facs);
}

/* f *= g */
inline void
fac_mul(fac_t f, fac_t g)
{
  fac_t tmp;
  fac_resize(fmul, f[0].num_facs + g[0].num_facs);
  fac_mul2(fmul, f, g);
  tmp[0]  = f[0];
  f[0]    = fmul[0];
  fmul[0] = tmp[0];
}

/* f *= base^pow */
inline void
fac_mul_bp(fac_t f, unsigned long base, unsigned long pow)
{
  fac_set_bp(ftmp, base, pow);
  fac_mul(f, ftmp);
}

/* remove factors of power 0 */
inline void
fac_compact(fac_t f)
{
  long int i, j;
  for (i=0, j=0; i<f[0].num_facs; i++) {
    if (f[0].pow[i]>0) {
      if (j<i) {
	      f[0].fac[j] = f[0].fac[i];
	f[0].pow[j] = f[0].pow[i];
      }
      j++;
    }
  }
  f[0].num_facs = j;
}

/* convert factorized form to nu