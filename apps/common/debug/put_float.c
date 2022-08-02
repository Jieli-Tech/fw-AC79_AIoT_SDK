#include <math.h>
#include <string.h>
#include <stdio.h>

#define PAD_RIGHT 1
#define PAD_ZERO 2
#define SIGN    4               // Unsigned/signed long
#define SPACE   8               // Space if plus
#define LEFT    16              // Left justified
#define SPECIAL 32              // 0x
#define LARGE   64              // Use 'ABCDEF' instead of 'abcdef'
#define PLUS    128               // Show plus
#define CVTBUFSIZE  128

static char *flt(char **str, double num, int size, int precision, char fmt, int flags);

static char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
static char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);

void put_float(double fv)
{
    if (__builtin_isnan(fv)) {
        puts("nan");
    } else if (__builtin_isinf(fv)) {
        puts("inf");
    } else {
        flt((void *)0, fv, 10, 3, 'f', ' ' | SIGN);
    }
}


void putbyte(char a);
static void printchar(char **str, int c)
{
    if (str) {
        **str = c;
        ++(*str);
    } else {
        putbyte(c);
    }
}


static void cfltcvt(double value, char *buffer, char fmt, int precision)
{
    int decpt, sign, exp, pos;
    char *digits = (char *)0;
    char cvtbuf[CVTBUFSIZE + 1];
    int capexp = 0;
    int magnitude;

    if (fmt == 'G' || fmt == 'E') {
        capexp = 1;
        fmt += 'a' - 'A';
    }

    if (fmt == 'g') {
        digits = ecvtbuf(value, precision, & decpt, & sign, cvtbuf);
        magnitude = decpt - 1;

        if (magnitude < - 4  ||  magnitude > precision - 1) {
            fmt = 'e';
            precision -= 1;
        } else {
            fmt = 'f';
            precision -= decpt;
        }
    }

    if (fmt == 'e') {
        digits = ecvtbuf(value, precision + 1, & decpt, & sign, cvtbuf);

        if (sign) {
            * buffer ++ = '-';
        }

        * buffer ++ = * digits;

        if (precision > 0) {
            * buffer ++ = '.';
        }

        memcpy(buffer, digits + 1, precision);
        buffer += precision;
        * buffer ++ = capexp ? 'E' : 'e';

        if (decpt == 0) {
            if (value == 0.0) {
                exp = 0;
            } else {
                exp = - 1;
            }
        } else {
            exp = decpt - 1;
        }

        if (exp < 0) {
            * buffer ++ = '-';
            exp = - exp;
        } else {
            * buffer ++ = '+';
        }

        buffer[2] = (exp % 10) + '0';
        exp = exp / 10;
        buffer[1] = (exp % 10) + '0';
        exp = exp / 10;
        buffer[0] = (exp % 10) + '0';
        buffer += 3;
    } else if (fmt == 'f') {
        digits = fcvtbuf(value, precision, & decpt, & sign, cvtbuf);

        if (sign) {
            * buffer ++ = '-';
        }

        if (* digits) {
            if (decpt <= 0) {
                * buffer ++ = '0';
                * buffer ++ = '.';

                for (pos = 0; pos < - decpt; pos ++) {
                    * buffer ++ = '0';
                }

                while (* digits) {
                    * buffer ++ = * digits ++;
                }
            } else {
                pos = 0;

                while (* digits) {
                    if (pos ++ == decpt) {
                        * buffer ++ = '.';
                    }

                    * buffer ++ = * digits ++;
                }
            }
        } else {
            * buffer ++ = '0';

            if (precision > 0) {
                * buffer ++ = '.';

                for (pos = 0; pos < precision; pos ++) {
                    * buffer ++ = '0';
                }
            }
        }
    }

    * buffer = '\0';
}

static void forcdecpt(char *buffer)
{
    while (* buffer) {
        if (* buffer == '.') {
            return;
        }

        if (* buffer == 'e' || * buffer == 'E') {
            break;
        }

        buffer ++;
    }

    if (* buffer) {
        int n = strlen(buffer);

        while (n > 0) {
            buffer[n + 1] = buffer[n];
            n --;
        }

        * buffer = '.';
    } else {
        * buffer ++ = '.';
        * buffer = '\0';
    }
}

static void cropzeros(char *buffer)
{
    char *stop;

    while (* buffer && * buffer != '.') {
        buffer ++;
    }

    if (* buffer ++) {
        while (* buffer && * buffer != 'e' && * buffer != 'E') {
            buffer ++;
        }

        stop = buffer --;

        while (* buffer == '0') {
            buffer --;
        }

        if (* buffer == '.') {
            buffer --;
        }

        while ((*++ buffer = * stop ++));
    }
}

static char *flt(char **str, double num, int size, int precision, char fmt, int flags)
{
    char tmp[80];
    char c, sign;
    int n, i;

    if (flags & LEFT) {
        flags &= ~ PAD_ZERO;
    }

    // Determine padding and sign char
    c = (flags & PAD_ZERO) ? '0' : ' ';
    sign = 0;

    if (flags & SIGN) {
        if (num < 0.0) {
            sign = '-';
            num = - num;
            size --;
        } else if (flags & PLUS) {
            sign = '+';
            size --;
        } else if (flags & SPACE) {
            sign = ' ';
            size --;
        }
    }

    // Compute the precision value
    if (precision < 0) {
        precision = 6;    // Default precision: 6
    } else if (precision == 0 && fmt == 'g') {
        precision = 1;    // ANSI specified
    }

    // Convert floating point number to text
    cfltcvt(num, tmp, fmt, precision);

    // '#' and precision == 0 means force a decimal point
    if ((flags & SPECIAL) && precision == 0) {
        forcdecpt(tmp);
    }

    // 'g' format means crop zero unless '#' given
    if (fmt == 'g' && !(flags & SPECIAL)) {
        cropzeros(tmp);
    }

    n = strlen(tmp);

    // Output number with alignment and padding
    size -= n;

    if (!(flags & (PAD_ZERO | LEFT))) while (size -- > 0) {
            printchar(str, ' ');    //* str ++ = ' ';
        }

    if (sign) {
        printchar(str, sign);    //* str ++ = sign;
    }

    if (!(flags & LEFT)) while (size -- > 0) {
            printchar(str, c);    //* str ++ = c;
        }

    for (i = 0; i < n; i ++) {
        printchar(str, tmp[i]);    //* str ++ = tmp[i];
    }

    while (size -- > 0) {
        printchar(str, ' ');    //* str ++ = ' ';
    }

    return (char *)str;
}

static char *cvt(double arg, int ndigits, int *decpt, int *sign, char *buf, int eflag)
{
    int r2;
    double fi, fj;
    char *p, * p1;

    if (ndigits < 0) {
        ndigits = 0;
    }

    if (ndigits >= CVTBUFSIZE - 1) {
        ndigits = CVTBUFSIZE - 2;
    }

    r2 = 0;
    * sign = 0;
    p = & buf[0];

    if (arg < 0) {
        * sign = 1;
        arg = - arg;
    }

    arg = modf(arg, & fi);
    p1 = & buf[CVTBUFSIZE];

    if (fi != 0) {
        p1 = & buf[CVTBUFSIZE];

        while (fi != 0) {
            fj = modf(fi / 10, & fi);
            *-- p1 = (int)((fj + .03) * 10) + '0';
            r2 ++;
        }

        while (p1 < & buf[CVTBUFSIZE]) {
            * p ++ = * p1 ++;
        }
    } else if (arg > 0) {
        while ((fj = arg * 10) < 1) {
            arg = fj;
            r2 --;
        }
    }

    p1 = & buf[ndigits];

    if (eflag == 0) {
        p1 += r2;
    }

    * decpt = r2;

    if (p1 < & buf[0]) {
        buf[0] = '\0';
        return buf;
    }

    while (p <= p1 && p < & buf[CVTBUFSIZE]) {
        arg *= 10;
        arg = modf(arg, & fj);
        * p ++ = (int) fj + '0';
    }

    if (p1 >= & buf[CVTBUFSIZE]) {
        buf[CVTBUFSIZE - 1] = '\0';
        return buf;
    }

    p = p1;
    * p1 += 5;

    while (* p1 > '9') {
        * p1 = '0';

        if (p1 > buf) {
            ++*-- p1;
        } else {
            * p1 = '1';
            (* decpt)++;

            if (eflag == 0) {
                if (p > buf) {
                    * p = '0';
                }

                p ++;
            }
        }
    }

    * p = '\0';
    return buf;
}

static char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf)
{
    return cvt(arg, ndigits, decpt, sign, buf, 1);
}

static char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf)
{
    return cvt(arg, ndigits, decpt, sign, buf, 0);
}

