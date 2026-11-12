#include "pch.h"
#include "irc.h"

void processCommand(ClientContext* client, char* line) {
    char reply[512];

    if (strncmp(line, "CAP LS", 6) == 0) {
        snprintf(reply, sizeof(reply), ":%s CAP * LS :multi-prefix cap-notify\r\n", SERVER_NAME);
        send(client->sockfd, reply, strlen(reply), 0);
    }
    else if (strncmp(line, "CAP REQ", 7) == 0) {
        snprintf(reply, sizeof(reply), ":%s CAP * ACK :multi-prefix\r\n", SERVER_NAME);
        send(client->sockfd, reply, strlen(reply), 0);
    }
    else if (strncmp(line, "CAP END", 7) == 0) {
        snprintf(reply, sizeof(reply), ":%s 001 %s :Welcome to the Internet Relay Network, %s!\r\n",
                 SERVER_NAME, client->nickname, client->nickname);
        send(client->sockfd, reply, strlen(reply), 0);
    }
    else if (strncmp(line, "NICK ", 5) == 0) {
        sscanf(line + 5, "%31s", client->nickname);
    }
    else if (strncmp(line, "USER ", 5) == 0) {
        // just so the server handles this command as well.
    }
    else if (strncmp(line, "PING ", 5) == 0) {
        snprintf(reply, sizeof(reply), ":%s PONG %s :%s\r\n", SERVER_NAME, SERVER_NAME, line + 5);
        send(client->sockfd, reply, strlen(reply), 0);
    }
    else if (strncmp(line, "JOIN ", 5) == 0) {
        char chanName[MAX_CHAN_NAME];
        if (sscanf(line + 5, "%49s", chanName) == 1 && chanName[0] == '#') {
            if (joinChannel(chanName, client) != -1) {
                snprintf(reply, sizeof(reply), ":%s!%s@localhost JOIN :%s\r\n",
                         client->nickname, client->nickname, chanName);
                send(client->sockfd, reply, strlen(reply), 0);
                broadcastToChannel(chanName, reply, client->sockfd);

                snprintf(reply, sizeof(reply), ":%s 332 %s %s :Welcome to %s\r\n",
                         SERVER_NAME, client->nickname, chanName, chanName);
                send(client->sockfd, reply, strlen(reply), 0);

                snprintf(reply, sizeof(reply), ":%s 353 %s = %s :%s\r\n",
                         SERVER_NAME, client->nickname, chanName, client->nickname);
                send(client->sockfd, reply, strlen(reply), 0);

                snprintf(reply, sizeof(reply), ":%s 366 %s %s :End of /NAMES list.\r\n",
                         SERVER_NAME, client->nickname, chanName);
                send(client->sockfd, reply, strlen(reply), 0);
            }
        }
    }
    else if (strncmp(line, "PRIVMSG ", 8) == 0) {
        char channelTarget[MAX_CHAN_NAME];
        char userTarget[MAX_NICK];
        char* msgStart = strchr(line + 8, ':');
        enum PassTo mode = NONE;
        
        char* check = strchr(line, '#');
        if(check){
            mode = BROADCAST;
        } else{
            mode = USER;
        }

        switch(mode){
            case BROADCAST: {
                if (sscanf(line + 8, "%49s", channelTarget) == 1 && msgStart != NULL) {
                    snprintf(reply, sizeof(reply), ":%s!%s@localhost privmsg %s %s\r\n",
                             client->nickname, client->nickname, channelTarget, msgStart);
                    broadcastToChannel(channelTarget, reply, client->sockfd);
                }
                break;
            }
            
            case USER: {
                if(sscanf(line + 8, "%31s", userTarget) == 1 && msgStart != NULL){
                    snprintf(reply, sizeof(reply), ":%s!%s@localhost privmsg %s %s\r\n",
                             client->nickname, client->nickname, userTarget, msgStart);
                    sendToUser(userTarget, reply, client->sockfd);
                }
                break;
            }
            
            case NONE: {
                printf("Error: This options is not available!\n");
                break;
            }
        }
    }
    else if (strncmp(line, "QUIT", 4) == 0) {
        const char* quitReply = ":Closing Link\r\n";
        send(client->sockfd, quitReply, strlen(quitReply), 0);
        shutdown(client->sockfd, SHUT_RDWR);
    }
}


int findChannel(const char* name) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].isActive && strcmp(channels[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

ClientContext* findUser(const char* name) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].isActive) continue;
        for (int j = 0; j < channels[i].clientCount; j++) {
            if (strcmp(channels[i].clients[j].nickname, name) == 0) {
                return &channels[i].clients[j];
            }
        }
    }
    return NULL;
}

int joinChannel(const char* chanName, ClientContext* client) {
    pthread_mutex_lock(&channelMutex);
    int idx = findChannel(chanName);

    if (idx == -1) {
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (!channels[i].isActive) {
                strncpy(channels[i].name, chanName, MAX_CHAN_NAME - 1);
                channels[i].name[MAX_CHAN_NAME - 1] = '\0';
                channels[i].clientCount = 0;
                channels[i].isActive = 1;
                idx = i;
                break;
            }
        }
    }

    if (idx != -1 && channels[idx].clientCount < MAX_CLIENTS) {
        for (int i = 0; i < channels[idx].clientCount; i++) {
            if (channels[idx].clients[i].sockfd == client->sockfd) {
                pthread_mutex_unlock(&channelMutex);
                return idx;
            }
        }
        channels[idx].clients[channels[idx].clientCount++] = *client;
        pthread_mutex_unlock(&channelMutex);
        return idx;
    }

    pthread_mutex_unlock(&channelMutex);
    return -1;
}

void removeClientFromAllChannels(int clientFd) {
    pthread_mutex_lock(&channelMutex);
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].isActive) continue;
        for (int j = 0; j < channels[i].clientCount; j++) {
            if (channels[i].clients[j].sockfd == clientFd) {
                for (int k = j; k < channels[i].clientCount - 1; k++) {
                    channels[i].clients[k] = channels[i].clients[k + 1];
                }
                channels[i].clientCount--;
                if (channels[i].clientCount == 0) {
                    channels[i].isActive = 0;
                }
                break;
            }
        }
    }
    pthread_mutex_unlock(&channelMutex);
}

void broadcastToChannel(const char* chanName, const char* message, int senderFd) {
    pthread_mutex_lock(&channelMutex);
    int idx = findChannel(chanName);
    if (idx != -1) {
        for (int i = 0; i < channels[idx].clientCount; i++) {
            int targetFd = channels[idx].clients[i].sockfd;
            if (targetFd != senderFd) {
                send(targetFd, message, strlen(message), 0);
            }
        }
    }
    pthread_mutex_unlock(&channelMutex);
}

void sendToUser(const char* userName, const char* message, int senderFd) {
    pthread_mutex_lock(&channelMutex);
    ClientContext* client = findUser(userName);
    if (client) {
        int targetFd = client->sockfd;
        if(targetFd != senderFd){
            send(targetFd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&channelMutex);
}
