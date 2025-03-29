#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

// ** Direcciones I2C **
#define TCS34725_ADDR  0x29  // Sensor de color (I2C-1)
#define MPU6050_ADDR   0x68  // Acelerómetro/Giroscopio (I2C-0)

// ** Registros TCS34725 **
#define COMMAND_BIT    0x80  
#define ENABLE_REG     0x00  
#define ATIME_REG      0x01  
#define CONTROL_REG    0x0F  
#define CDATAL_REG     0x14  

// ** Registros MPU6050 **
#define PWR_MGMT_1     0x6B  
#define ACCEL_XOUT_H   0x3B  

// Función para escribir en I2C
int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {COMMAND_BIT | reg, value};
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    return write(fd, buffer, 2);
}

// Función para leer desde I2C
int i2c_read(int fd, uint8_t addr, uint8_t reg, uint8_t *data, uint8_t length) {
    uint8_t reg_buffer = COMMAND_BIT | reg;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    if (write(fd, &reg_buffer, 1) != 1) return -1;
    return read(fd, data, length);
}

// Leer datos del sensor de color
void read_tcs34725(int fd) {
    uint8_t data[8];

    if (i2c_read(fd, TCS34725_ADDR, CDATAL_REG, data, 8) < 0) {
        perror("Error al leer el sensor de color");
        return;
    }

    uint16_t clear = (data[1] << 8) | data[0];
    uint16_t red   = (data[3] << 8) | data[2];
    uint16_t green = (data[5] << 8) | data[4];
    uint16_t blue  = (data[7] << 8) | data[6];

    printf("[TCS34725] Clear: %d, Red: %d, Green: %d, Blue: %d\n", clear, red, green, blue);
}

// Leer datos del MPU6050
void read_mpu6050(int fd) {
    uint8_t data[6];

    if (i2c_read(fd, MPU6050_ADDR, ACCEL_XOUT_H, data, 6) < 0) {
        perror("Error al leer el MPU6050");
        return;
    }

    int16_t accel_x = (data[0] << 8) | data[1];
    int16_t accel_y = (data[2] << 8) | data[3];
    int16_t accel_z = (data[4] << 8) | data[5];

    printf("[MPU6050] Accel X: %.2f g, Y: %.2f g, Z: %.2f g\n",
           accel_x / 16384.0, accel_y / 16384.0, accel_z / 16384.0);
}

// Configurar TCS34725
void setup_tcs34725(int fd) {
    i2c_write(fd, TCS34725_ADDR, ENABLE_REG, 0x03);
    i2c_write(fd, TCS34725_ADDR, ATIME_REG, 0xD5);
    i2c_write(fd, TCS34725_ADDR, CONTROL_REG, 0x01);
}

// Configurar MPU6050
void setup_mpu6050(int fd) {
    i2c_write(fd, MPU6050_ADDR, PWR_MGMT_1, 0x00);
}

int main() {
    int fd0, fd1;
    char *i2c_file1 = "/dev/i2c-0"; // Bus 0 para TCS34725
    char *i2c_file0 = "/dev/i2c-1"; // Bus 1 para MPU6050

    // Abrir I2C-1
    if ((fd1 = open(i2c_file1, O_RDWR)) < 0) {
        perror("Error al abrir I2C-1");
        return 1;
    }

    // Abrir I2C-0
    if ((fd0 = open(i2c_file0, O_RDWR)) < 0) {
        perror("Error al abrir I2C-0");
        close(fd1);
        return 1;
    }

    // Configurar sensores
    setup_tcs34725(fd1);
    setup_mpu6050(fd0);

    while (1) {
        sleep(1);
        read_tcs34725(fd1);
        read_mpu6050(fd0);
    }

    close(fd1);
    close(fd0);
    return 0;
}
