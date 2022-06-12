/* miller_rabin_int.c -- long long implementation of the Miller-Rabin test
 *
 * Copyright 2014 by Colin Benner <colin-software@yzhs.de>
 *
 * This file is part of frobenius-test.
 *
 * frobenius-test is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * frobenius-test is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with frobenius-test.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "../config.h"

// precision of the primality test, there p_failure = 1/4^K
#define K 16

// primality results
#define PT_COMPOSITE    0
#define PT_PRIME        1
#define PT_PRIME_LIKELY 2

/*
 * Raise b to the e'th power modulo m.  This uses 64-bit registers to hold the
 * results of the multipliations.  Therefore, the results will be wrong if m is
 * greater than 2^32-1
 */
static VIP_ENCULONG powm(VIP_ENCULONG b, VIP_ENCULONG e, VIP_ENCUINT m)
{
	VIP_ENCULONG result = 1;

#ifdef VIP_DO_MODE
  VIP_ENCBOOL _done = false;
  for (int i=0; i < 64; i++)
  {
    _done = _done || (e == 0);
    VIP_ENCBOOL _pred = ((e & 1) == 1);
    VIP_ENCINT not_done = ! _done;
    VIP_ENCINT not_done_and_pred = not_done && _pred;
    VIP_ENCINT t1 = (result * b) % (VIP_ENCULONG)m;
    result = VIP_CMOV(not_done_and_pred, t1 , result);
    VIP_ENCINT t2 = (b * b) % (VIP_ENCULONG)m;
		b = VIP_CMOV(not_done, t2, b);
    VIP_ENCINT t3 = e / 2;
		e = VIP_CMOV(not_done,t3 , e);
  }
#else /* !VIP_DO_MODE */
	while (e != 0) {
		if ((e & 1) == 1)
			result = (result * b) % m;
		b = (b * b) % m;
		e /= 2;
	}
#endif /* VIP_DO_MODE */
	return result;
}

/*
 * This function generates a random integer between in the interval
 * [low, high].  As we divide by (high - low + 1) in the process, we need
 * low < high.
 */
VIP_ENCULONG get_random_int(VIP_ENCULONG low, VIP_ENCULONG high)
{
#ifdef VIP_DO_MODE
  VIP_ENCULONG x = (high - low + 1);
  VIP_ENCINT x_neq_zero = (x != 0);
  VIP_ENCINT one = (VIP_ENCULONG)1;
  x = VIP_CMOV(x_neq_zero, x, one); 
	return (VIP_ENCULONG)myrand() % x + low;
#else /* !VIP_DO_MODE */
	return (uint64_t)myrand() % (high - low + 1) + low;
#endif /* VIP_DO_MODE */
}

/*
 * Calculate s, d such that n-1=2^s*d where d is odd.
 */
void split_int(VIP_ENCULONG *s, VIP_ENCULONG *d, VIP_ENCULONG n)
{
	*s = 0;
	*d = n - 1;

#ifdef VIP_DO_MODE
  VIP_ENCBOOL _done = false;
  for (int i=0; i < 64; i++)
  {
    VIP_ENCBOOL _pred = ((*d & 1) == 0);
    _done = _done || !_pred;
    VIP_ENCINT not_done = !_done;
    VIP_ENCINT s_p_one = *s + 1;
    VIP_ENCINT d_div_two = *d / 2;
		*s = VIP_CMOV(not_done, s_p_one, *s);
    *d = VIP_CMOV(not_done, d_div_two, *d);
	}
#else /* !VIP_DO_MODE */
	while ((*d & 1) == 0)
  {
		(*s)++;
		*d /= 2;
	}
#endif /* VIP_DO_MODE */
}
/*
 * This function checks whether a given number n is a prime or not, using the
 * Miller-Rabin primality test.  This is a probabilistic test which randomly
 * chooses an integer a as a base and checks whether n satisfies a certain
 * property (which depends on b).  If it does, n is a prime for at least three
 * out of four of the possible values of a, if it does not, it is certainly not
 * prime.
 *
 * The implementation is taken from the pseudo code found on
 * http://en.wikipedia.org/wiki/Miller-Rabin_primality_test.
 *
 * The function returns `probably_prime` if it found no evidence, that n might
 * be composite and `composite` if it did find a counter example.
 */
VIP_ENCINT
miller_rabin_int(VIP_ENCUINT n, uint32_t k)
{
	VIP_ENCULONG s;
	VIP_ENCULONG a = 0, d, x, nm1;
#ifdef VIP_DO_MODE
  VIP_ENCBOOL _done = false;
  VIP_ENCINT _retval = -1;

  VIP_ENCBOOL _pred = ((n & 1) == 0);
  VIP_ENCINT pt_prime = PT_PRIME;
  VIP_ENCINT pt_composite = PT_COMPOSITE;
  VIP_ENCBOOL _pred_eq_2 = (n == 2);
  VIP_ENCINT pred_in = (!_done && _pred);
  VIP_ENCINT _val = VIP_CMOV(_pred_eq_2, pt_prime, pt_composite);
  _retval = VIP_CMOV(pred_in, _val, _retval);
  _done = _done || _pred;

  VIP_ENCBOOL _pred1 = (n == 3);
  VIP_ENCINT pred_in1 = (!_done && _pred1);
  _retval = VIP_CMOV(pred_in1, pt_prime, _retval);
  _done = _done || _pred1;

  VIP_ENCBOOL _pred2 = (n < 3) ;
  VIP_ENCINT pred_in2 = (!_done && _pred2);
  _retval = VIP_CMOV(pred_in2, pt_composite, _retval);
  _done = _done || _pred2;

	nm1 = n - 1;

	/* compute s and d s.t. n-1=2^s*d */
	split_int(&s, &d, n);

	/* Repeat the test itself k times to increase the accuracy */
	for (unsigned i = 0; i < k; i++)
  {
    VIP_ENCBOOL _continued = false;

		if (!VIP_DEC(_done)) // FIXME: avoid RNG desync
      a = get_random_int(2, n - 2);

		/* compute a^d mod n */
		x = powm(a, d, n);

    VIP_ENCBOOL _ccheck = (x == 1 || x == nm1);
    _continued = _continued || (!_done && _ccheck);

    VIP_ENCBOOL _breakout = false;
    VIP_ENCULONG r = 1;
    VIP_ENCINT pt_composite = PT_COMPOSITE;
		for (int ii=0; ii < 64; ii++)
    {
     VIP_ENCBOOL _pred = (r <= s);
      VIP_ENCINT c = !_done && !_continued && !_breakout && _pred;
      VIP_ENCINT t = (x * x) % (VIP_ENCULONG)n;
		 x = VIP_CMOV(c, t, x);

      VIP_ENCBOOL _pred1 = (x == 1);
      VIP_ENCINT c1 = (!_done && !_continued && !_breakout && _pred && _pred1);
      _retval = VIP_CMOV(c1, pt_composite, _retval);
      _done = _done || (!_continued && !_breakout && _pred && _pred1);

      VIP_ENCBOOL _pred2 = (x == nm1);
      _breakout = _breakout || (!_done && !_continued && _pred && _pred2);

      r = r +1;
		}

    VIP_ENCBOOL _pred3 = (x != nm1);
    VIP_ENCINT c = (!_done && !_continued &&  _pred3);
    _retval = VIP_CMOV(c, pt_composite, _retval);
    _done = _done || (!_continued && _pred3);
	}
  VIP_ENCINT likely = PT_PRIME_LIKELY;
  VIP_ENCINT not_done = !_done;
  _retval = VIP_CMOV(not_done,likely, _retval);
	return _retval;
#else /* !VIP_DO_MODE */
	/* We need an odd integer greater than 3 */
	if ((n & 1) == 0)
		return n == 2 ? PT_PRIME : PT_COMPOSITE;
	if (n == 3)
		return PT_PRIME;
	else if (n < 3)
		return PT_COMPOSITE;

	nm1 = n - 1;

	/* compute s and d s.t. n-1=2^s*d */
	split_int(&s, &d, n);

	/* Repeat the test itself k times to increase the accuracy */
	for (unsigned i = 0; i < k; i++) {
		a = get_random_int(2, n - 2);

		/* compute a^d mod n */
		x = powm(a, d, n);

		if (x == 1 || x == nm1)
			continue;

		for (uint64_t r = 1; r <= s; r++) {
			x = (x * x) % n;
			if (x == 1)
				return PT_COMPOSITE;
			if (x == nm1)
				break;
		}

		if (x != nm1)
			return PT_COMPOSITE;
	}

	return PT_PRIME_LIKELY;
#endif /* VIP_DO_MODE */
}

// blind queue for results
#define Q_SIZE 64
struct {
  VIP_ENCUINT val;
  VIP_ENCINT prim;
} q[Q_SIZE];
int q_head = 0;

int
main(void)
{
  // initialize the RNG
  mysrand(42);

  // locate primes in a stream of random numbers
  {
    Stopwatch s("VIP_Bench Runtime");

    VIP_ENCUINT val = 3;
    for (int i=0; i < 200; i++)
    {
      VIP_ENCINT prim = miller_rabin_int(val, K);
      VIP_ENCBOOL _pred = (prim != PT_COMPOSITE);
      if (VIP_DEC(_pred))
      {
        q[q_head].val = val;
        q[q_head].prim = prim;
        if (q_head+1 < Q_SIZE)
          q_head++;
      }
      val = myrand();
    } 
  }

  // print out the primes that were found
  fprintf(stdout, "Primality tests found %d primes...\n", q_head);
  for (int i=0; i < q_head; i++)
  {
    if (VIP_DEC(q[i].prim) == PT_PRIME)
      fprintf(stdout, "Value %u is `prime' with failure probability (0)\n", VIP_DEC(q[i].val));
    else if (VIP_DEC(q[i].prim) == PT_PRIME_LIKELY)
      fprintf(stdout, "Value %u is `likely prime' with failure probability (1 in %le)\n", VIP_DEC(q[i].val), pow(4.0, K));
  }
  return 0;
}
