 /* --
 * -- File:	gpio.h
 * -- Date:	09.12.2022
 * -- Author:	gelk
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
#include <pthread.h>
#include <stdbool.h>
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

#define GPIO_BASE_ADDR   (0xfe200000) // base of GPIO page
#define GPIO_FSEL1         (0x00000004) // GPIOFSEL1 offset
#define GPIO_FSEL2         (0x00000008) // GPIOFSEL1 offset
#define GPIO_SET0          (0x0000001c) // GPIOSET0 offset
#define GPIO_CLR0          (0x00000028) //GPIOCLR0 offset
#define GPIO_LVL0          (0x00000034) //GPIOLVL0 offset

#define DC_LOOP_INT (0x06)
#define GPIO26 (0x1a)
#define LOOP_CLOSED_N_1 (27)
#define LOOP_CLOSED_N_2 (22)
#define LOOP_CLOSED_N_3 (23)
#define LOOP_CLOSED_N_4 (24)
#define LOOP_CLOSED_N_5 (9)
#define LOOP_CLOSED_N_6 (25)
#define LOOP_CLOSED_N_7 (17)
#define LOOP_CLOSED_N_8 (5)
#define RING_INDICATOR_N (13)
#define DTMF_INT (16)
#define PICK_UP_N (4)
#define LOOP_MASK (0x0bc20220)
#define LINE_1 (0x08000000)
#define LINE_2 (0x00400000)
#define LINE_3 (0x00800000)
#define LINE_4 (0x01000000)
#define LINE_5 (0x00000020)
#define LINE_6 (0x02000000)
#define LINE_7 (0x00020000)
#define LINE_8 (0x00000020)




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
uint8_t read_ctrl_register(uint8_t register_addr, uint8_t device_addr, uint32_t id);
void write_ctrl_register(uint8_t device_addr, uint8_t register_addr, uint8_t register_data);
void set_connections(uint8_t from, uint8_t to);
uint8_t hex2lines(uint8_t hex);
uint8_t hex2notlines(uint8_t hex);
void write_mcp_bit(uint8_t device_addr, uint8_t mcp_reg , uint8_t bit_pos, char value, uint32_t id);
int8_t wait_select(uint8_t sec, uint8_t usec, uint8_t gpio, bool timeout);
int8_t file_gpio_init (uint8_t gpio, char *direction);
uint32_t mmap_lvl_read(void);
void mmap_gpio_set( uint8_t gpio, uint8_t value);
int32_t gpio_read (uint32_t gpio);
bool mmap_gpio_test(uint8_t gpio);
void ac_on (bool acon, uint8_t line_no);
void set_ext_connect(uint8_t from);
uint8_t line_requesting();



bool loop_detected ();

void init_gpios();

