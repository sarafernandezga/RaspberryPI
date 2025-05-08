// IOTClient.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// UDP Config
#define SERVER_IP "192.168.149.237"
#define SERVER_PORT 12345
#define BUFFER_SIZE 2048

// I2C
#define TCS34725_ADDR  0x29
#define MPU6050_ADDR   0x68
#define COMMAND_BIT    0x80
#define ENABLE_REG     0x00
#define ATIME_REG      0x01
#define CONTROL_REG    0x0F
#define CDATAL_REG     0x14
#define PWR_MGMT_1     0x6B
#define ACCEL_XOUT_H   0x3B

int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {COMMAND_BIT | reg, value};
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    return write(fd, buffer, 2);
}

int i2c_read(int fd, uint8_t addr, uint8_t reg, uint8_t *data, uint8_t length) {
    uint8_t reg_buffer = COMMAND_BIT | reg;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    if (write(fd, &reg_buffer, 1) != 1) return -1;
    return read(fd, data, length);
}

void setup_sensors(int fd) {
    // TCS34725
    i2c_write(fd, TCS34725_ADDR, ENABLE_REG, 0x03);
    i2c_write(fd, TCS34725_ADDR, ATIME_REG, 0xD5);
    i2c_write(fd, TCS34725_ADDR, CONTROL_REG, 0x01);
    // MPU6050
    i2c_write(fd, MPU6050_ADDR, PWR_MGMT_1, 0x00);
}

void read_color(int fd, uint16_t *r, uint16_t *g, uint16_t *b) {
    uint8_t data[8];
    if (i2c_read(fd, TCS34725_ADDR, CDATAL_REG, data, 8) < 0) {
        perror("Error al leer sensor de color");
        *r = *g = *b = 0;
        return;
    }

    uint16_t clear = (data[1] << 8) | data[0];
    uint16_t red   = (data[3] << 8) | data[2];
    uint16_t green = (data[5] << 8) | data[4];
    uint16_t blue  = (data[7] << 8) | data[6];

    if (clear == 0) clear = 1; // prevenir división por cero

    *r = red * 255 / clear;
    *g = green * 255 / clear;
    *b = blue * 255 / clear;
}

void read_accel(int fd, float *ax, float *ay, float *az) {
    uint8_t data[6];
    if (i2c_read(fd, MPU6050_ADDR, ACCEL_XOUT_H, data, 6) < 0) {
        perror("Error al leer MPU6050");
        *ax = *ay = *az = 0;
        return;
    }

    int16_t raw_x = (data[0] << 8) | data[1];
    int16_t raw_y = (data[2] << 8) | data[3];
    int16_t raw_z = (data[4] << 8) | data[5];

    *ax = raw_x / 16384.0;
    *ay = raw_y / 16384.0;
    *az = raw_z / 16384.0;
}

int main() {
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("No se pudo abrir el bus I2C");
        exit(EXIT_FAILURE);
    }

    setup_sensors(i2c_fd);

    // UDP
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear socket UDP");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    while (1) {
        float ax[10], ay[10], az[10];
        uint16_t r[10], g[10], b[10];

        printf("Recolectando datos por 10 segundos...\n");
        for (int i = 0; i < 10; i++) {
            read_accel(i2c_fd, &ax[i], &ay[i], &az[i]);
            read_color(i2c_fd, &r[i], &g[i], &b[i]);
            sleep(1);
        }

        // Formatear mensaje
        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);
        strcat(message, "ACC:");
        for (int i = 0; i < 10; i++) {
            char temp[64];
            snprintf(temp, sizeof(temp), "%.2f,%.2f,%.2f%s", ax[i], ay[i], az[i], (i < 9) ? ";" : "\n");
            strcat(message, temp);
        }

        strcat(message, "COL:");
        for (int i = 0; i < 10; i++) {
            char temp[64];
            snprintf(temp, sizeof(temp), "%u,%u,%u%s", r[i], g[i], b[i], (i < 9) ? ";" : "\n");
            strcat(message, temp);
        }

        // Enviar al servidor
        sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&server_addr, addr_len);
        printf("Datos enviados al servidor:\n%s\n", message);

        // Esperar ACK
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Servidor respondió: %s\n", buffer);
        } else {
            printf("No se recibió respuesta del servidor.\n");
        }
    }

    close(sockfd);
    close(i2c_fd);
    return 0;
}
