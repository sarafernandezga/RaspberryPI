// This code has been developed by Sara Fernández and Yaiza Moyons with the help of AI, the slides of the subject and based 
// on the operation of the MPU6050 code of the subject Microprocessor-Based Systems created by Sara Fernández last semester

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>

#define MPU6050_ADDR 0x68
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B

int w_len = 2; //longitud de los datos a escribir(2 bytes)
int r_len = 6; //longitud de los datos a leer (6 bytes)

struct i2c_rdwr_ioctl_data packets;
struct i2c_msg messages[2];

uint8_t write_bytes[w_len]; // Buffer para los bytes a escribir
uint8_t read_bytes[r_len]; // Buffer para los bytes a leer

int main() {
    char i2cFile[15];
    sprintf(i2cFile, "/dev/i2c-1");
    int fd = open(i2cFile, O_RDWR);
    ioctl(fd, I2C_SLAVE, MPU6050_ADDR);

    // Wake up the MPU6050
    write_bytes[0] = PWR_MGMT_1;
    write_bytes[1] = 0x00; // Escribe 0x00 en el registro para despertar el sensor
    messages[0].addr = MPU6050_ADDR;  // Establece la dirección del mensaje
    messages[0].flags = 0;// Flag para operación de escritura
    messages[0].len = w_len;
    messages[0].buf = write_bytes;
    
    packets.msgs = messages; //Se mete el mensaje a la estructura de paquetes
    packets.nmsgs = 1; //Numero de mensajes a enviar
    ioctl(fd, I2C_RDWR, &packets); //Envia el mensaje para despertar al sensor
    
    // Request accelerometer data
    write_bytes[0] = ACCEL_XOUT_H;
    messages[0].addr = MPU6050_ADDR;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = write_bytes;
    
    messages[1].addr = MPU6050_ADDR;
    messages[1].flags = I2C_M_RD; //Flag para operación de lectura
    messages[1].len = r_len;
    messages[1].buf = read_bytes;
    
    packets.msgs = messages;
    packets.nmsgs = 2; //Nº mensajes totales, 1 escritura y 1 lectura
    ioctl(fd, I2C_RDWR, &packets);// Envía los mensajes para leer los datos de aceleración
    
    // Convert data
	// Para ejes x, y, z
    int16_t accel_x = (read_bytes[0] << 8) | read_bytes[1]; 
    int16_t accel_y = (read_bytes[2] << 8) | read_bytes[3];
    int16_t accel_z = (read_bytes[4] << 8) | read_bytes[5];
    
    printf("Accel X: %d\n", accel_x / 16384.0);
    printf("Accel Y: %d\n", accel_y / 16384.0);
    printf("Accel Z: %d\n", accel_z / 16384.0);
    
    close(fd); //se cierra el descriptor 
    return 0; //termina el programa
}
