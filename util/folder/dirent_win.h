#pragma once

#include <util/system/defaults.h>

#ifdef _win_

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct DIR {
    HANDLE sh;
    WIN32_FIND_DATA wfd;
    char *fff_templ;
    int file_no;
    struct dirent *readdir_buf;
};

#define MAXNAMLEN       (MAX_PATH - 1)
#define MAXPATHLEN      MAX_PATH
#define DT_DIR           4
#define DT_REG           8
#define DT_LNK          10
#define DT_UNKNOWN       0

struct dirent {
    ui32 d_fileno;            /* file number of entry */
    ui16 d_reclen;            /* length of this record */
    ui8  d_type;              /* file type */
    ui8  d_namlen;            /* length of string in d_name */
    char d_name[MAXNAMLEN + 1];  /* name must be no longer than this */
};

struct DIR *opendir(const char *dirname);
int closedir(struct DIR *dir);
int readdir_r(struct DIR *dir, struct dirent *entry, struct dirent **result);
struct dirent *readdir(struct DIR *dir);
void rewinddir(struct DIR *dir);

#ifdef  __cplusplus
}
#endif

#endif //_win_
