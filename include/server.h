#pragma once
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

typedef struct {
    int serverFd;
    struct sockaddr_in address;
} ServerSettings;

ServerSettings setupTCPServer(const char* ip, unsigned short port);
void runServer(ServerSettings* settings);
void* handleClient(void* arg);
