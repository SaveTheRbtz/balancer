#include <stdlib.h>

#include <util/system/platform.h>

#if defined(_solaris_)
#   include <stdlib.h>
#elif defined(_darwin_)
#   include <mach-o/dyld.h>
#elif defined(_win_)
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <io.h>
#   include <windows.h>
#elif defined(_linux_)
#   include <unistd.h>
#elif defined(_freebsd_)
#   include <string.h>
#   include <sys/types.h>  // for u_int not defined in sysctl.h
#   include <sys/sysctl.h>
#   include <unistd.h>
#endif

#include <util/folder/dirut.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/pipe.h>
#include <util/string/cast.h>
#include <util/system/filemap.h>

#include "execpath.h"

#if defined(_linux_) || defined(_freebsd_)
static Stroka ReadLink(const Stroka& path) {
    TTempBuf buf;
    for (;;) {
        ssize_t r = readlink(~path, buf.Data(), buf.Size());
        if (r < 0)
            ythrow yexception() << "can't read link " << path;
        if (r < (ssize_t)buf.Size())
            return Stroka(buf.Data(), r);
        buf = TTempBuf(buf.Size() * 2);
    }
}
#endif

#if defined(_freebsd_)

static inline bool GoodPath(const Stroka& path) {
    return path.find('/') != Stroka::npos;
}

static inline int FreeBSDSysCtl(int* mib, size_t mibSize, TTempBuf& res) {
    for (size_t i = 0; i < 2; ++i) {
        size_t cb = res.Size();
        if (sysctl(mib, mibSize, res.Data(), &cb, NULL, 0) == 0) {
            res.Proceed(cb);
            return 0;
        } else if (errno == ENOMEM) {
            res = TTempBuf(cb);
        } else {
            return errno;
        }
    }
    return errno;
}

static inline Stroka FreeBSDGetExecPath() {
    int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    TTempBuf buf;
    int r = FreeBSDSysCtl(mib, ARRAY_SIZE(mib), buf);
    if (r == 0) {
        return Stroka(buf.Data(), buf.Filled()-1);
    } else if (r == ENOTSUP) { // older FreeBSD version
        return ReadLink("/proc/" + ToString(getpid()) + "/file");
    } else {
        return Stroka();
    }
}

static inline Stroka FreeBSDGetArgv0() {
    int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_ARGS, getpid() };
    TTempBuf buf;
    int r = FreeBSDSysCtl(mib, ARRAY_SIZE(mib), buf);
    if (r == 0) {
        return Stroka(buf.Data());
    } else if (r == ENOTSUP) {
        return Stroka();
    } else {
        ythrow yexception() << "FreeBSDGetArgv0() failed: " << LastSystemErrorText();
    }
}

static inline bool FreeBSDGuessExecPath(const Stroka& guessPath, Stroka& execPath) {
    if (isexist(~guessPath)) {
        // now it should work for real
        execPath = FreeBSDGetExecPath();
        if (RealPath(execPath) == RealPath(guessPath)) {
            return true;
        }
    }
    return false;
}

static inline bool FreeBSDGuessExecBasePath(const Stroka& guessBasePath, Stroka& execPath) {
    return FreeBSDGuessExecPath(Stroka(guessBasePath) + "/" + getprogname(), execPath);
}

#endif

static Stroka GetExecPathImpl() {
#if defined(_solaris_)
    return execname();
#elif defined(_darwin_)
    TTempBuf execNameBuf;
    for (size_t i = 0; i < 2; ++i) {
        uint32_t bufsize = (uint32_t)execNameBuf.Size();
        int r = _NSGetExecutablePath(execNameBuf.Data(), &bufsize);
        if (r == 0) {
            return execNameBuf.Data();
        } else if (r == -1) {
            execNameBuf = TTempBuf(bufsize);
        }
    }
    ythrow yexception() << "GetExecPathImpl() failed";
#elif defined(_win_)
    TTempBuf execNameBuf;
    for (;;) {
        DWORD r = GetModuleFileName(NULL, execNameBuf.Data(), execNameBuf.Size());
        if (r == execNameBuf.Size()) {
            execNameBuf = TTempBuf(execNameBuf.Size() * 2);
        } else if (r == 0) {
            ythrow yexception() << "GetExecPathImpl() failed: " << LastSystemErrorText();
        } else {
            return execNameBuf.Data();
        }
    }
#elif defined(_linux_)
    return ReadLink("/proc/" + ToString(getpid()) + "/exe");
    // TODO(yoda): check if the filename ends with " (deleted)"
#elif defined(_freebsd_)
    Stroka execPath = FreeBSDGetExecPath();
    if (GoodPath(execPath)) {
        return execPath;
    }
    if (FreeBSDGuessExecPath(FreeBSDGetArgv0(), execPath)) {
        return execPath;
    }
    if (FreeBSDGuessExecPath(getenv("_"), execPath)) {
        return execPath;
    }
    if (FreeBSDGuessExecBasePath(getenv("PWD"), execPath)) {
        return execPath;
    }
    if (FreeBSDGuessExecBasePath(GetCwd(), execPath)) {
        return execPath;
    }

    ythrow yexception() << "can not resolve exec path";
#else
#       error dont know how to implement GetExecPath on this platform
#endif
}

struct TExecPathHolder {
    inline TExecPathHolder() {
        ExecPath = GetExecPathImpl();
    }

    Stroka ExecPath;
};

const Stroka& GetExecPath() {
    return Singleton<TExecPathHolder>()->ExecPath;
}

TMappedFile* OpenExecFile() {
    Stroka path = GetExecPathImpl();
    THolder<TMappedFile> mf(new TMappedFile(~path));

    Stroka path2 = GetExecPathImpl();
    if (path != path2)
        ythrow yexception() << "OpenExecFile(): something happened to the binary while we were opening it";

    return mf.Release();
}
