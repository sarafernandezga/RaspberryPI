#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MPU6050_ADDR   0x68
#define PWR_MGMT_1     0x6B
#define ACCEL_XOUT_H   0x3B
#define TEMP_OUT_H     0x41

int i2c_read(int fd, uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    if (write(fd, &reg, 1) != 1) return -1;
    return read(fd, data, len);
}

int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    return write(fd, buffer, 2);
}

int main() {
    int fd;
    const char *i2c_file = "/dev/i2c-1";

    // Abrir bus I2C
    if ((fd = open(i2c_file, O_RDWR)) < 0) {
        perror("Error al abrir I2C");
        return 1;
    }

    // Activar MPU6050
    i2c_write(fd, MPU6050_ADDR, PWR_MGMT_1, 0x00);

    // Crear socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear socket UDP");
        return 1;
    }

    struct sockaddr_in servidor;
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(12345);
    servidor.sin_addr.s_addr = inet_addr("192.168.X.X"); // <-- PON AQUÍ LA IP DE TU MÁQUINA VIRTUAL

    char mensaje[256];

    while (1) {
        sleep(1);

        // Leer aceleración
        uint8_t accel_data[6];
        i2c_read(fd, MPU6050_ADDR, ACCEL_XOUT_H, accel_data, 6);
        int16_t ax = (accel_data[0] << 8) | accel_data[1];
        int16_t ay = (accel_data[2] << 8) | accel_data[3];
        int16_t az = (accel_data[4] << 8) | accel_data[5];

        // Leer temperatura
        uint8_t temp_data[2];
        i2c_read(fd, MPU6050_ADDR, TEMP_OUT_H, temp_data, 2);
        int16_t raw_temp = (temp_data[0] << 8) | temp_data[1];
        float temp_c = (raw_temp / 340.0) + 36.53;

        // Formatear y enviar mensaje
        snprintf(mensaje, sizeof(mensaje),
                 "AX: %.2f g, AY: %.2f g, AZ: %.2f g, TEMP: %.2f C",
                 ax / 16384.0, ay / 16384.0, az / 16384.0, temp_c);

        sendto(sockfd, mensaje, strlen(mensaje), 0, (struct sockaddr *)&servidor, sizeof(servidor));
        printf("Enviado: %s\n", mensaje);
    }

    close(fd);
    close(sockfd);
    return 0;
}
