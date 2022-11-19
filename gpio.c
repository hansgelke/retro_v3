/* ------------------------------------------------------------------
 * --  _____       ______  _____                                    -
 * -- |_   _|     |  ____|/ ____|                                   -
 * --   | |  _ __ | |__  | (___    Institute of Embedded Systems    -
 * --   | | | '_ \|  __|  \___ \   Zurich University of             -
 * --  _| |_| | | | |____ ____) |  Applied Sciences                 -
 * -- |_____|_| |_|______|_____/   8401 Winterthur, Switzerland     -
 * ------------------------------------------------------------------
 * --
 * -- File:	cli.c
 * -- Date:	05.02.2017
 * -- Author:	rosn
 * --
 * ------------------------------------------------------------------
 */
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "gpio.h"

// Hardware definitions
#define GPIO_IN 0x0
#define GPIO_OUT 0x1
#define GPIO_EDGE_FALLING 0x0
#define GPIO_EDGE_RISING 0x1
#define GPIO_HIGH_DETECT 0x2
#define GPIO_LOW_DETECT 0x3

// Macros to calculate register offsets
#define GPFSELX_OFFSET(pin) (pin / 10 * 0x04)
#define FSELX_OFFSET(pin) (pin % 10 * 3)

#define GPIO_BASE_ADDR   	   (0xfe200000) // base of GPIO page
#define GPIO_FSEL1         (0x00000004) // GPIOFSEL1 offset
#define GPIO_FSEL2         (0x00000008) // GPIOFSEL1 offset
#define GPIO_SET0          (0x0000001c) // GPIOSET0 offset
#define GPIO_CLR0          (0x00000028) //GPIOCLR0 offset
#define GPIO_LVL0          (0x00000034) //GPIOCLR0 offset

#define MCP_IODIR   (0x00) // 1=input, 0=output
#define MCP_IPOL    (0x01)
#define MCP_GPINTEN (0x02)
#define MCP_DEFVAL  (0x03)
#define MCP_INTCON  (0x04)
#define MCP_IOCON   (0x05)
#define MCP_GPPU    (0x06)
#define MCP_INTF    (0x07)
#define MCP_INTCAP  (0x08)
#define MCP_GPIO    (0x09)
#define MCP_OLAT    (0x0a)


void *virtual_gpio_base;

int fd;



/*  mmap_gpio_direction(25, GPIO_OUT);
    mmap_gpio_direction(5, GPIO_OUT);
    mmap_gpio_direction(12, GPIO_OUT);
    mmap_gpio_direction(13, GPIO_OUT);


    mmap_gpio_set(12, 1);
    mmap_gpio_set(13, 1);
    mmap_gpio_set(25, 0);
    mmap_gpio_set(5, 0);
    mmap_gpio_set(12, 0);
    mmap_gpio_set(13, 0);
    mmap_gpio_set(25, 1);
    mmap_gpio_set(5, 1);*/




/****************************************************************
 * mmap virtual base calculation
 ****************************************************************/
int mmap_virtual_base()
{
    int  m_mfd;

    if ((m_mfd = open("/dev/mem", O_RDWR)) < 0)
    {
        printf("FAIL by open /dev/mem\n");
        return m_mfd;
    }
    // http://man7.org/linux/man-pages/man2/mmap.2.html
    virtual_gpio_base = (void *)mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_WRITE, MAP_SHARED, m_mfd, GPIO_BASE_ADDR);
    close(m_mfd);

    if (virtual_gpio_base == MAP_FAILED)
    {
        return errno;
    }

    return 0;
}

/*
 * Init GPIO pins
*/
static void mmap_gpio_direction( int gpio, int direction)
{
    uint32_t *gpio_reg;


    /* set io direction */
    if (gpio <= 9)
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base);

    }
    else if ((gpio > 9) & (gpio < 20))
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_FSEL1);

    }
    else
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_FSEL2);

    }

    *gpio_reg &=  ~(0x7 << FSELX_OFFSET(gpio));              // Clear 3 bits of gpio
    *gpio_reg |= (direction << FSELX_OFFSET(gpio));

}

static void mmap_gpio_set( int gpio, int value)
{
    uint32_t *gpio_reg;

    if (value == 1)
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_SET0);
    }
    else
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_CLR0);
    }

    *gpio_reg =  (0x1 << gpio);
}

/****************************************************************
 * i2c-dev
 ****************************************************************/
void
write_ctrl_register(uint8_t device_addr, uint8_t mcp_reg, uint8_t write_data){

    uint8_t wr_buf[2];
    char *i2c_device = "/dev/i2c-4";

    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }
//Switches the I2C bus depending on I2C device address


    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 174: Failed to open I2C device \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 179: Failed to set I2C device as slave.\n");

    }

    //Write I2C Register
    wr_buf[0]=mcp_reg; // Register Address
    wr_buf[1]=write_data; //
    if (write(fd,wr_buf,2) != 2)
        printf("Error gpio.c Line 188: Failed to write bus\n");


    close(fd);

}
/***************************************************************************
 * Function Reads the I2C Register
 ***************************************************************************/

int
read_ctrl_register(uint8_t device_addr, uint8_t mcp_reg){

    uint8_t register_data = 0;
    uint8_t rd_buf[1];
    uint8_t wr_buf[1];

    char *i2c_device = "/dev/i2c-4";
//Set the i2c Bus 4 or 5 depending on the I2Cchip beeing written
//Only MATRIX_TO, MATRIX_FROM and DTMF_I2C devices are on I2c-5
// All the rest is in I2C-4
    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }


    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 217: Failed to open I2C Bus \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 222: Failed to set I2C Bus as slave.\n");
        exit(1);
    }

    wr_buf[0] = mcp_reg;
    if (write(fd,wr_buf,1) != 1)
        printf("Error gpio.c Line 228: Failed to write from i2c bus\n");


    if (read(fd,rd_buf,1) != 1) {
        printf("Error gpio.c Line 232: Failed to read i2c bus\n");

    } else {
        register_data = rd_buf[0];
    }

    close(fd);
    return register_data;
}


void set_connections(uint8_t from, uint8_t to){

//Converts numeric to bit value
    uint8_t exch_lines[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

  write_ctrl_register(MATRIX_FROM, MCP_OLAT, exch_lines[from]);
  write_ctrl_register(MATRIX_TO, MCP_OLAT, exch_lines[to]);

}

void init_gpios(){
    //Set Matrix Controllers to Output
    write_ctrl_register(MATRIX_FROM, MCP_IODIR, 0x00);
    write_ctrl_register(MATRIX_TO, MCP_IODIR, 0x00);
    //Set the AC/DC Registers to output
    write_ctrl_register(PHONE_DC, MCP_IODIR, 0x00);
    write_ctrl_register(PHONE_AC, MCP_IODIR, 0x00);
    //Set the AC/DC Registers to DC
    write_ctrl_register(PHONE_DC, MCP_OLAT, 0xff); // set to one
    write_ctrl_register(PHONE_AC, MCP_OLAT, 0x00); // set to zero
// Set the Connect control register to output and off
    write_ctrl_register(CONNECT_CTRL, MCP_IODIR, 0x00);
    write_ctrl_register(CONNECT_CTRL, MCP_OLAT, 0x00);
// DTMF and LOOP Detect are input registers
    write_ctrl_register(DTMF_READ, MCP_IODIR, 0xff);
    write_ctrl_register(LOOP_DETECT, MCP_IODIR, 0xff);


}

uint8_t
hex2lines(uint8_t hex){
    //Converts a hex number from the arguments to a single bit
    //Define Array
    uint8_t exch_lines[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    uint8_t lines =exch_lines[hex];

    return lines;
}
uint8_t
hex2notlines(uint8_t hex){
    //Converts a hex number from the arguments to a single bit
    //Returns low active bits
    uint8_t exch_lines[9] = {0xff, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    uint8_t lines =exch_lines[hex];

    return lines;
}
//
// Write Single Bit in I2C
//
void
write_mcp_bit(uint8_t device_addr, uint8_t mcp_reg , uint8_t bit_pos, char value){
    char rd_buf[0];
    char wr_buf[0];
    uint8_t cur_value = 0;
    uint8_t mask;

    char *i2c_device = "/dev/i2c-4";
//Set the i2c Bus 4 or 5 depending on the I2Cchip beeing written
//Only MATRIX_TO, MATRIX_FROM and DTMF_I2C devices are on I2c-5
// All the rest is in I2C-4
    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }

    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 335: Failed to open I2C Bus \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Failed to acquire I2C bus access and/or talk to slave.\n");
        exit(1);
    }

    wr_buf[0] = mcp_reg;
    if (write(fd,wr_buf,1) != 1)
        printf("Failed to write to the i2c bus.\n");

    if (read(fd,rd_buf,1) != 1) {
        /* ERROR HANDLING: i2c transaction failed */
        printf("Failed to read from the i2c bus.\n");
    } else {
        cur_value = rd_buf[0];
    }

    mask = 0x1 << bit_pos;
    if (value == 1){
        cur_value |= mask;
    }
    else {
        cur_value &= ~mask;
    }
    wr_buf[0] = mcp_reg; // Register Address
    wr_buf[1] = cur_value; //
    if (write(fd,wr_buf,2) != 2)
        printf("Failed to write to the i2c bus.\n");

    close(fd);
}


