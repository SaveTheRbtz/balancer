#include "init.h"

#include <cstdio>
#include <cstdlib>

#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/singleton.h>

class TNetworkInit {
    public:
        inline TNetworkInit() {
#ifndef ROBOT_SIGPIPE
            signal(SIGPIPE, SIG_IGN);
#endif

#if defined(_win_)
            #pragma comment(lib, "ws2_32.lib")
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            YASSERT(!result);
            if (result) {
                exit(-1);
            }
#endif
        }
};

void InitNetworkSubSystem() {
    (void)Singleton<TNetworkInit>();
}
