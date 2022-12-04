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

//Hardware definitions
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

// NEW FROM HERE //


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

#define LOOP_DETECT (0x20) // Bus0
#define PHONE_AC (0x21) // Bus0
#define PHONE_DC (0x22) // Bus0
#define CONNECT_CTRL (0x23) // Bus0
#define MATRIX_TO (0x24) // Bus1
#define MATRIX_FROM (0x25) // Bus1
#define DTMF_READ (0x26) // Bus1

//Single Bits of I2C
#define RINGER_ENABLE (0x02) // Turn Bridge on
#define EXT_LINE_RELAY (0x03) // Assert Relay for external line
#define EXT_TO_ENABLE (0x04) // Turn on To line
#define EXT_FROM_ENABLE (0x5) // Turn on From Line

#define SIGNAL_B_FROM (0x5)//Codec Output Signals
#define SIGNAL_B_TO (0x7)//Codec Output Signals



//Function declarations
int mmap_virtual_base();
int read_ctrl_register(uint8_t register_addr, uint8_t device_addr);
void write_ctrl_register(uint8_t device_addr, uint8_t register_addr, uint8_t register_data);
void set_connections(uint8_t from, uint8_t to);
uint8_t hex2lines(uint8_t hex);
uint8_t hex2notlines(uint8_t hex);
void write_mcp_bit(uint8_t device_addr, uint8_t mcp_reg , uint8_t bit_pos, char value);

void init_gpios();

