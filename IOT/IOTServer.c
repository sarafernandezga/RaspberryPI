#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

void print_usage() {
    printf("Uso: IOTServer_Sensors -p <port> -b <buffer_size> -m <max_samples>\n");
    printf("  -p <port>         Puerto de escucha (por defecto: 12345)\n");
    printf("  -b <buffer_size>  Tamaño del buffer (por defecto: 2048)\n");
    printf("  -m <max_samples>  Número máximo de muestras (por defecto: 10)\n");
}

typedef struct {
    float *ax, *ay, *az;
    int *r, *g, *b;
    int count;
} SensorData;

void allocate_sensor_data(SensorData *data, int max_samples) {
    data->ax = (float *)malloc(max_samples * sizeof(float));
    data->ay = (float *)malloc(max_samples * sizeof(float));
    data->az = (float *)malloc(max_samples * sizeof(float));
    data->r = (int *)malloc(max_samples * sizeof(int));
    data->g = (int *)malloc(max_samples * sizeof(int));
    data->b = (int *)malloc(max_samples * sizeof(int));
}

void free_sensor_data(SensorData *data) {
    free(data->ax);
    free(data->ay);
    free(data->az);
    free(data->r);
    free(data->g);
    free(data->b);
}

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
