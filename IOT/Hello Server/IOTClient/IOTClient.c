// This code has been developed by Sara Fernández and Yaiza Moyons with the help of AI and the functions that appear in the slides of the subject

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void print_usage() {
    printf("Uso: IOTClient -h <host> -p <port> -b <buffer_size> -m <message>\n");
    printf("  -h <host>         Dirección IP del servidor\n");
    printf("  -p <port>         Puerto del servidor\n");
    printf("  -b <buffer_size>  Tamaño del buffer\n");
    printf("  -m <message>      Mensaje a enviar\n");
}

int main(int argc, char *argv[]) {
    int sockfd, port = 0, buffer_size = 0;
    char *host = NULL;
    char *message = NULL;
    char *buffer;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Parsear argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0) {
            buffer_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0) {
            message = argv[++i];
        }
    }

    if (!host || port == 0 || buffer_size == 0 || !message) {
        print_usage();
        return 1;
    }

    buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    // Crear socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    // Enviar mensaje
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Mensaje enviado: %s\n", message);

    // Esperar respuesta
    recvfrom(sockfd, buffer, buffer_size, 0, (struct sockaddr*)&server_addr, &addr_len);
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("Respuesta del servidor: %s\n", buffer);

    close(sockfd);
    free(buffer);
    return 0;
}
