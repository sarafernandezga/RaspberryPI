#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

typedef struct {
    float *ax, *ay, *az;
    int *r, *g, *b;
    int count;
} SensorData;

void print_usage() {
    printf("Uso: IOTServer_Sensors -p <port> -b <buffer_size> -m <max_samples>\n");
    printf("  -p <port>         Puerto de escucha\n");
    printf("  -b <buffer_size>  Tamaño del buffer en bytes\n");
    printf("  -m <max_samples>  Número máximo de muestras a almacenar\n");
}

void allocate_sensor_data(SensorData *data, int max_samples) {
    data->ax = malloc(max_samples * sizeof(float));
    data->ay = malloc(max_samples * sizeof(float));
    data->az = malloc(max_samples * sizeof(float));
    data->r = malloc(max_samples * sizeof(int));
    data->g = malloc(max_samples * sizeof(int));
    data->b = malloc(max_samples * sizeof(int));
}

void free_sensor_data(SensorData *data) {
    free(data->ax); free(data->ay); free(data->az);
    free(data->r);  free(data->g);  free(data->b);
}

void compute_stats_float(const float *data, int count, const char *label) {
    float sum = 0, max = data[0], min = data[0];
    for (int i = 0; i < count; i++) {
        sum += data[i];
        if (data[i] > max) max = data[i];
        if (data[i] < min) min = data[i];
    }
    float mean = sum / count;
    float stddev = 0;
    for (int i = 0; i < count; i++) {
        stddev += (data[i] - mean) * (data[i] - mean);
    }
    stddev = sqrt(stddev / count);
    printf("%s -> Mean: %.2f, Max: %.2f, Min: %.2f, Stddev: %.2f\n", label, mean, max, min, stddev);
}

void compute_stats_int(const int *data, int count, const char *label) {
    int sum = 0, max = data[0], min = data[0];
    for (int i = 0; i < count; i++) {
        sum += data[i];
        if (data[i] > max) max = data[i];
        if (data[i] < min) min = data[i];
    }
    float mean = (float)sum / count;
    float stddev = 0;
    for (int i = 0; i < count; i++) {
        stddev += (data[i] - mean) * (data[i] - mean);
    }
    stddev = sqrt(stddev / count);
    printf("%s -> Mean: %.2f, Max: %d, Min: %d, Stddev: %.2f\n", label, mean, max, min, stddev);
}

// Debes tener una función parse_sensor_data(buffer, &data) implementada

int main(int argc, char *argv[]) {
    int port = 0, buffer_size = 0, max_samples = 0;

    // Leer argumentos estilo cliente
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0) {
            buffer_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0) {
            max_samples = atoi(argv[++i]);
        }
    }

    if (port == 0 || buffer_size == 0 || max_samples == 0) {
        fprintf(stderr, "Error: Todos los parámetros -p, -b y -m son obligatorios.\n\n");
        print_usage();
        return 1;
    }

    printf("Servidor iniciado en el puerto %d\n", port);

    // Crear socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear socket");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(sockfd);
        return 1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Error al asignar buffer");
        close(sockfd);
        return 1;
    }

    SensorData data;
    allocate_sensor_data(&data, max_samples);

    while (1) {
        int len = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                           (struct sockaddr*)&client_addr, &addr_len);
        if (len < 0) {
            perror("Error al recibir datos");
            continue;
        }

        buffer[len] = '\0';
        printf("\nDatos recibidos del cliente:\n");

        data.count = 0;
        parse_sensor_data(buffer, &data);

        for (int i = 0; i < data.count; i++) {
            printf("Muestra %d: Accel[%.2f, %.2f, %.2f] Color[%d, %d, %d]\n",
                   i + 1, data.ax[i], data.ay[i], data.az[i],
                   data.r[i], data.g[i], data.b[i]);
        }

        if (data.count > 0) {
            compute_stats_float(data.ax, data.count, "Accel X");
            compute_stats_float(data.ay, data.count, "Accel Y");
            compute_stats_float(data.az, data.count, "Accel Z");
            compute_stats_int(data.r, data.count, "Color R");
            compute_stats_int(data.g, data.count, "Color G");
            compute_stats_int(data.b, data.count, "Color B");
        } else {
            printf("No se pudieron procesar muestras válidas.\n");
        }

        char ack[] = "ACK: Datos recibidos y procesados.";
        sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr*)&client_addr, addr_len);
    }

    free_sensor_data(&data);
    free(buffer);
    close(sockfd);
    return 0;
}

