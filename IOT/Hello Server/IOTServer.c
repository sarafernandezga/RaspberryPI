#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void print_usage() {
    printf("Uso: IOTServer -h <host> -p <port> -b <buffer_size>\n");
    printf("  -h <host>         Dirección IP del servidor (por defecto: 0.0.0.0)\n");
    printf("  -p <port>         Puerto de escucha (debe especificarse)\n");
    printf("  -b <buffer_size>  Tamaño del buffer (debe especificarse)\n");
}

int main(int argc, char *argv[]) {
    int sockfd, port = 0, buffer_size = 0;
    char *host = "0.0.0.0";
    char *buffer;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Parsear argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0) {
            buffer_size = atoi(argv[++i]);
        }
    }

    if (port == 0 || buffer_size == 0) {
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
    server_addr.sin_addr.s_addr = inet_addr(host);
    server_addr.sin_port = htons(port);

    // Asociar socket
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en %s:%d con buffer de %d bytes\n", host, port, buffer_size);

    // Esperar mensajes
    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, buffer_size - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';
        printf("Recibido: %s\n", buffer);

        char *reply = (strcmp(buffer, "Hello Server") == 0) ? "Hello RPI" : "Wrong Message";
        sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, addr_len);
        printf("Respuesta enviada: %s\n", reply);
    }

    close(sockfd);
    free(buffer);
    return 0;
}
