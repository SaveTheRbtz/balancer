#pragma once

#include "stroka.h"
#include "utility.h"

template <typename TChar, typename TTraits = TCharTraits<TChar> >
class TStringBufImpl
    : public TFixedString<TChar, TTraits>
    , public TStringBase<TStringBufImpl<TChar, TTraits>, TChar, TTraits>
{
    typedef TStringBufImpl TdSelf;
    typedef TFixedString<TChar, TTraits> TBaseStr;
    typedef TStringBase<TdSelf, TChar, TTraits> TBase;
    typedef typename TTraits::TWhatString::TString TStroka;

public:
    typedef TChar char_type;
    typedef TTraits traits_type;

    inline TStringBufImpl(const TChar* data, size_t len) throw ()
        : TBaseStr(data, len)
    {
    }

    inline TStringBufImpl(const TChar* data) throw ()
        : TBaseStr(data, TBase::StrLen(data))
    {
    }

    inline TStringBufImpl(const TChar* beg, const TChar* end) throw ()
        : TBaseStr(beg, end)
    {
        YASSERT(beg <= end);
    }

    template <typename D, typename T>
    inline TStringBufImpl(const TStringBase<D, TChar, T>& str) throw ()
        : TBaseStr(str)
    {
    }

    template <typename T, typename A>
    inline TStringBufImpl(const NStl::basic_string<TChar, T, A>& str) throw ()
        : TBaseStr(str)
    {
    }

    inline TStringBufImpl() throw ()
        : TBaseStr()
    {
    }

    inline TStringBufImpl(const TBaseStr &src) throw ()
        : TBaseStr(src)
    {
    }

    inline TStringBufImpl(const TBaseStr &src, size_t pos, size_t n) throw ()
        : TBaseStr(src)
    {
        Skip(pos).Trunc(n);
    }

public: // required by TStringBase
    inline const TChar* data() const throw () {
        return Start;
    }

    inline size_t length() const throw () {
        return Length;
    }

public:
    void Clear() {
        *this = TdSelf();
    }

    bool IsInited() const throw () {
        return NULL != Start;
    }

public: // delimiter splitting and iteration
    inline bool SplitImpl(TChar delim, TdSelf& l, TdSelf& r) const throw () {
        const size_t pos = this->find(delim);

        if (pos != TBase::npos) {
            l = Head(pos);
            r = Tail(pos + 1);

            return true;
        }

        return false;
    }

    inline bool RSplitImpl(TChar delim, TdSelf& l, TdSelf& r) const throw () {
        const size_t pos = this->rfind(delim);

        if (pos != TBase::npos) {
            l = Head(pos);
            r = Tail(pos + 1);

            return true;
        }

        return false;
    }

    inline void Split(TChar delim, TdSelf& l, TdSelf& r) const throw () {
        // make a copy in case one of l/r is really *this
        if (!TdSelf(*this).SplitImpl(delim, l, r)) {
            l = *this;
            r = TdSelf();
        }
    }

    inline void RSplit(TChar delim, TdSelf& l, TdSelf& r) const throw () {
        // make a copy in case one of l/r is really *this
        if (!TdSelf(*this).RSplitImpl(delim, l, r)) {
            r = *this;
            l = TdSelf();
        }
    }

    inline void SplitAt(size_t pos, TdSelf & l, TdSelf & r) const throw () {
        // make a copy in case one of l/r is really *this
        const TdSelf tok = SubStr(pos);
        l = Head(pos);
        r = tok;
    }

    inline TdSelf After(TChar c) const throw () {
        TdSelf l, r;

        if (SplitImpl(c, l, r)) {
            return r;
        }

        return *this;
    }

    inline TdSelf Before(TChar c) const throw () {
        TdSelf l, r;

        if (SplitImpl(c, l, r)) {
            return l;
        }

        return *this;
    }

    // returns tail, including pos
    TdSelf SplitOffAt(size_t pos) {
        const TdSelf tok = SubStr(pos);
        Trunc(pos);
        return tok;
    }

    // returns head, tail includes pos
    TdSelf NextTokAt(size_t pos) {
        const TdSelf tok = Head(pos);
        Skip(pos);
        return tok;
    }

    TdSelf SplitOff(TChar delim) {
        TdSelf tok;
        Split(delim, *this, tok);
        return tok;
    }

    TdSelf RSplitOff(TChar delim) {
        TdSelf tok;
        RSplit(delim, tok, *this);
        return tok;
    }

    TdSelf NextTok(TChar delim) {
        TdSelf tok;
        Split(delim, tok, *this);
        return tok;
    }

    TdSelf RNextTok(TChar delim) {
        TdSelf tok;
        RSplit(delim, *this, tok);
        return tok;
    }

public: // string subsequences
    inline TdSelf& Chop(size_t shift) throw () {
        ChopImpl(shift);

        return *this;
    }

    inline TdSelf& Skip(size_t shift) throw () {
        Start += ChopImpl(shift);

        return *this;
    }

    // sets the start pointer to a position relative to the end
    inline TdSelf& RSeek(size_t len) throw () {
        if (Length > len) {
            Start += Length - len;
            Length = len;
        }

        return *this;
    }

    inline TdSelf& Trunc(size_t len) throw () {
        if (Length > len) {
            Length = len;
        }

        return *this;
    }

    inline TdSelf SubStr(size_t beg) const throw () {
        return TdSelf(*this).Skip(beg);
    }

    inline TdSelf SubStr(size_t beg, size_t len) const throw () {
        return SubStr(beg).Trunc(len);
    }

    inline TdSelf Head(size_t pos) const throw () {
        return TdSelf(*this).Trunc(pos);
    }

    inline TdSelf Tail(size_t pos) const throw () {
        return SubStr(pos);
    }

    inline TdSelf Last(size_t len) const throw () {
        return TdSelf(*this).RSeek(len);
    }

    inline TdSelf& operator+=(size_t shift) throw () {
        return Skip(shift);
    }

    inline TdSelf& operator++() throw () {
        return Skip(1);
    }

    inline TdSelf operator+(size_t shift) const throw () {
        return SubStr(shift);
    }

    // defined in a parent, but repeat for overload above
    inline size_t operator+() const throw () {
        return length();
    }

    TStroka ToString() const {
        return TStroka(Start, Length);
    }

private:
    inline size_t ChopImpl(size_t shift) throw () {
        if (shift > length())
            shift = length();
        Length -= shift;
        return shift;
    }

private:
    using TBaseStr::Start;
    using TBaseStr::Length;
};

typedef TStringBufImpl<char> TStringBuf;
typedef TStringBufImpl<wchar16> TWtringBuf;

static inline Stroka ToString(const TStringBuf& str) {
    return Stroka(str);
}

static inline Wtroka ToWtring(const TWtringBuf& wtr) {
    return Wtroka(wtr);
}

template<size_t size>
inline TStringBuf AsStringBuf(const char (&str)[size]) {
    return TStringBuf(str, size - 1);
}

#define STRINGBUF(x) AsStringBuf(x)
#define XSTRINGBUF(x) STRINGBUF(#x)
