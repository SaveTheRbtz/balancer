#define USE_INTERNAL_STRTOD
#include <util/system/defaults.h>

#if defined(_freebsd_) && !defined(__LONG_LONG_SUPPORTED)
    #define __LONG_LONG_SUPPORTED
#endif

#include <limits.h>

#if defined(_linux_)
    #ifndef LLONG_MIN
        #define LLONG_MIN   LONG_LONG_MIN
    #endif
    #ifndef LLONG_MAX
        #define LLONG_MAX   LONG_LONG_MAX
    #endif
    #ifndef ULLONG_MAX
        #define ULLONG_MAX  ULONG_LONG_MAX
    #endif
#endif

#include <cstdio>
#include <string>
#include <cstring>

#include <util/generic/stroka.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>
#include <util/generic/typetraits.h>

#include <util/memory/tempbuf.h>
#include <util/string/escape.h>
#include <util/string/traits.h>

#include "type.h"

#include "cast.h"

#define HEX_MACROS_MAP(mac, type) mac(type, 2) mac(type, 3) mac(type, 4) \
    mac(type, 5) mac(type, 6) mac(type, 7) mac(type, 8) \
    mac(type, 9) mac(type, 10) mac(type, 11) mac(type, 12) \
    mac(type, 13) mac(type, 14) mac(type, 15) mac(type, 16)

/*
 * ------------------------------ formatters ------------------------------
 */

template <>
size_t ToStringImpl<bool>(bool t, char* buf, size_t len) {
    if (!len) {
        ythrow yexception() << "zero length";
    }
    *buf = t ? '1' : '0';
    return 1;
}

// this functions is added to avoid warnings for T == bool
template <int base, class T>
inline T SafeMod(T t);

template <int base, class T>
inline T SafeMod(T t) {
    T res = t % (T)base;
    return res;
}
template <int base>
inline bool SafeMod(bool t) {
    return t;
}

template <int base, class T>
inline void SafeShift(T& t);

template <int base, class T>
inline void SafeShift(T& t) {
    t /= base;
}
template <int base>
inline void SafeShift(bool& t) {
    t = 0;
}

static char IntToChar[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

STATIC_ASSERT(ARRAY_SIZE(IntToChar) == 16);

template <bool isSigned>
struct TSelector;

template <>
struct TSelector<false> {
    template <int base, class T>
    static inline size_t Format(T t, char* buf, size_t len) {
        STATIC_ASSERT(1 < base && base < 17);
        if (!len) {
            ythrow yexception() << "zero length";
        }

        char* tmp = buf;

        do {
            *tmp++ = IntToChar[(ui32)SafeMod<base>(t)];
            SafeShift<base>(t);
        } while (t && --len);

        if (t) {
            ythrow yexception() << "not enough room in buffer";
        }

        const size_t ret = tmp - buf;

        --tmp;

        while (buf < tmp) {
            const char c = *buf;

            *buf = *tmp;
            *tmp = c;
            ++buf; --tmp;
        }

        return ret;
    }
};

template <>
struct TSelector<true> {
    template <int base, class T>
    static inline size_t Format(T t, char* buf, size_t len) {
        STATIC_ASSERT(1 < base && base < 17);
        typedef typename TTypeTraits<T>::TUnsigned TUnsigned;

        if (t < 0) {
            if (len < 2) {
                ythrow yexception() << "not enough room in buffer";
            }

            const size_t ret = TSelector<false>::Format<base>((TUnsigned)(-t), buf + 1, len - 1);
            *buf = '-';

            return ret + 1;
        }

        return TSelector<false>::Format<base>((TUnsigned)t, buf, len);
    }
};

template <class T, int base>
static inline size_t FormatInt(T t, char* buf, size_t len) {
    return TSelector<TTypeTraits<T>::IsSignedInt>::template Format<base>(t, buf, len);
}

template <class T>
struct TFltModifiers;

#define DEF_FLT_MOD(type, modifierWrite, modifierRead)\
    template <>\
    struct TFltModifiers<type> {\
        static const char *ModifierWrite, *ModifierReadAndChar;\
    };\
    \
    const char* TFltModifiers<type>::ModifierWrite = modifierWrite;\
    const char* TFltModifiers<type>::ModifierReadAndChar = modifierRead "%c";

DEF_FLT_MOD(float, "%g", "%g")
DEF_FLT_MOD(double, "%.10lg", "%lg")
DEF_FLT_MOD(long double, "%.10Lg", "%Lg")

#undef DEF_FLT_MOD

template <class T>
static inline size_t FormatFlt(T t, char* buf, size_t len) {
    const int ret = snprintf(buf, len, TFltModifiers<T>::ModifierWrite, t);

    if (ret < 0 || (size_t)ret > len) {
        ythrow yexception() << "cannot format float";
    }

    return (size_t)ret;
}

#define DEF_HEX_INT_SPEC(type, base)\
    template <>\
    size_t IntToString<base, type>(type t, char* buf, size_t len) {\
        return FormatInt<type, base>(t, buf, len);\
    }

#define DEF_RAW_INT_SPEC(type)\
    template <>\
    size_t ToStringImpl<type>(type t, char* buf, size_t len) {\
        return FormatInt<type, 10>(t, buf, len);\
    }\
    HEX_MACROS_MAP(DEF_HEX_INT_SPEC, type)

#define DEF_INT_SPEC(type)\
    DEF_RAW_INT_SPEC(signed type)\
    DEF_RAW_INT_SPEC(unsigned type)

DEF_RAW_INT_SPEC(char)
DEF_RAW_INT_SPEC(wchar_t)

DEF_INT_SPEC(char)
DEF_INT_SPEC(short)
DEF_INT_SPEC(int)
DEF_INT_SPEC(long)
DEF_INT_SPEC(long long)

#undef DEF_RAW_INT_SPEC
#undef DEF_INT_SPEC
#undef DEF_HEX_INT_SPEC

#define DEF_FLT_SPEC(type)\
    template <>\
    size_t ToStringImpl<type>(type t, char* buf, size_t len) {\
        return FormatFlt<type>(t, buf, len);\
    }

DEF_FLT_SPEC(long double)

// if USE_INTERNAL_STROD is defined, ToStringImpl for float and double will be specialized
// after dtoa.cpp included
#ifndef USE_INTERNAL_STRTOD
DEF_FLT_SPEC(float)
DEF_FLT_SPEC(double)
#endif

#undef DEF_FLT_SPEC

/*
 * ------------------------------ parsers ------------------------------
 */

template <>
bool FromStringImpl<bool>(const char* data, size_t len) {
    if (len == 1) {
        if (data[0] == '0') {
            return false;
        } else if (data[0] == '1') {
            return true;
        }
    }
    TStringBuf buf(data, len);
    if (istrue(buf)) {
        return true;
    } else if (isfalse(buf)) {
        return false;
    }
    ythrow TFromStringException() << "cannot parse bool(" << Stroka(data, len) << ")";
}

template <class T>
struct TAbs;

#define DEF_ABS(type, VAL1, VAL2)\
    template <>\
    struct TAbs<signed type> {\
        static const unsigned type Min = VAL1 - VAL2;\
        static const unsigned type Max = VAL2;\
    };\
    template <>\
    struct TAbs<unsigned type> {\
        static const unsigned type Min = 0;\
        static const unsigned type Max = VAL1;\
    };

DEF_ABS(short, USHRT_MAX, SHRT_MAX)
DEF_ABS(int, UINT_MAX, INT_MAX)
DEF_ABS(long, ULONG_MAX, LONG_MAX)
DEF_ABS(long long, ULLONG_MAX, LLONG_MAX)

#undef DEF_ABS

template <bool isSigned>
struct TIntParser;

static int LetterToIntMap[] = {
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 0, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 20, 20,
    20, 20, 20, 20, 20, 10, 11, 12, 13, 14,
    15, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 10, 11, 12,
    13, 14, 15,
};

template <>
struct TIntParser<false> {
    template <typename T, int base, typename TChar>
    static inline T Parse(const TChar* begin0, const TChar* end) {
        STATIC_ASSERT(1 < base && base < 17);
        typedef typename TCharStringTraits<TChar>::TString TString;

        const TChar* begin = begin0;

        if (begin == end) {
            ythrow TFromStringException() << "cannot parse empty string as number";
        }

        if (*begin == '+') {
            ++begin;

            if (begin == end) {
                ythrow TFromStringException() << "cannot parse string \"+\" as number";
            }
        }

        T ret = 0;
        const T max1 = TAbs<T>::Max;
        const T max2 = max1 / base;

        while (begin != end) {
            const T sym = *begin;

            if (sym >= ARRAY_SIZE(LetterToIntMap)) {
                ythrow TFromStringException() << "unexpected symbol \"" << EscapeC(*begin)
                        << "\" at pos " << (begin - begin0) << " in string " << TString(begin0, end - begin0).Quote();
            }

            const int num = LetterToIntMap[sym];
            if (num >= base) {
                ythrow TFromStringException() << "unexpected symbol \"" << EscapeC(*begin)
                        << "\" at pos " << (begin - begin0) << " in string " << TString(begin0, end - begin0).Quote();
            }

            if (ret > max2) {
                ythrow TFromStringException() << "integer overflow in string " << TString(begin0, end - begin0).Quote();
            }

            ret *= base;

            const T digit = static_cast<T>(num);

            if (ret > max1 - digit) {
                ythrow TFromStringException() << "integer overflow in string " << TString(begin0, end - begin0).Quote();
            }

            ret += digit;

            ++begin;
        }

        return ret;
    }
};

template <>
struct TIntParser<true> {
    template <typename T, int base, typename TChar>
    static inline T Parse(const TChar* begin, const TChar* end) {
        STATIC_ASSERT(1 < base && base < 17);
        typedef typename TTypeTraits<T>::TUnsigned TUnsigned;

        if (begin == end) {
            ythrow TFromStringException() << "cannot parse zero length number";
        }

        if (*begin == '-') {
            const TUnsigned ret = TIntParser<false>::Parse<TUnsigned, base>(begin + 1, end);

            if (ret > TAbs<T>::Min) {
                ythrow TFromStringException() << "overflow";
            }

            return -((T)ret);
        }

        const TUnsigned ret = TIntParser<false>::Parse<TUnsigned, base>(begin, end);

        if (ret > TAbs<T>::Max) {
            ythrow TFromStringException() << "overflow";
        }

        return (T)ret;
    }
};

template <typename T, int base, typename TChar>
static inline T ParseInt(const TChar* data, size_t len) {
    return TIntParser<TTypeTraits<T>::IsSignedInt>::template Parse<T, base>(data, data + len);
}

template <class T>
static inline T ParseFlt(const char* data, size_t len) {
    /*
     * TODO
     */

    if (len > 256) {
        len = 256;
    }

    char* c = (char*)alloca(len + 1);
    memcpy(c, data, len);
    c[len] = 0;

    T ret;
    char ec;

    // try to read a value and an extra character in order to catch cases when
    // the string start with a valid float but is followed by unexpected characters
    if (sscanf(c, TFltModifiers<T>::ModifierReadAndChar, &ret, &ec) == 1) {
        return ret;
    }

    ythrow TFromStringException() << "cannot parse float(" << Stroka(data, len) << ")";
}

template <>
Stroka FromStringImpl<Stroka>(const char* data, size_t len) {
    return Stroka(data, len);
}

template <>
Wtroka FromStringImpl<Wtroka>(const wchar16* data, size_t len) {
    return Wtroka(data, len);
}

#define DEF_HEX_INT_SPEC(type, base)\
    template <>\
    type IntFromString<type, base>(const char* data, size_t len) {\
        return ParseInt<type, base>(data, len);\
    }\
    template <>\
    type IntFromString<type, base>(const wchar16* data, size_t len) {\
        return ParseInt<type, base>(data, len);\
    }

#define DEF_RAW_INT_SPEC(type)\
    template <>\
    type FromStringImpl<type>(const char* data, size_t len) {\
        return ParseInt<type, 10>(data, len);\
    }\
    template <>\
    type FromStringImpl<type>(const wchar16* data, size_t len) {\
        return ParseInt<type, 10>(data, len);\
    }\
    HEX_MACROS_MAP(DEF_HEX_INT_SPEC, type)

#define DEF_INT_SPEC(type)\
    DEF_RAW_INT_SPEC(signed type)\
    DEF_RAW_INT_SPEC(unsigned type)

DEF_INT_SPEC(short)
DEF_INT_SPEC(int)
DEF_INT_SPEC(long)
DEF_INT_SPEC(long long)

#undef DEF_INT_SPEC
#undef DEF_RAW_INT_SPEC
#undef DEF_HEX_INT_SPEC

#define DEF_FLT_SPEC(type)\
    template <>\
    type FromStringImpl<type>(const char* data, size_t len) {\
        return ParseFlt<type>(data, len);\
    }

DEF_FLT_SPEC(long double)

#undef DEF_FLT_SPEC

ui32 strtoui32(const char *s) throw () {
    return strtonum_u<ui32>(s);
}

int yatoi(const char *s) throw () {
    return (int)strtonum_u<long>(s);
}

static inline double StrToDSystem(const char* b, const char* e, char** se) {
    const size_t len = e - b;
    TTempBuf tmpBuf(len + 1);
    char* tmp = (char*)tmpBuf.Data();

    memcpy(tmp, b, len);
    tmp[len] = 0;
    char* ret = 0;
    const double res = strtod(tmp, &ret);

    if (se) {
        *se = (char*)(b + (ret - tmp));
    }

    return res;
}

// Using StrToD for float and double because it is faster than sscanf.
template <>
double FromStringImpl<double>(const char* data, size_t len) {
    char* se;
    double d = StrToD(data, data + len, &se);

    if (se != data + len) {
        ythrow TFromStringException() << "cannot parse float(" << Stroka(data, len) << ")";
    }
    return d;
}

template <>
float FromStringImpl<float>(const char* data, size_t len) {
    return (float)FromStringImpl<double>(data, len);
}

#if defined(USE_INTERNAL_STRTOD)
#define MULTIPLE_THREADS
#include "dtoa.cpp"

double StrToD(const char* b, const char* e, char** se) {
    return StrToDImpl(b, e, se);
}

static inline void FormatExponent(int expt, char* e) {
    *e++ = 'e';
    *e++ = expt < 0 ? '-' : '+';
    expt = expt > 0 ? expt : -expt;
    if (expt < 10) {
        *e++ = '0';
    }
    e += ToString(expt, e, 6);
    *e = 0;
}

static inline size_t Dtoa(double d, char* buf, size_t len, int mode, int ndigits) {
    char* out = buf;
    char* oute = buf + len;

    int decpt;
    int sign;
    char *se;

    NDtoa::TParserClosure p;
    char* s = p.dtoa(d, mode, ndigits, &decpt, &sign, &se);

    // there should be no trailing zeroes in dtoa result, but they occur sometimes
    while (se > s+1 && se[-1] == '0') {
        --se;
    }

    int zeroes = 0;
    char exp[16];
    exp[0] = 0;

    if (decpt == 9999) {
        decpt = -1;
    } else if (decpt <= -4 || decpt > ndigits) {
        FormatExponent(decpt - 1, exp);
        decpt = 1;
    } else if (decpt <= 0) {
        zeroes = 1 - decpt;
        decpt = 1;
    }

    if (sign && out != oute) {
        *out++ = '-';
    }

    for (; out != oute; ++out, --decpt) {
        if (decpt == 0 && s != se) {
            // no need in decimal point if there are no digits after it
            *out = '.';
        } else if (zeroes > 0) {
            // leading zeroes
            *out = '0';
            --zeroes;
        } else if (s != se) {
            *out = *s++;
        } else if (decpt > 0) {
            // trailing zeroes
            *out = '0';
        } else {
            break;
        }
    }

    for (const char* e = exp; *e && out != oute; ++e, ++out) {
        *out = *e;
    }

    return out - buf;
}

// Using Dtoa for float and double because it is faster than snprintf
// (snprintf for floats in freebsd is single threaded)
template <>
size_t ToStringImpl<double>(double d, char* buf, size_t len) {
    return Dtoa(d, buf, len, 4, 10);
}

template <>
size_t ToStringImpl<float>(float f, char* buf, size_t len) {
    return Dtoa(f, buf, len, 2, 6);
}
#else
double StrToD(const char* b, const char* e, char** se) {
    return StrToDSystem(b, e, se);
}
#endif

double StrToD(const char* b, char** se) {
    return StrToD(b, b + strlen(b), se);
}
