#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>

#define TCS3472_ADDR  0x29  // Dirección I2C del sensor
#define ENABLE_REG     0x00  // Registro de control
#define ATIME_REG      0x01  // Tiempo de integración
#define CONTROL_REG    0x0F  // Configuración de ganancia
#define CDATAL_REG     0x14  // Datos de color (16 bits cada canal)

int w_len = 2;  // Longitud de escritura
int r_len = 8;  // Longitud de lectura (RGBA)

struct i2c_rdwr_ioctl_data packets;
struct i2c_msg messages[2];

uint8_t write_bytes[2];  // Buffer de escritura
uint8_t read_bytes[8];   // Buffer de lectura

int main() {
    char i2cFile[] = "/dev/i2c-1";
    int fd = open(i2cFile, O_RDWR);
    if (fd < 0) {
        perror("Error al abrir el bus I2C");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, TCS3472_ADDR) < 0) {
        perror("No se pudo conectar con el sensor");
        close(fd);
        return 1;
    }

    printf("Habilitando el sensor...\n");

    // Habilitar el sensor (PON y AEN)
    write_bytes[0] = ENABLE_REG;
    write_bytes[1] = 0x03; // PON (bit 0) y AEN (bit 1)

    messages[0].addr = TCS3472_ADDR;
    messages[0].flags = 0;  // Escritura
    messages[0].len = w_len;
    messages[0].buf = write_bytes;

    packets.msgs = messages;
    packets.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        perror("Error al habilitar el sensor");
        close(fd);
        return 1;
    }

    // Configurar tiempo de integración y ganancia
    write_bytes[0] = ATIME_REG;
    write_bytes[1] = 0xD5; // 101 ms de integración
    ioctl(fd, I2C_RDWR, &packets);

    write_bytes[0] = CONTROL_REG;
    write_bytes[1] = 0x01; // Ganancia de 4x
    ioctl(fd, I2C_RDWR, &packets);

    printf("Esperando medición...\n");
    sleep(1); // Esperar a que el sensor haga la medición

    // Solicitar datos de color
    write_bytes[0] = CDATAL_REG;
    messages[0].addr = TCS3472_ADDR;
    messages[0].flags = 0;  // Escritura
    messages[0].len = 1;
    messages[0].buf = write_bytes;

    messages[1].addr = TCS3472_ADDR;
    messages[1].flags = I2C_M_RD;  // Lectura
    messages[1].len = r_len;
    messages[1].buf = read_bytes;

    packets.msgs = messages;
    packets.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        perror("Error al leer datos del sensor");
        close(fd);
        return 1;
    }

    // Convertir datos
    uint16_t clear = (read_bytes[1] << 8) | read_bytes[0];
    uint16_t red = (read_bytes[3] << 8) | read_bytes[2];
    uint16_t green = (read_bytes[5] << 8) | read_bytes[4];
    uint16_t blue = (read_bytes[7] << 8) | read_bytes[6];

    printf("Medición de color:\n");
    printf("Clear: %d\n", clear);
    printf("Red:   %d\n", red);
    printf("Green: %d\n", green);
    printf("Blue:  %d\n", blue);

    close(fd);
    return 0;
}
