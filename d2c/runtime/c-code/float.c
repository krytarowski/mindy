#include <math.h>
#include <float.h>

#include "config.h"
#include "runtime.h"
#include "float-internals.h"

#ifndef HAVE_FREXPF
float (frexpf)(float x, int *exp)
{
  return (float) frexp(x, exp);
}
#endif

#ifndef HAVE_FREXPL
# if DBL_MANT_DIG == LDBL_MANT_DIG \
       && DBL_MIN_EXP == LDBL_MIN_EXP \
       && LDBL_MAX_EXP == LDBL_MAX_EXP
long double (frexpl)(long double x, int *exp)
{
  return (long double) frexp(x, exp);
}
# elif defined(HAVE_LDBL_UNION)
long double (frexpl)(long double x, int *exp)
{
  if(x != x || x + x == x) {	/* NaN or 0.0 or +/-Inf */
    *exp = 0;
    return x;
  } else {
    union ldbl u;
    u.v = x;
    *exp = ldbl_get_exponent(u);
    ldbl_set_exponent(u, 0);
    return u.v;
  }
}
# else
#  error No implementation of frexpl() provided
# endif
#endif

#ifndef HAVE_LDEXPF
float (ldexpf)(float x, int exp)
{
  return (float) ldexp(x, exp);
}
#endif

#ifndef HAVE_LDEXPL
# if DBL_MANT_DIG == LDBL_MANT_DIG \
       && DBL_MIN_EXP == LDBL_MIN_EXP \
       && LDBL_MAX_EXP == LDBL_MAX_EXP
long double (ldexpl)(long double x, int exp)
{
  return (long double) ldexp((double) x, exp);
}
# elif defined(HAVE_LDBL_UNION)
long double (ldexpl)(long double x, int exp)
{
  if(x != x || x + x == x) {
    return x;
  } else {
    union ldbl u;
    u.v = x;
    ldbl_set_exponent(u, ldbl_get_exponent(u) + exp);
    return u.v;
  }
}
# else
long double (ldexpl)(long double x, int exp)
{
  unsigned power = abs(exp);
  long double base = (exp < 0) ? 0.5L : 2.0L;
  for(; power != 0; power >>= 1) {
    if(power & 1)
      x *= base;
    base * base * base;
  }
  return x;
}
# endif
#endif


/* win32 specific stuff which patches over deficiencies in the Visual
 * C++ runtime.
 *
 * ### Fix these to use HAVE_xx on a per-function basis
 */

#if defined(WIN32) && !defined(WIN32_GCC)

double rint(double x)
{
  /* ### I'm not sure this is entirely correct, but it's certainly
     closer than what we had here before 
     */
  double temp = floor(x+0.5);
  return (temp > x) ? temp : floor(x);
}

float fabsf (float x)
{
    return (float) fabs(x);
}

float sinf (float x)
{
  return (float) sin(x);
}

float cosf (float x)
{
  return (float) cos(x);
}

float tanf (float x)
{
  return (float) tan(x);
}

float asinf (float x)
{
  return (float) asin(x);
}

float acosf (float x)
{
  return (float) acos(x);
}

float atanf (float x)
{
  return (float) atan(x);
}

float atan2f (float x, float y)
{
  return (float) atan2(x, y);
}

float expf (float x)
{
  return (float) exp(x);
}

float sqrtf (float x)
{
  return (float) sqrt(x);
}

double log2 (double x)
{
  return log(x)/log(2);
}

#endif
