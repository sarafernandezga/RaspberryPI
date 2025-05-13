/ IOTServer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#define PORT 12345
#define BUFFER_SIZE 2048
#define MAX_SAMPLES 10

typedef struct {
    float ax[MAX_SAMPLES], ay[MAX_SAMPLES], az[MAX_SAMPLES];
    int r[MAX_SAMPLES], g[MAX_SAMPLES], b[MAX_SAMPLES];
    int count;
} SensorData;

// Calculo de la media, maximo, minimo y desviacion estandar de los ejes de aceleracion
void compute_stats_float(const float *data, int count, const char *label) {
    float sum = 0, max = data[0], min = data[0], mean, stddev = 0;

    for (int i = 0; i < count; i++) {
        sum += data[i];
        if (data[i] > max) max = data[i];
        if (data[i] < min) min = data[i];
    }
    mean = sum / count;
    for (int i = 0; i < count; i++) {
        stddev += (data[i] - mean) * (data[i] - mean);
    }
    stddev = sqrt(stddev / count);

    printf("%s -> Mean: %.2f, Max: %.2f, Min: %.2f, Stddev: %.2f\n", label, mean, max, min, stddev);
}

// Cálculo de la media, maximo, minimo y desviacion estandar de los valores RGB
void compute_stats_int(const int *data, int count, const char *label) {
    int sum = 0, max = data[0], min = data[0];
    float mean, stddev = 0;

    for (int i = 0; i < count; i++) {
        sum += data[i];
        if (data[i] > max) max = data[i];
        if (data[i] < min) min = data[i];
    }
    mean = (float)sum / count;
    for (int i = 0; i < count; i++) {
        stddev += (data[i] - mean) * (data[i] - mean);
    }
    stddev = sqrt(stddev / count);

    printf("%s -> Mean: %.2f, Max: %d, Min: %d, Stddev: %.2f\n", label, mean, max, min, stddev);
}

// Extrae los valores numericos de los mensajes de texto recibidos del cliente para luego analizarlos
void parse_sensor_data(const char *msg, SensorData *data) {
    char line[256];
    int i = 0;
    const char *start = strstr(msg, "SENSOR_DATA_START");
    if (!start) return;

    const char *end = strstr(msg, "SENSOR_DATA_END");
    if (!end) return;

    start = strchr(start, '\n'); // skip header
    while (start && start < end && i < MAX_SAMPLES) {
        start++; // skip newline
        const char *next_line = strchr(start, '\n');
        int r, g, b;
        float ax, ay, az;

        if (sscanf(start, "Sample %*d: Accel[%f,%f,%f] Color[%d,%d,%d]",
                   &ax, &ay, &az, &r, &g, &b) == 6) {
            data->ax[i] = ax;
            data->ay[i] = ay;
            data->az[i] = az;
            data->r[i] = r;
            data->g[i] = g;
            data->b[i] = b;
            i++;
        }

        start = next_line;
    }
    data->count = i;
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Crear socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP escuchando en el puerto %d...\n", PORT);

    while (1) {
        // Esperar mensaje
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            perror("Error al recibir datos");
            continue;
        }

        buffer[n] = '\0'; // asegurar terminación
        printf("\n Datos recibidos del cliente:\n");

        // Analiza los datos recibidos
        SensorData data = { .count = 0 };
        parse_sensor_data(buffer, &data);

        // Imprimir cada muestra recibida
		for (int i = 0; i < data.count; i++) {
			printf("Medida %d: Accel[%.2f, %.2f, %.2f] Color[%d, %d, %d]\n",
				   i + 1,
				   data.ax[i], data.ay[i], data.az[i],
				   data.r[i], data.g[i], data.b[i]);
		}

        // Calcula la media, maximo, minimo y desviacion estandar de los datos recibidos
        if (data.count > 0) {
            compute_stats_float(data.ax, data.count, "Accel X");
            compute_stats_float(data.ay, data.count, "Accel Y");
            compute_stats_float(data.az, data.count, "Accel Z");
            compute_stats_int(data.r, data.count, "Color R");
            compute_stats_int(data.g, data.count, "Color G");
            compute_stats_int(data.b, data.count, "Color B");
        } else {
            printf(" No se pudieron extraer muestras validas.\n");
        }

        // Enviar ACK
        char ack[] = "ACK: Datos recibidos y procesados.";
        sendto(sockfd, ack, strlen(ack), 0,
               (struct sockaddr*)&client_addr, addr_len);
    }

    // Cerrar socket
    close(sockfd);
    return 0;
}

