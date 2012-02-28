#include <stlport/cstdio>
#include <stlport/cstdlib>
#include <stlport/cstring>
#include <stlport/cctype>
#include <stlport/ctime>
#include <stlport/numeric>

#include "parser.h"


%%{

machine DateTimeParserCommon;

sp  = ' ';

action clear_int {
    I = 0;
    Dc = 0;
}

action update_int {
    I = I * 10 + (fc - '0');
    ++Dc;
}

int = (digit+)
    >clear_int
    $update_int;

int1 = digit
    >clear_int
    $update_int;

int2 = (digit digit)
    >clear_int
    $update_int;

int3 = (digit digit digit)
    >clear_int
    $update_int;

int4 = (digit digit digit digit)
    >clear_int
    $update_int;

int12 = (digit digit?)
    >clear_int
    $update_int;

int24 = ( digit digit ( digit digit )? )
    >clear_int
    $update_int;

# According to both RFC2822 and RFC2616 dates MUST be case-sensitive,
# but Andrey fomichev@ wants relaxed parser

month3 =
      'Jan'i %{ DateTimeFields.Month =  1; }
    | 'Feb'i %{ DateTimeFields.Month =  2; }
    | 'Mar'i %{ DateTimeFields.Month =  3; }
    | 'Apr'i %{ DateTimeFields.Month =  4; }
    | 'May'i %{ DateTimeFields.Month =  5; }
    | 'Jun'i %{ DateTimeFields.Month =  6; }
    | 'Jul'i %{ DateTimeFields.Month =  7; }
    | 'Aug'i %{ DateTimeFields.Month =  8; }
    | 'Sep'i %{ DateTimeFields.Month =  9; }
    | 'Oct'i %{ DateTimeFields.Month = 10; }
    | 'Nov'i %{ DateTimeFields.Month = 11; }
    | 'Dec'i %{ DateTimeFields.Month = 12; };

wkday   = 'Mon'i | 'Tue'i | 'Wed'i | 'Thu'i | 'Fri'i | 'Sat'i | 'Sun'i;
weekday = 'Monday'i | 'Tuesday'i | 'Wednesday'i | 'Thursday'i
        | 'Friday'i | 'Saturday'i | 'Sunday'i;

action set_second   { DateTimeFields.Second = I; }
action set_minute   { DateTimeFields.Minute = I; }
action set_hour     { DateTimeFields.Hour = I; }
action set_day      { DateTimeFields.Day = I; }
action set_month    { DateTimeFields.Month = I; }
action set_year     { DateTimeFields.SetLooseYear(I); }
action set_zone_utc { DateTimeFields.ZoneOffsetMinutes = 0; }

}%%

%%{

machine RFC822DateParser;

################# RFC 2822 3.3 Full Date ###################

include DateTimeParserCommon;

ws1 = (space+);
ws0 = (space*);
dow_spec = ( wkday ',' )?;

day  = int12 %set_day;
year = int24 %set_year;

# actually it must be from 0 to 23
hour = int2 %set_hour;

# actually it must be from 0 to 59
min  = int2 %set_minute;

# actually it must be from 0 to 59
sec  = int2 %set_second;

sec_spec  = ( ':' . sec )?;

# so called "military zone offset". I hardly believe someone uses it now, but we MUST respect RFc822
action set_mil_offset {
    char c = (char)toupper(fc);
    if (c == 'Z')
        DateTimeFields.ZoneOffsetMinutes = 0;
    else {
        if (c <= 'M') {
            // ['A'..'M'] \ 'J'
            if (c < 'J')
                DateTimeFields.ZoneOffsetMinutes = (i32)TDuration::Hours(c - 'A' + 1).Minutes();
            else
                DateTimeFields.ZoneOffsetMinutes = (i32)TDuration::Hours(c - 'A').Minutes();
        } else {
            // ['N'..'Y']
            DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(c - 'N' + 1).Minutes();
        }
    }
}

action set_digit_offset {
    DateTimeFields.ZoneOffsetMinutes = Sign * (i32)(TDuration::Hours(I / 100) + TDuration::Minutes(I % 100)).Minutes();
}

mil_zone = /[A-IK-Za-ik-z]/ $set_mil_offset;

# actions % were replaced with @ (when the script was migrated to ragel 5.24)
# because ragel 5.24 does not call to the % action if it  be called at the very end of string.
# it is a bug in ragel 5 because ragel 6.2 works correctly with % at the end of string.
# see http://www.complang.org/ragel/ChangeLog.

zone = 'UT' @{ DateTimeFields.ZoneOffsetMinutes = 0; }
    | 'GMT' @{ DateTimeFields.ZoneOffsetMinutes = 0; }
    | 'EST' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(5).Minutes();}
    | 'EDT' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(4).Minutes(); }
    | 'CST' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(6).Minutes();}
    | 'CDT' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(5).Minutes(); }
    | 'MST' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(7).Minutes();}
    | 'MDT' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(6).Minutes(); }
    | 'PST' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(8).Minutes();}
    | 'PDT' @{ DateTimeFields.ZoneOffsetMinutes = -(i32)TDuration::Hours(7).Minutes(); };

digit_offset = ('+' | '-') > { Sign = fc == '+' ? 1 : -1; } . int4 @set_digit_offset;

offset = ( zone | mil_zone | digit_offset );

rfc822datetime = ws0 . dow_spec . ws0 . day . ws1 . month3 . ws1 . year . ws1 . hour . ':' . min . sec_spec . ws1 . offset . ws0;

main := (rfc822datetime) $!{ return false; };

write data noerror;

}%%

TRfc822DateTimeParser::TRfc822DateTimeParser() {
    %% write init;
}

bool TRfc822DateTimeParser::ParsePart(const char* input, size_t len) {
    const char* p = input;
    const char* pe = input + len;

    %% write exec;
    return true;
}

%%{

machine ISO8601DateTimeParser;

include DateTimeParserCommon;

year = int4 @set_year;
month = int2 @set_month;
day = int2 @set_day;
hour = int2 @set_hour;
minute = int2 @set_minute;
second = int2 @set_second;
secondFrac = int @{
    ui32 us = I;
    for (int k = Dc; k < 6; ++k) {
        us *= 10;
    }
    DateTimeFields.MicroSecond = us;
};

zoneZ = [Zz] @set_zone_utc;
zoneOffset = space? . ('+' | '-') >{ Sign = fc == '+' ? 1 : -1; } . int2 @{ DateTimeFields.ZoneOffsetMinutes = Sign * (i32)TDuration::Hours(I).Minutes(); } . (':')? . (int2 @{ DateTimeFields.ZoneOffsetMinutes += I * Sign; })?;
zone = zoneZ | zoneOffset;

iso8601date = (year . '-' . month . '-' . day) | (year . month . day);
iso8601time = (hour . ':' . minute . (':' . second ('.' secondFrac)?)?) | (hour . minute . second?);

iso8601datetime = iso8601date . ([Tt ] . iso8601time . zone?)?;

main := (iso8601datetime) $!{ return false; };

write data noerror;

}%%

TIso8601DateTimeParser::TIso8601DateTimeParser() {
    %% write init;
}

bool TIso8601DateTimeParser::ParsePart(const char* input, size_t len) {
    const char* p = input;
    const char* pe = input + len;

    %% write exec;
    return true;
}

%%{

machine HttpDateTimeParser;

include DateTimeParserCommon;

################# RFC 2616 3.3.1 Full Date #################

time            = int2 %set_hour ':' int2 %set_minute ':' int2 %set_second;
date1           = int2 %set_day ' ' month3 ' ' int4 %set_year;
date2           = int2 %set_day '-' month3 '-' int2 %set_year;
date3           = month3 sp (int2 | sp int1) %set_day;

rfc1123_date    = wkday ',' sp date1 sp time sp 'GMT'i;
rfc850_date     = weekday ',' sp date2 sp time sp 'GMT'i;
asctime_date    = wkday sp date3 sp time sp int4 @set_year;
http_date       = (rfc1123_date | rfc850_date | asctime_date) @set_zone_utc;

}%%

%%{

machine HttpDateTimeParserStandalone;

include HttpDateTimeParser;

main := (http_date) $!{ return false; };

write data noerror;

}%%

THttpDateTimeParser::THttpDateTimeParser() {
    %% write init;
}

bool THttpDateTimeParser::ParsePart(const char* input, size_t len) {
    const char* p = input;
    const char* pe = input + len;

    %% write exec;
    return true;
}

%%{

machine X509ValidityDateTimeParser;

include DateTimeParserCommon;

################# X.509 certificate validity time (see rfc5280 4.1.2.5.*) #################

year = int2 @{ DateTimeFields.Year = (I < 50 ? I + 2000 : I + 1900); };
month = int2 @set_month;
day = int2 @set_day;
hour = int2 @set_hour;
minute = int2 @set_minute;
second = int2 @set_second;
zone = 'Z' @set_zone_utc;

main := year . month . day . hour . minute . second . zone;

write data noerror;

}%%

TX509ValidityDateTimeParser::TX509ValidityDateTimeParser() {
    %% write init;
}

bool TX509ValidityDateTimeParser::ParsePart(const char *input, size_t len) {
    const char *p = input;
    const char *pe = input + len;

    %% write exec;
    return true;
}

%%{

machine X509Validity4yDateTimeParser;

include DateTimeParserCommon;

year = int4 @{ DateTimeFields.Year = I; };
month = int2 @set_month;
day = int2 @set_day;
hour = int2 @set_hour;
minute = int2 @set_minute;
second = int2 @set_second;
zone = 'Z' @set_zone_utc;


main := year . month . day . hour . minute . second . zone;

write data noerror;

}%%

TX509Validity4yDateTimeParser::TX509Validity4yDateTimeParser() {
    %% write init;
}

bool TX509Validity4yDateTimeParser::ParsePart(const char *input, size_t len) {
    const char *p = input;
    const char *pe = input + len;

    %% write exec;
    return true;
}

TInstant TIso8601DateTimeParser::GetResult(TInstant defaultValue) const {
    return TDateTimeParserBase::GetResult(ISO8601DateTimeParser_first_final, defaultValue);
}

TInstant TRfc822DateTimeParser::GetResult(TInstant defaultValue) const {
    return TDateTimeParserBase::GetResult(RFC822DateParser_first_final, defaultValue);
}

TInstant THttpDateTimeParser::GetResult(TInstant defaultValue) const {
    return TDateTimeParserBase::GetResult(HttpDateTimeParserStandalone_first_final, defaultValue);
}

TInstant TX509ValidityDateTimeParser::GetResult(TInstant defaultValue) const {
    return TDateTimeParserBase::GetResult(X509ValidityDateTimeParser_first_final, defaultValue);
}

TInstant TX509Validity4yDateTimeParser::GetResult(TInstant defaultValue) const {
    return TDateTimeParserBase::GetResult(X509Validity4yDateTimeParser_first_final, defaultValue);
}

template<class TParser, class TResult>
static inline TResult Parse(const char* input, size_t len, TResult defaultValue) {
    TParser parser;
    if (!parser.ParsePart(input, len))
        return defaultValue;
    return parser.GetResult(defaultValue);
}

template<class TParser, class TResult>
static inline TResult ParseUnsafe(const char* input, size_t len) {
    TResult r = Parse<TParser, TResult>(input, len, TResult::Max());
    if (r == TResult::Max())
        throw TDateTimeParseException() << "error in datetime parsing";
    return r;
}

TInstant TInstant::ParseIso8601(const Stroka& input) {
    return ParseUnsafe<TIso8601DateTimeParser, TInstant>(~input, +input);
}

TInstant TInstant::ParseRfc822(const Stroka& input) {
    return ParseUnsafe<TRfc822DateTimeParser, TInstant>(~input, +input);
}

TInstant TInstant::ParseHttp(const Stroka& input) {
    return ParseUnsafe<THttpDateTimeParser, TInstant>(~input, +input);
}

TInstant TInstant::ParseX509Validity(const Stroka &input) {
    switch (+input) {
    case 13:
        return ParseUnsafe<TX509ValidityDateTimeParser, TInstant>(~input, 13);
    case 15:
        return ParseUnsafe<TX509Validity4yDateTimeParser, TInstant>(~input, 15);
    default:
        throw TDateTimeParseException();
    }
}


bool ParseRFC822DateTime(const char* input, time_t& utcTime) {
    return ParseRFC822DateTime(input, strlen(input), utcTime);
}

bool ParseISO8601DateTime(const char* input, time_t& utcTime) {
    return ParseISO8601DateTime(input, strlen(input), utcTime);
}

bool ParseHTTPDateTime(const char* input, time_t& utcTime) {
    return ParseHTTPDateTime(input, strlen(input), utcTime);
}

bool ParseX509ValidityDateTime(const char* input, time_t& utcTime) {
    return ParseX509ValidityDateTime(input, strlen(input), utcTime);
}


bool ParseRFC822DateTime(const char* input, size_t inputLen, time_t& utcTime) {
    try {
        utcTime = ParseUnsafe<TRfc822DateTimeParser, TInstant>(input, inputLen).TimeT();
        return true;
    } catch (const TDateTimeParseException&) {
        return false;
    }
}

bool ParseISO8601DateTime(const char* input, size_t inputLen, time_t& utcTime) {
    try {
        utcTime = ParseUnsafe<TIso8601DateTimeParser, TInstant>(input, inputLen).TimeT();
        return true;
    } catch (const TDateTimeParseException&) {
        return false;
    }
}

bool ParseHTTPDateTime(const char* input, size_t inputLen, time_t& utcTime) {
    try {
        utcTime = ParseUnsafe<THttpDateTimeParser, TInstant>(input, inputLen).TimeT();
        return true;
    } catch (const TDateTimeParseException&) {
        return false;
    }
}

bool ParseX509ValidityDateTime(const char* input, size_t inputLen, time_t& utcTime) {
    TInstant r;
    switch (inputLen) {
    case 13:
        r = Parse<TX509ValidityDateTimeParser, TInstant>(input, 13, TInstant::Max());
        break;
    case 15:
        r = Parse<TX509Validity4yDateTimeParser, TInstant>(input, 15, TInstant::Max());
        break;
    default:
        return false;
    }
    if (r == TInstant::Max())
        return false;
    utcTime = r.TimeT();
    return true;
}

%%{

machine TDurationParser;

include DateTimeParserCommon;


multiplier
    = '' #   >{ MultiplierPower =  6; } # work around Ragel bugs
    | 's'  @{ MultiplierPower =  6; }
    | 'ms' @{ MultiplierPower =  3; }
    | 'us' @{ MultiplierPower =  0; }
    | 'ns' @{ MultiplierPower = -3; }
    ;

integer = int @{ IntegerPart = I; };

fraction = '.' digit {1,6} >clear_int $update_int @{ FractionPart = I; FractionDigits = Dc; } digit*;

duration = integer fraction? multiplier;

main := (duration) $!{ return false; };

write data noerror;

}%%

TDurationParser::TDurationParser()
    : cs(0)
    , I(0)
    , Dc(0)
    , MultiplierPower(6)
    , IntegerPart(0)
    , FractionPart(0)
    , FractionDigits(0)
{
    %% write init;
}

bool TDurationParser::ParsePart(const char* input, size_t len) {
    const char* p = input;
    const char* pe = input + len;

    %% write exec;
    return true;
}

static inline ui64 Power(ui64 base, i32 power) {
    // Note: std::power is stlport extension
    return NStl::power(base, power);
}

TDuration TDurationParser::GetResult(TDuration defaultValue) const {
    if (cs < TDurationParser_first_final)
        return defaultValue;
    ui64 us = 0;
    us += IntegerPart * Power(10, MultiplierPower);
    us += FractionPart * Power(10, MultiplierPower - FractionDigits);
    return TDuration::MicroSeconds(us);
}
                                    

TDuration TDuration::Parse(const Stroka& input) {
    return ParseUnsafe<TDurationParser, TDuration>(~input, +input);
}

