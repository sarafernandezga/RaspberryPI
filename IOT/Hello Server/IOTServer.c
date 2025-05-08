// IOTServer.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 1. Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Wait for message
    printf("Server waiting for message...\n");
       ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &addr_len);  // Use BUFFER_SIZE-1 to leave room for '\0'
       if (n < 0) {
           perror("Receive failed");
           close(sockfd);
           exit(EXIT_FAILURE);
       }

    buffer[n] = '\0'; // Ensure null termination after receiving

    printf("Received: %s\n", buffer);

    // 4. Prepare reply
    char *reply;
    if (strcmp(buffer, "Hello Server") == 0)
        reply = "Hello RPI";
    else
        reply = "Wrong Message";

    sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, addr_len);
    printf("Reply sent: %s\n", reply);

    // 5. Close
    close(sockfd);
    return 0;
}
