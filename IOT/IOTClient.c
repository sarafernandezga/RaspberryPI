// IOTClient.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "sensors.h"

#define SERVER_IP "192.168.149.237" // Cambiar por la IP de la VM
#define SERVER_PORT 12345
#define BUFFER_SIZE 2048
#define SAMPLE_RATE 1       // 1 muestra por segundo
#define SAMPLE_DURATION 10  // 10 segundos de muestreo

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Inicializar sensores
    if (init_sensors() < 0) {
        fprintf(stderr, "Error al inicializar sensores\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Cliente iniciado. Comenzando muestreo de sensores...\n");

    while (1) {
        AccelData accel_samples[SAMPLE_DURATION];
        ColorData color_samples[SAMPLE_DURATION];

        // Toma de muestras cada 1s por 10s
        for (int i = 0; i < SAMPLE_DURATION; i++) {
            if (read_accel_sample(&accel_samples[i]) < 0)
                fprintf(stderr, "Error al leer acelerómetro en muestra %d\n", i);
            if (read_color_sample(&color_samples[i]) < 0)
                fprintf(stderr, "Error al leer sensor de color en muestra %d\n", i);

            sleep(SAMPLE_RATE);
        }

        // Preparar mensaje
        char message[BUFFER_SIZE];
        int offset = 0;

        offset += snprintf(message + offset, BUFFER_SIZE - offset, "SENSOR_DATA_START\n");
        for (int i = 0; i < SAMPLE_DURATION; i++) {
            offset += snprintf(
                message + offset, BUFFER_SIZE - offset,
                "Sample %02d: Accel[%.2f,%.2f,%.2f] Color[%u,%u,%u]\n",
                i + 1,
                accel_samples[i].ax,
                accel_samples[i].ay,
                accel_samples[i].az,
                color_samples[i].r,
                color_samples[i].g,
                color_samples[i].b
            );
        }
        offset += snprintf(message + offset, BUFFER_SIZE - offset, "SENSOR_DATA_END");

        // Enviar al servidor
        sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, addr_len);

        printf("Datos de sensores enviados al servidor.\n");

        // Esperar ACK
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr*)&server_addr, &addr_len);
        if (n >= 0) {
            buffer[n] = '\0';
            printf("ACK recibido del servidor: %s\n", buffer);
        } else {
            perror("Error al recibir ACK del servidor");
        }
    }

    close_sensors();
    close(sockfd);
    return 0;
}
