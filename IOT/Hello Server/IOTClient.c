// IOTClient.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.149.237" // Replace with your VM's IP
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // 1. Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Server details
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // 3. Send message
    char *message = "Hello Server";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Message sent: %s\n", message);

    // 4. Wait for reply
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("Server replied: %s\n", buffer);

    // 5. Close
    close(sockfd);
    return 0;
}
