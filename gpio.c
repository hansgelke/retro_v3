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
write_ctrl_register(uint8_t busno, uint8_t device_addr, uint8_t register_addr, uint8_t write_data){

    uint8_t wr_buf[2];
    char *i2c_device = "/dev/i2c-4";

    if (busno == 0) {
        i2c_device = "/dev/i2c-4";
    }
    else if (busno == 1) {
        i2c_device = "/dev/i2c-5";
    }
    else {
        i2c_device = "/dev/i2c-4";
    }

    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 174: Failed to open I2C Bus %d \n O_RDWR", busno);
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 179: Failed to set I2C Bus %d bus as slave.\n", busno);

    }

    //Write I2C Register
    wr_buf[0]=register_addr; // Register Address
    wr_buf[1]=write_data; //
    if (write(fd,wr_buf,2) != 2)
        printf("Error gpio.c Line 188: Failed to write bus %d\n", busno);


    close(fd);

}
/***************************************************************************
 * Function Reads the I2C Register
 ***************************************************************************/

int
read_ctrl_register(uint8_t busno, uint8_t device_addr, uint8_t register_addr){

    uint8_t register_data = 0;
    uint8_t rd_buf[1];
    uint8_t wr_buf[1];

    char *i2c_device = "/dev/i2c-4";

    if (busno == 0) {
        i2c_device = "/dev/i2c-4";
    }
    else if (busno == 1) {
        i2c_device = "/dev/i2c-5";
    }
    else {
        i2c_device = "/dev/i2c-4";
    }

    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 217: Failed to open I2C Bus %d \n O_RDWR", busno);
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 222: Failed to set I2C Bus %d bus as slave.\n", busno);
        exit(1);
    }

    wr_buf[0] = register_addr;
    if (write(fd,wr_buf,1) != 1)
        printf("Error gpio.c Line 228: Failed to write from i2c bus No: %d\n", busno);


    if (read(fd,rd_buf,1) != 1) {
        printf("Error gpio.c Line 232: Failed to read i2c bus No: %d\n", busno);

    } else {
        register_data = rd_buf[0];
    }

    close(fd);
    return register_data;
}



