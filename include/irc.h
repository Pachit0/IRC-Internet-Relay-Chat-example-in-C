#pragma once
#include <pthread.h>

#define SERVER_NAME "irc.myproject.local"

#define MAX_CLIENTS 10
#define MAX_CHANNELS 5
#define MAX_CHAN_NAME 50
#define MAX_NICK 32

enum PassTo {
    BROADCAST,
    USER,
    NONE
};

typedef struct {
    int sockfd;
    char nickname[MAX_NICK];
} ClientContext;

typedef struct {
    char name[MAX_CHAN_NAME];
    ClientContext clients[MAX_CLIENTS];
    int clientCount;
    int isActive;
} Channel;

extern Channel channels[MAX_CHANNELS];
extern pthread_mutex_t channelMutex;

void processCommand(ClientContext* client, char* line);
int findChannel(const char* name);
ClientContext* findUser(const char* name);
int joinChannel(const char* chanName, ClientContext* client);
void removeClientFromAllChannels(int clientFd);
void broadcastToChannel(const char* chanName, const char* message, int senderFd);
void sendToUser(const char* userName, const char* message, int senderFd);
