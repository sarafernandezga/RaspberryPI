#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "sensors.h"

void print_usage() {
    printf("Uso: IOTClient_Sensors -h <host> -p <port> -b <buffer_size> -r <sample_rate> -d <sample_duration>\n");
    printf("  -h <host>         Dirección IP del servidor\n");
    printf("  -p <port>         Puerto del servidor\n");
    printf("  -b <buffer_size>  Tamaño del buffer\n");
    printf("  -r <sample_rate>  Frecuencia de muestreo (segundos)\n");
    printf("  -d <duration>     Duración del muestreo (segundos)\n");
}

int main(int argc, char *argv[]) {
    int sockfd, port = 0, buffer_size = 0, sample_rate = 0, sample_duration = 0;
    char *host = NULL;
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
        } else if (strcmp(argv[i], "-r") == 0) {
            sample_rate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0) {
            sample_duration = atoi(argv[++i]);
        }
    }

    if (!host || port == 0 || buffer_size == 0 || sample_rate == 0 || sample_duration == 0) {
        print_usage();
        return 1;
    }

    buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (init_sensors() < 0) {
        fprintf(stderr, "Error al inicializar sensores\n");
        close(sockfd);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    while (1) {
        AccelData accel_samples[sample_duration];
        ColorData color_samples[sample_duration];
        char message[buffer_size];
        int offset = 0;

        for (int i = 0; i < sample_duration; i++) {
            read_accel_sample(&accel_samples[i]);
            read_color_sample(&color_samples[i]);
            sleep(sample_rate);
        }

        offset += snprintf(message + offset, buffer_size - offset, "SENSOR_DATA_START\n");
        for (int i = 0; i < sample_duration; i++) {
            offset += snprintf(message + offset, buffer_size - offset,
                "Sample %02d: Accel[%.2f,%.2f,%.2f] Color[%u,%u,%u]\n",
                i + 1,
                accel_samples[i].ax, accel_samples[i].ay, accel_samples[i].az,
                color_samples[i].r, color_samples[i].g, color_samples[i].b);
        }
        offset += snprintf(message + offset, buffer_size - offset, "SENSOR_DATA_END");

        sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&server_addr, addr_len);
        printf("Datos enviados al servidor.\n");

        ssize_t n = recvfrom(sockfd, buffer, buffer_size - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (n >= 0) {
            buffer[n] = '\0';
            printf("ACK del servidor: %s\n", buffer);
        } else {
            perror("Error al recibir ACK");
        }
    }

    close_sensors();
    close(sockfd);
    free(buffer);
    return 0;
}

