#include "conf.h"

#include <util/string/util.h>
#include <util/string/split.h>
#include <util/stream/file.h>
#include <util/logger/all.h>
#include <util/generic/algorithm.h>

void TYandexConfig::Clear()
{
    delete [] FileData;
    FileData = 0;
    CurrentMemoryPtr = 0;
    Len = 0;
    while (!CurSections.empty())
        CurSections.pop();
    for (size_t i = 0; i < AllSections.size(); i++) {
        if (AllSections[i]->Owner)
            delete AllSections[i]->Cookie;
        delete AllSections[i];
    }
    AllSections.clear();
    Errors.clear();
    EndLines.clear();
    ConfigPath.remove();
}

void TYandexConfig::PrintErrors(TLog* Log)
{
    size_t sz = Errors.size();
    if (sz) {
        Log->AddLog("Processing of \'%s\':\n", ~ConfigPath);
        for (size_t i = 0; i < sz; i++)
            *Log << Errors[i];
        Errors.clear();
    }
}

void TYandexConfig::PrintErrors(Stroka& Err)
{
    size_t sz = Errors.size();
    if (sz) {
        char buf[512];
        snprintf(buf, 512, "Processing of \'%s\':\n", ~ConfigPath);
        Err += buf;
        for (size_t i = 0; i < sz; i++)
            Err += Errors[i];
        Errors.clear();
    }
}

void TYandexConfig::ReportError(const char* ptr, const char* err, bool warning)
{
    if (ptr) {
        char buf[1024];
        yvector<const char*>::iterator I = UpperBound(EndLines.begin(), EndLines.end(), ptr);
        if (I == EndLines.end())
            I = EndLines.end() - 1;
        int line = int(I - EndLines.begin());
        I--;
        int col = int(ptr - (*I));
        if (warning)
            snprintf(buf, 1024, "Warning at line %d, col %d: %s.\n", line, col, err);
        else
            snprintf(buf, 1024, "Error at line %d, col %d: %s.\n", line, col, err);
        Errors.push_back(buf);
    } else
        Errors.push_back(err);
}

void TYandexConfig::ReportError(const char* ptr, bool warning, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[512];
    vsnprintf(buf, 512, format, args);
    ReportError(ptr, buf, warning);
}

bool TYandexConfig::Read(const char* path)
{
    assert(FileData == 0);
    ConfigPath = path;
    //read the file to memory
    TFile doc(path, OpenExisting|RdOnly);
    if (!doc.IsOpen()) {
        Errors.push_back(Stroka("can't open file ") + path + "\n");
        return false;
    }
    Len = (ui32)doc.GetLength();
    FileData = new char[Len + 1];
    doc.Load(FileData, Len);
    FileData[Len] = 0;
    doc.Close();
    return PrepareLines();
}

bool TYandexConfig::ReadMemory(const char* buffer) {
    assert(FileData == 0);
    assert(buffer);
    Len = (ui32)strlen(buffer);
    FileData = new char[Len + 1];
    memcpy(FileData, buffer, Len);
    FileData[Len] = 0;
    return PrepareLines();
}

bool TYandexConfig::PrepareLines()
{
    //scan line breaks
    EndLines.push_back(FileData-1);
    CurrentMemoryPtr = FileData;
    while (*CurrentMemoryPtr) { // Are you in a great hurry? I am not... :-)
        if (iscntrl((unsigned char)*CurrentMemoryPtr) && !isspace((unsigned char)*CurrentMemoryPtr)) {
            ReportError(CurrentMemoryPtr, "it's a binary file");
            return false;
        }
        if (*CurrentMemoryPtr++ == '\n')
            EndLines.push_back(CurrentMemoryPtr-1);
    }
    EndLines.push_back(CurrentMemoryPtr);

    // convert simple comments inceptive with '#' or '!' or ';' to blanks
    ProcessComments();

    //convert the XML comments to blanks
    char* endptr = 0;
    CurrentMemoryPtr = strstr(FileData, "<!--");
    while (CurrentMemoryPtr != 0) {
        endptr = strstr(CurrentMemoryPtr, "-->");
        if (endptr) {
            endptr += 3;
            while (CurrentMemoryPtr != endptr)
                *CurrentMemoryPtr++ = ' ';
            CurrentMemoryPtr = strstr(endptr, "<!--");
        } else {
            ReportError(CurrentMemoryPtr, "unclosed comment");
            return false;
        }
    }
    return true;
}

bool TYandexConfig::ParseMemory(const char* buffer, bool process_directives)
{
    if (!ReadMemory(buffer))
        return false;
    return ProcessRoot(process_directives);
}

bool TYandexConfig::Parse(const char* path, bool process_directives)
{
    if (!Read(path))
        return false;
    return ProcessRoot(process_directives);
}

bool TYandexConfig::ProcessRoot(bool process_directives)
{
    CurrentMemoryPtr = FileData;
    // Add the unnamed root section
    assert(AllSections.empty());
    AllSections.push_back(new Section);
    AllSections.back()->Parent = AllSections.back();
    if (!OnBeginSection(*AllSections.back()))
        return false;
    CurSections.push(AllSections.back());
    bool ret = ProcessAll(process_directives) && OnEndSection(*CurSections.top());
    CurSections.pop();
    while (!CurSections.empty()) {
        // There are some not closed main sections.
        OnEndSection(*CurSections.top());
        CurSections.pop();
    }
    return ret;
}

bool TYandexConfig::FindEndOfSection(const char* SecName, const char* begin, char*& endsec, char*& endptr)
{
    // find "</SecName" and set '<' and '>' to '\0'
    char* p = (char*)begin;
    char* EndName = (char*)alloca(strlen(SecName) + 3);
    *EndName = '<';
    *(EndName+1) = '/';
    strcpy(EndName+2, SecName);
    while (p && p < FileData + Len) {
        p = strstr(p, EndName);
        if (p == 0) {
            ReportError(SecName, "mismatched section");
            return false;
        }
        endsec = p;
        p += strlen(SecName) + 2;
        if (*p != '>' && !isspace((unsigned char)*p))
            continue; // it's a prefix but not the required section-end
        endptr = strchr(p, '>');
        if (endptr == 0) {
            ReportError(p, "mismatched \'<\'");
            return false;
        }
        *endptr = 0;
        *endsec++ = 0;
        *endsec++ = 0;
        p = endptr-1;
        while (p > endsec && isspace((unsigned char)*p))
            *p-- = 0;
        return true;
    }
    ReportError(SecName, "mismatched section");
    return false;
}

bool TYandexConfig::ParseSection(const char* SecName, const char* idname, const char* idvalue)
{
    assert(FileData); // Call Read() firstly
    size_t slen = strlen(SecName);
    CurrentMemoryPtr = FileData;

    assert(AllSections.empty());
    AllSections.push_back(new Section);
    AllSections.back()->Parent = AllSections.back();
    if (!OnBeginSection(*AllSections.back()))
        return false;
    CurSections.push(AllSections.back());

    bool ret = false;
    while (CurrentMemoryPtr < FileData + Len) {
        // May be *CurrentMemoryPtr == 0 if FileData has been parsed.
        while (*CurrentMemoryPtr++ != '<' && CurrentMemoryPtr < FileData + Len) {
        }
        if (strnicmp(CurrentMemoryPtr, SecName, slen) == 0) {
            char* p = CurrentMemoryPtr + slen;
            char* endptr = strchr(p, '>');
            if (endptr == 0)
                continue; // a section may be parsed
            if (*p != '>' && *p != '/' && !isspace((unsigned char)*p))
                continue; // required section must match the name and may not be parsed
            //parse now
            *endptr = 0;
            bool endnow = false;
            p = endptr-1;
            if (*p == '/') {
                *p-- = 0;
                endnow = true;
            }
            while (isspace((unsigned char)*p))
                *p-- = 0;
            char *body = endptr + 1, *endsec = 0;
            if (!ProcessBeginSection())
                break; // false
            if (!endnow) {
                if (!FindEndOfSection(CurSections.top()->Name, body, endsec, endptr))
                    break; // false
            }
            if (idname && idvalue) {
                SectionAttrs::iterator I = CurSections.top()->Attrs.find(idname);
                if (I != CurSections.top()->Attrs.end()) {
                    if (stricmp((*I).second, idvalue) != 0) {
                        CurrentMemoryPtr = endptr + 1;
                        CurSections.pop();
                        assert(AllSections.size() == 2);
                        Section* Last = AllSections.back();
                        assert(Last->Parent->Next == NULL);
                        assert(Last->Parent->Child == Last);
                        assert(Last->Next == NULL);
                        Last->Parent->Child = NULL;
                        delete Last;
                        AllSections.pop_back();
                        continue;
                    }
                } else {
                    if (*idvalue != 0) {
                        CurrentMemoryPtr = endptr + 1;
                        CurSections.pop();
                        assert(AllSections.size() == 2);
                        Section* Last = AllSections.back();
                        assert(Last->Parent->Next == NULL);
                        assert(Last->Parent->Child == Last);
                        assert(Last->Next == NULL);
                        Last->Parent->Child = NULL;
                        delete Last;
                        AllSections.pop_back();
                        continue;
                    }
                }
            }
            if (!OnBeginSection(*CurSections.top()))
                break; // false
            if (!endnow) {
                CurrentMemoryPtr = body;
                if (!ProcessAll(true))
                    break; // false
                CurrentMemoryPtr = endsec;
            }
            if (!OnEndSection(*CurSections.top()))
                break; // false
            if (!ProcessEndSection())
                break; // false
            CurrentMemoryPtr = endptr + 1;
            ret = true;
            break; // section found and processed
        }
    }
    CurSections.pop();
    while (!CurSections.empty()) {
        OnEndSection(*CurSections.top());
        CurSections.pop();
    }
    return ret;
}

// Parse some chunk of memory ended by \0
bool TYandexConfig::ProcessAll(bool process_directives)
{
    char* endptr;
    while (CurrentMemoryPtr && *CurrentMemoryPtr) {
        switch (*CurrentMemoryPtr) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                CurrentMemoryPtr++;
                break;
            case '>':
                ReportError(CurrentMemoryPtr, "mismatched \'>\'");
                return false;
                break;
            //It is not XML. We need (\n[\n\r\t ]*<) for the section started.
            case '<':
                {
                endptr = strchr(CurrentMemoryPtr, '>');
                if (endptr == 0) {
                    ReportError(CurrentMemoryPtr, "mismatched \'<\'");
                    return false;
                }
                *endptr = 0;
                char* p = CurrentMemoryPtr+1;
                if (*p != '/' && !isalpha(*p)) {
                    ReportError(p, "invalid character");
                    return false;
                }
                bool endnow = false;
                p = endptr-1;
                if (*p == '/') {
                    *p-- = 0;
                    endnow = true;
                }
                while (isspace((unsigned char)*p))
                    *p-- = 0;
                *CurrentMemoryPtr++ = 0;
                if (*CurrentMemoryPtr == '/') {
                    *CurrentMemoryPtr++ = 0;
                    if (!OnEndSection(*CurSections.top()))
                        return false;
                    if (!ProcessEndSection())
                        return false;
                } else {
                    if (!ProcessBeginSection())
                        return false;
                    if (!OnBeginSection(*CurSections.top()))
                        return false;
                }
                if (endnow) {
                    if (!OnEndSection(*CurSections.top()))
                        return false;
                    if (!ProcessEndSection())
                        return false;
                }
                CurrentMemoryPtr = endptr + 1;
                }
                break;
            default:
                if (process_directives && CurSections.top()->Cookie) {
                    if (!ProcessDirective())
                        return false;
                } else {
                    CurrentMemoryPtr = strchr(CurrentMemoryPtr, '\n');
                    if (!CurrentMemoryPtr)
                        return true; // the end of file
                }
                break;
        }
    }
    return true;
}

void TYandexConfig::ProcessLineBreak(char*& LineBreak, char toChange)
{
    assert(*LineBreak == '\n');
    assert(toChange == ' ' || toChange == 0);
    if (toChange == 0) {
        char* p = LineBreak - 1;
        while ((*p == ' ' || *p == '\r' || *p == '\t') && p >= FileData)
            *p-- = 0;
    }
    *LineBreak++ = toChange;
}

// convert simple comments inceptive with '#' or '!' or ';' to blanks
void TYandexConfig::ProcessComments()
{
    assert(FileData); // Call Read() firstly
    char* endptr = FileData;
    while (true) {
        //process the leading blanks for the next
        endptr += strspn(endptr, " \t\r");

        //process the comment-line
        if (*endptr == '!' || *endptr == '#' || *endptr == ';') {
            while (*endptr != 0 && *endptr != '\n')
                *endptr++ = ' ';
            if (*endptr == '\n') {
                endptr++;
                continue;
            } else // may be the last line in file
                break;
        }

        //process the regular line
        endptr = strchr(endptr, '\n');
        if (endptr)
            endptr++;
        else // may be the last line in file
            break;
    }
}

bool TYandexConfig::ProcessDirective()
{
    char* endptr = CurrentMemoryPtr;

    //find the end of the directive
    while (true) {
        //process the leading blanks for the next
        endptr += strspn(endptr, " \t\r");

        //process the blank line
        if (*endptr == '\n') {
            ProcessLineBreak(endptr, ' ');
            continue;
        }

        //process the regular line
        endptr = strchr(endptr, '\n');
        if (!endptr) // may be the last line in file
            break;
        //may be continue at the next line
        char* p = endptr-1;
        while ((*p == ' ' || *p == '\r' || *p == '\t') && p > FileData)
            p--;
        if (*p == '\\') {
            *p = ' ';
            ProcessLineBreak(endptr, ' ');
        } else {
            ProcessLineBreak(endptr, 0);
            break;
        }
    }
    assert(endptr == 0 || *endptr == 0 || *(endptr-1) == 0);

    //split the directive into key and value
    char* args = CurrentMemoryPtr;
    args += strcspn(CurrentMemoryPtr, " \t\r=:");
    if (*args) {
        bool olddelimiter = (*args == ':' || *args == '=');
        *args++ = 0;
        args += strspn(args, " \t\r");
        if ((*args == ':' || *args == '=') && !olddelimiter) {
            args++;
            args += strspn(args, " \t\r");
        }
    }

    //add the directive at last
    assert(!CurSections.empty());
    Section* sec = CurSections.top();
    if (!AddKeyValue(*sec, CurrentMemoryPtr, args))
        return false;

    CurrentMemoryPtr = endptr;
    return true;
}

void TYandexConfig::AddSection(Section* sec)
{
    assert(sec && sec->Parent);
    if (sec->Parent->Child == 0)
        sec->Parent->Child = sec;
    else {
        Section** pNext = &sec->Parent->Child->Next;
        while (*pNext)
            pNext = &(*pNext)->Next;
        *pNext = sec;
    }
    AllSections.push_back(sec);
}

bool TYandexConfig::ProcessBeginSection()
{
    assert(!CurSections.empty());
    Section* sec = new Section;
    sec->Parent = CurSections.top();
    AddSection(sec);
    char* endptr = CurrentMemoryPtr;
    endptr += strcspn(endptr, " \t\r\n");
    if (endptr && *endptr)
        *endptr++ = 0;
    AllSections.back()->Name = CurrentMemoryPtr;

    //find the attributes
    const char* AttrName = 0;
    bool EqPassed = false;
    while (endptr && *endptr) {
        switch (*endptr) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                endptr++;
                break;
            case '=':
                if (!AttrName || EqPassed) {
                    ReportError(endptr, "invalid character");
                    return false;
                } else {
                    EqPassed = true;
                    *endptr++ = 0;
                }
                break;
            case '\"':
            case '\'':
            case '`':
                if (!AttrName || !EqPassed) {
                    ReportError(endptr, "invalid character");
                    return false;
                }
                {
                char* endattr = strchr(endptr+1, *endptr);
                if (!endattr) {
                    ReportError(endptr, "mismatched character");
                    return false;
                }
                *endattr++ = 0;
                AllSections.back()->Attrs[AttrName] = endptr+1;
                AttrName = 0;
                EqPassed = false;
                endptr = endattr;
                }
                if (!(*endptr==0 || isspace((unsigned char)*endptr))) {
                    ReportError(endptr, "invalid character");
                    return false;
                }
                break;
            default:
                if (AttrName || EqPassed) {
                    ReportError(endptr, "invalid character");
                    return false;
                }
                AttrName = endptr;
                endptr += strcspn(endptr, " \t\r\n=");
                if (*endptr == 0) {
                    ReportError(AttrName, "invalid characters");
                    return false;
                }
                if (*endptr == '=')
                    EqPassed = true;
                *endptr++ = 0;
                break;
        }
    }
    CurSections.push(AllSections.back());
    return true;
}

bool TYandexConfig::ProcessEndSection()
{
    assert(!CurSections.empty());
    Section* sec = CurSections.top();
    if (sec->Name && CurrentMemoryPtr && strcmp(sec->Name, CurrentMemoryPtr) != 0) {
        ReportError(CurrentMemoryPtr, "mismatched open element");
        return false;
    }
    CurSections.pop();
    return true;
}

bool TYandexConfig::AddKeyValue(Section& sec, const char* key, const char* value)
{
    assert(sec.Cookie);
    if (!sec.Cookie->AddKeyValue(key, value)) {
        if (*sec.Name)
            ReportError(key, true, "section \'%s\' does not allow directive \'%s\'. The directive will be ignored", sec.Name, key);
        else
            ReportError(key, true, "directive \'%s\' not allowed here. It will be ignored", key);
    }
    return true;
}

bool TYandexConfig::OnBeginSection(Section& secnew)
{
    if (*secnew.Name) {
        ReportError(secnew.Name, false, "section \'%s\' not allowed here", secnew.Name);
        return false;
    }
    return true;
}

bool TYandexConfig::OnEndSection(Section& sec)
{
    if (sec.Cookie) {
        if (!sec.Cookie->CheckOnEnd(*this, sec)) {
            if (sec.Owner) {
                delete sec.Cookie;
                sec.Cookie = 0;
            }
        }
    }
    return true;
}

TYandexConfig::Section* TYandexConfig::GetFirstChild(const char* Name, TYandexConfig::Section* CurSection /*= NULL*/)
{
    if (CurSection == NULL)
        CurSection = GetRootSection();
    CurSection = CurSection->Child;
    while (CurSection) {
        if (CurSection->Parsed()) {
            if (stricmp(CurSection->Name, Name) == 0)
                break;
        }
        CurSection = CurSection->Next;
    }
    return CurSection;
}

static void PrintSectionConfig(const TYandexConfig::Section* section, TOutputStream& os) {
    if (section == 0 || !section->Parsed())
           return;
    bool hasName = section->Name && *section->Name;
    if (hasName) {
       os << "<" << section->Name;
       for (TYandexConfig::SectionAttrs::const_iterator iter = section->Attrs.begin(); iter != section->Attrs.end(); iter++) {
           os << " " << iter->first << "=\"" << iter->second << "\"";
       }
       os << ">\n";
    }
    for (TYandexConfig::Directives::const_iterator iter = section->GetDirectives().begin(); iter != section->GetDirectives().end(); iter++) {
        if (iter->second != 0 && *iter->second && !IsSpace(iter->second)) {
           os << iter->first;
           os << " " << iter->second << "\n" ;
        }
    }
    if (section->Child) {
       PrintSectionConfig(section->Child, os);
    }
    if (hasName)
        os << "</" << section->Name << ">\n";

    if (section->Next) {
        PrintSectionConfig(section->Next, os);
    }
}

void TYandexConfig::PrintConfig(TOutputStream& os) const {
    const Section* sec = GetRootSection();
    return PrintSectionConfig(sec, os);
}


//*********************************************************************************************
bool TYandexConfig::Directives::CheckOnEnd(TYandexConfig& , TYandexConfig::Section&)
{
    return true;
}

bool TYandexConfig::Directives::AddKeyValue(const char* key, const char* value)
{
    iterator I = find(key);
    if (I == end()) {
        if (strict)
            return false;
        else
            I = insert(value_type(key, 0)).first;
    }
    (*I).second = value;
    return true;
}

bool TYandexConfig::Directives::GetValue(const char* key, Stroka& value) const
{
    const_iterator I = find(key);

    if (I == end()) {
        if (strict) {
            ythrow yexception() << "key " <<  key << " not declared";
        }

        return false;
    }

    if ((*I).second == 0) {
        return false;
    }

    value = (*I).second;

    return true;
}

bool TYandexConfig::Directives::GetValue(const char* key, bool& value) const
{
    Stroka tmp;

    if (GetValue(key, tmp)) {// istrue won't return true on empty strings anymore
        value = !tmp || istrue(tmp);

        return true;
    }

    return false;
}

void TYandexConfig::Directives::FillArray(const char* key, yvector<Stroka>& values) const
{
    const_iterator I = find(key);

    if (I == end()) {
        return;
    }

    if ((*I).second != 0) {
        Split((*I).second, " ,\t\r", values);
    }
}

void TYandexConfig::Directives::Clear()
{
    if (strict) {
        clear();
    } else {
        for (iterator I = begin(); I != end(); ++I)
            (*I).second = 0;
    }
}

TYandexConfig::TSectionsMap TYandexConfig::Section::GetAllChildren() const {
    TSectionsMap result;
    if (Child == 0)
        return result;
    Section* curSection = Child;
    while (curSection) {
        result.insert(ymultimap<stroka, Section*>::value_type(curSection->Name, curSection));
        curSection = curSection->Next;
    }
    return result;
}

