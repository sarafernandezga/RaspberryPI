// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

#define SAMPLE_COUNT 10

typedef struct {
    float ax, ay, az;
} AccelData;

typedef struct {
    uint16_t r, g, b;
} ColorData;

// Inicializa los sensores (debe llamarse una vez)
int init_sensors();

// Lee una muestra de acelerómetro
int read_accel_sample(AccelData *sample);

// Lee una muestra del sensor de color
int read_color_sample(ColorData *sample);

// Cierra recursos si es necesario
void close_sensors();

#endif
