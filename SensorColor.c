#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define TCS34725_ADDR  0x29  // Dirección I2C del sensor
#define COMMAND_BIT    0x80  // Bit de comando para acceder a registros
#define ENABLE_REG     0x00  // Registro de control
#define ATIME_REG      0x01  // Tiempo de integración
#define CONTROL_REG    0x0F  // Configuración de ganancia
#define CDATAL_REG     0x14  // Datos de color

int main() {
    int fd;
    char *i2c_file = "/dev/i2c-1";
    uint8_t buffer[2];
    uint8_t data[8];

    // Abrir bus I2C
    if ((fd = open(i2c_file, O_RDWR)) < 0) {
        perror("Error al abrir el bus I2C");
        return 1;
    }

    // Seleccionar el dispositivo
    if (ioctl(fd, I2C_SLAVE, TCS34725_ADDR) < 0) {
        perror("No se pudo conectar con el sensor");
        close(fd);
        return 1;
    }

    // Habilitar el sensor (PON y AEN)
    buffer[0] = COMMAND_BIT | ENABLE_REG;
    buffer[1] = 0x03;  // PON | AEN
    write(fd, buffer, 2);

    // Configurar tiempo de integración (101 ms)
    buffer[0] = COMMAND_BIT | ATIME_REG;
    buffer[1] = 0xD5;
    write(fd, buffer, 2);
    
    // Configurar ganancia (4x)
    buffer[0] = COMMAND_BIT | CONTROL_REG;
    buffer[1] = 0x01;
    write(fd, buffer, 2);

    // Esperar integración
    sleep(1);

    // Solicitar datos de color
    buffer[0] = COMMAND_BIT | CDATAL_REG;
    write(fd, buffer, 1);
    read(fd, data, 8);

    // Convertir datos
    uint16_t clear = (data[1] << 8) | data[0];
    uint16_t red   = (data[3] << 8) | data[2];
    uint16_t green = (data[5] << 8) | data[4];
    uint16_t blue  = (data[7] << 8) | data[6];

    printf("Clear: %d\n", clear);
    printf("Red:   %d\n", red);
    printf("Green: %d\n", green);
    printf("Blue:  %d\n", blue);

    close(fd);
    return 0;
}
