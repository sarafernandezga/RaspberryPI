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

int w_len = 2;
int r_len = 6;

struct i2c_rdwr_ioctl_data packets;
struct i2c_msg messages[2];

uint8_t write_bytes[w_len];
uint8_t read_bytes[r_len];

int main() {
    char i2cFile[15];
    sprintf(i2cFile, "/dev/i2c-1");
    int fd = open(i2cFile, O_RDWR);
    ioctl(fd, I2C_SLAVE, MPU6050_ADDR);

    // Wake up the MPU6050
    write_bytes[0] = PWR_MGMT_1;
    write_bytes[1] = 0x00;
    messages[0].addr = MPU6050_ADDR;
    messages[0].flags = 0;
    messages[0].len = w_len;
    messages[0].buf = write_bytes;
    
    packets.msgs = messages;
    packets.nmsgs = 1;
    ioctl(fd, I2C_RDWR, &packets);
    
    // Request accelerometer data
    write_bytes[0] = ACCEL_XOUT_H;
    messages[0].addr = MPU6050_ADDR;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = write_bytes;
    
    messages[1].addr = MPU6050_ADDR;
    messages[1].flags = I2C_M_RD;
    messages[1].len = r_len;
    messages[1].buf = read_bytes;
    
    packets.msgs = messages;
    packets.nmsgs = 2;
    ioctl(fd, I2C_RDWR, &packets);
    
    // Convert data
    int16_t accel_x = (read_bytes[0] << 8) | read_bytes[1];
    int16_t accel_y = (read_bytes[2] << 8) | read_bytes[3];
    int16_t accel_z = (read_bytes[4] << 8) | read_bytes[5];
    
    printf("Accel X: %d\n", accel_x / 16384.0);
    printf("Accel Y: %d\n", accel_y / 16384.0);
    printf("Accel Z: %d\n", accel_z / 16384.0);
    
    close(fd);
    return 0;
}
