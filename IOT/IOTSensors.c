// sensors.c
#include "sensors.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define TCS34725_ADDR  0x29
#define MPU6050_ADDR   0x68

#define COMMAND_BIT    0x80
#define ENABLE_REG     0x00
#define ATIME_REG      0x01
#define CONTROL_REG    0x0F
#define CDATAL_REG     0x14

#define PWR_MGMT_1     0x6B
#define ACCEL_XOUT_H   0x3B

static int i2c_fd = -1;

static int i2c_write(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = { COMMAND_BIT | reg, value };
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0) return -1;
    return write(i2c_fd, buffer, 2);
}

static int i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t length) {
    uint8_t reg_buffer = COMMAND_BIT | reg;
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0) return -1;
    if (write(i2c_fd, &reg_buffer, 1) != 1) return -1;
    return read(i2c_fd, data, length);
}

int init_sensors() {
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("No se pudo abrir el bus I2C");
        return -1;
    }

    // Init color sensor
    i2c_write(TCS34725_ADDR, ENABLE_REG, 0x03);
    i2c_write(TCS34725_ADDR, ATIME_REG, 0xD5);
    i2c_write(TCS34725_ADDR, CONTROL_REG, 0x01);

    // Init accelerometer
    i2c_write(MPU6050_ADDR, PWR_MGMT_1, 0x00);

    return 0;
}

int read_accel_sample(AccelData *sample) {
    uint8_t data[6];
    if (i2c_read(MPU6050_ADDR, ACCEL_XOUT_H, data, 6) < 0) return -1;

    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];

    sample->ax = ax / 16384.0;
    sample->ay = ay / 16384.0;
    sample->az = az / 16384.0;

    return 0;
}

int read_color_sample(ColorData *sample) {
    uint8_t data[8];
    if (i2c_read(TCS34725_ADDR, CDATAL_REG, data, 8) < 0) return -1;

    uint16_t clear = (data[1] << 8) | data[0];
    uint16_t red   = (data[3] << 8) | data[2];
    uint16_t green = (data[5] << 8) | data[4];
    uint16_t blue  = (data[7] << 8) | data[6];

    // Normalize to 255 scale
    if (clear == 0) clear = 1; // evitar división por 0
    sample->r = red * 255 / clear;
    sample->g = green * 255 / clear;
    sample->b = blue * 255 / clear;

    return 0;
}

void close_sensors() {
    if (i2c_fd >= 0) close(i2c_fd);
}
