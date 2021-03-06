#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/singleton.h>

//! converts an URL from windows-1251 to utf-8 and encodes it according to RFC1738 (see URL Character Encoding)
//! @param pszRes       destination buffer
//! @param nResBufSize  size of buffer
//! @param pszUrl       URL to be normalized
//! @return @c false if buffer is not long enough to contain normalized URL
bool NormalizeUrl(char *pszRes, size_t nResBufSize, const char *pszUrl, size_t pszUrlSize = Stroka::npos);

//! converts an URL from windows-1251 to utf-8 and encodes it according to RFC1738 (see URL Character Encoding)
//! @param url          URL to be normalized
//! @return normalized URL
//! @throw yexception if the URL cannot be normalized
Stroka NormalizeUrl(const TStringBuf& url);

Wtroka NormalizeUrl(const TWtringBuf& url);

Stroka StrongNormalizeUrl(const TStringBuf& url);

size_t GetHttpPrefixSize(const char* url);
size_t GetHttpPrefixSize(const wchar16* url);

size_t GetHttpPrefixSize(const TStringBuf& url);
size_t GetHttpPrefixSize(const TWtringBuf& url);

/** BEWARE of TStringBuf! You can not use operator ~ or c_str() like in Stroka
    !!!!!!!!!!!! */

//! removes protocol prefixes 'http://' and 'https://' from given URL
//! @note if URL has no prefix or some other prefix the function does nothing
//! @param szUrl    URL from which the prefix should be removed
//! @return a new URL without protocol prefix
TStringBuf CutHttpPrefix(const TStringBuf& url);
TWtringBuf CutHttpPrefix(const TWtringBuf& url);
TStringBuf CutSchemePrefix(const TStringBuf& url);

//! adds specified scheme prefix if URL has no scheme
//! @note if URL has scheme prefix already the function returns unchanged URL
Stroka AddSchemePrefix(const Stroka& url, Stroka scheme = "http");

TStringBuf GetHost(const TStringBuf& url);
TStringBuf GetHostAndPort(const TStringBuf& url);

TStringBuf GetPathAndQuery(const TStringBuf& url);
/**
 * Extracts host from url and cuts http(https) protocol prefix and port if any.
 * @param[in] url   any URL
 * @return          host without port and http(https) prefix.
 */
TStringBuf GetOnlyHost(const TStringBuf& url);
TStringBuf GetZone(const TStringBuf& host);
TStringBuf CutWWWPrefix(const TStringBuf& url);
TStringBuf GetDomain(const TStringBuf &host); // should not be used

size_t NormalizeUrlName(char* dest, const TStringBuf& source, size_t dest_size);
size_t NormalizeHostName(char* dest, const TStringBuf& source, size_t dest_size, ui16 defport = 80);

const char** GetTlds();

bool IsTld(TStringBuf s);
bool InTld(TStringBuf s);
