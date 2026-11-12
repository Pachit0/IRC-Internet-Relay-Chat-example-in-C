#include "pch.h"
#include "server.h"
#include "irc.h"

ServerSettings setupTCPServer(const char* ip, unsigned short port){
    ServerSettings settings;
    int opt = 1;
    
    if ((settings.serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        memset(&settings, 0, sizeof(settings));
        settings.serverFd = -1;
        return settings;
    }

    if (setsockopt(settings.serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(settings.serverFd);
        memset(&settings, 0, sizeof(settings));
        settings.serverFd = -1;
        return settings;
    }

    settings.address.sin_family = AF_INET;
    settings.address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &(settings.address.sin_addr)) <= 0) {
        perror("Invalid IP address");
        close(settings.serverFd);
        memset(&settings, 0, sizeof(settings));
        settings.serverFd = -1;
        return settings;
    }

    if (bind(settings.serverFd, (struct sockaddr*)&settings.address, sizeof(settings.address)) < 0) {
        perror("bind failed");
        close(settings.serverFd);
        memset(&settings, 0, sizeof(settings));
        settings.serverFd = -1;
        return settings;
    }

    if (listen(settings.serverFd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(settings.serverFd);
        memset(&settings, 0, sizeof(settings));
        settings.serverFd = -1;
        return settings;
    }

    printf("IRC Server listening on %s:%d...\n", ip, port);
    return settings;
}

void runServer(ServerSettings* settings){
    socklen_t addrlen = sizeof(settings->address);
    while (1) {
        int clientSocket = accept(settings->serverFd, (struct sockaddr*)&settings->address, &addrlen);
        if (clientSocket < 0) {
            perror("accept failed");
            continue;
        }

        printf("[+] New client connected (fd: %d)\n", clientSocket);

        ClientContext* client = malloc(sizeof(ClientContext));
        if (!client) {
            close(clientSocket);
            continue;
        }
        client->sockfd = clientSocket;
        strncpy(client->nickname, "user", MAX_NICK - 1);
        client->nickname[MAX_NICK - 1] = '\0';

        pthread_t threadId;
        if (pthread_create(&threadId, NULL, handleClient, client) != 0) {
            perror("pthread_create failed");
            close(clientSocket);
            free(client);
            continue;
        }
        pthread_detach(threadId);
    }
}

void* handleClient(void* arg) {
    ClientContext* client = (ClientContext*)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = recv(client->sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';

        char* line = strtok(buffer, "\r\n");
        while (line != NULL) {
            printf("[%s | fd:%d] -> %s\n", client->nickname, client->sockfd, line);
            processCommand(client, line);
            line = strtok(NULL, "\r\n");
        }
    }

    printf("[-] Client disconnected (%s | fd: %d)\n", client->nickname, client->sockfd);
    removeClientFromAllChannels(client->sockfd);
    close(client->sockfd);
    free(client);
    return NULL;
}
