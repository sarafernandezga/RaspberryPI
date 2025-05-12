#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <string.h>

// ** Direcciones I2C **
#define TCS34725_ADDR  0x29  // Sensor de color
#define MPU6050_ADDR   0x68  // Acelerómetro/Giroscopio

// ** Registros TCS34725 **
#define COMMAND_BIT    0x80  
#define ENABLE_REG     0x00  
#define ATIME_REG      0x01  
#define CONTROL_REG    0x0F  
#define CDATAL_REG     0x14  

// ** Registros MPU6050 **
#define PWR_MGMT_1     0x6B  
#define ACCEL_XOUT_H   0x3B  

// ** Funciones para escribir y leer de los sensores I2C**
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

// ** Funciones para leer los sensores**
void read_tcs34725(int fd, uint16_t* red, uint16_t* green, uint16_t* blue, uint16_t* clear) {
    uint8_t data[8];

    if (i2c_read(fd, TCS34725_ADDR, CDATAL_REG, data, 8) < 0) {
        perror("Error al leer el sensor de color");
        return;
    }

    *clear = (data[1] << 8) | data[0];
    *red   = (data[3] << 8) | data[2];
    *green = (data[5] << 8) | data[4];
    *blue  = (data[7] << 8) | data[6];
}

void read_mpu6050(int fd, float* accel_x, float* accel_y, float* accel_z) {
    uint8_t data[6];

    if (i2c_read(fd, MPU6050_ADDR, ACCEL_XOUT_H, data, 6) < 0) {
        perror("Error al leer el MPU6050");
        return;
    }

    int16_t accel_x_raw = (data[0] << 8) | data[1];
    int16_t accel_y_raw = (data[2] << 8) | data[3];
    int16_t accel_z_raw = (data[4] << 8) | data[5];

    *accel_x = accel_x_raw / 16384.0;
    *accel_y = accel_y_raw / 16384.0;
    *accel_z = accel_z_raw / 16384.0;
}

// ** Funciones para configurar los sensores**
void setup_tcs34725(int fd) {
    i2c_write(fd, TCS34725_ADDR, ENABLE_REG, 0x03); // Activar sensor
    i2c_write(fd, TCS34725_ADDR, ATIME_REG, 0xD5);  // Configuración de tiempo
    i2c_write(fd, TCS34725_ADDR, CONTROL_REG, 0x01); // Configuración de ganancia
}

void setup_mpu6050(int fd) {
    i2c_write(fd, MPU6050_ADDR, PWR_MGMT_1, 0x00); // Despertar el MPU6050
}

// ** Función para enviar datos a ThingsBoard**
void send_to_thingsboard(const char* host, const char* token, uint16_t red, uint16_t green, uint16_t blue, float accel_x, float accel_y, float accel_z) {
    char command[1024];

    // Comando de publicación con datos de los sensores
    snprintf(command, sizeof(command),
        "mosquitto_pub -d -q 1 -h %s -p 1883 -t v1/devices/me/telemetry -u \"%s\" -m \"{\\\"red\\\":%d,\\\"green\\\":%d,\\\"blue\\\":%d,\\\"accel_x\\\":%.2f,\\\"accel_y\\\":%.2f,\\\"accel_z\\\":%.2f}\"",
        host, token, red, green, blue, accel_x, accel_y, accel_z);

    // Ejecutar el comando
    system(command);
    printf("Datos enviados a ThingsBoard\n");
}

void print_usage() {
    printf("Uso: sensor_to_thingsboard -h <host> -t <token> -i <i2c_device> [-p <publish_interval>]\n");
    printf("  -h <host>            Dirección IP de ThingsBoard\n");
    printf("  -t <token>           Token del dispositivo en ThingsBoard\n");
    printf("  -i <i2c_device>      Dispositivo I2C (ejemplo: /dev/i2c-1)\n");
    printf("  -p <publish_interval> (opcional) Intervalo entre publicaciones en segundos (por defecto: 5)\n");
}

int main(int argc, char *argv[]) {
    int fd;
    int publish_interval = 5;
    char *i2c_file = NULL;
    char *host = NULL;
    char *token = NULL;

    // Parsear argumentos de línea de comandos
    if (argc < 7) {
        print_usage();
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0) {
            token = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0) {
            i2c_file = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            publish_interval = atoi(argv[++i]);
        }
    }

    if (!host || !token || !i2c_file) {
        print_usage();
        return 1;
    }

    // Abrir el bus I2C
    if ((fd = open(i2c_file, O_RDWR)) < 0) {
        perror("Error al abrir el bus I2C");
        return 1;
    }

    // Configurar los sensores
    setup_tcs34725(fd);
    setup_mpu6050(fd);

    uint16_t red, green, blue, clear;
    float accel_x, accel_y, accel_z;

    // Bucle principal
    while (1) {
        // Leer datos de los sensores
        read_tcs34725(fd, &red, &green, &blue, &clear);
        read_mpu6050(fd, &accel_x, &accel_y, &accel_z);

        // Imprimir valores de sensores (opcional)
        printf("[TCS34725] Clear: %d, Red: %d, Green: %d, Blue: %d\n", clear, red, green, blue);
        printf("[MPU6050] Accel X: %.2f g, Y: %.2f g, Z: %.2f g\n", accel_x, accel_y, accel_z);

        // Enviar datos a ThingsBoard
        send_to_thingsboard(host, token, red, green, blue, accel_x, accel_y, accel_z);

        // Esperar antes de enviar los siguientes datos
        sleep(publish_interval);
    }

    close(fd);
    return 0;
}
