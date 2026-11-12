#include "pch.h"
#include "server.h"
#include "irc.h"

#define DEFAULT_mIRC_PORT 6667

Channel channels[MAX_CHANNELS];
pthread_mutex_t channelMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]){
    if (argc <= 1) {
        printf("Usage: %s <IP_ADDRESS>\n", argv[0]);
        return -1;
    }

    memset(channels, 0, sizeof(channels));

    ServerSettings settings;
    settings = setupTCPServer(argv[1], DEFAULT_mIRC_PORT);
    
    runServer(&settings);

    close(settings.serverFd);
    return 0;
}
