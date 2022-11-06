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



void *virtual_gpio_base;

int main(void) {
    char line[100];
    int  a_arg1 = 0, a_arg2 = 0, a_arg3 = 0, a_arg4;
    char s_arg[100];
    int pin_val = 0;
    int mcp_rd_val = 0;
    int ret;

    ret =mmap_virtual_base();
        if (ret != 0) {
            printf("Error: Failed to initialize GPIOs: %s\n", strerror(abs(ret)));
            exit(ret);
        }


    while(1) {
        printf("Command > ");
        fgets(line,100,stdin);
        if (line[0] == 'x') return 0;

        switch(line[0]) {
        //Intialises GPIO via file system
        // a <gpio no> <in|out>
        case 'a':
            sscanf(&line[1], "%d %s", &a_arg1, s_arg);
         //   fs_gpio_direction(a_arg1, s_arg);
            break;
            // Turns LED On or Off
            // b <gpio no> <1|0>
        case 'b':
            sscanf(&line[1], "%d %d", &a_arg1, &a_arg2);
            //fs_gpio_set(a_arg1, a_arg2);
            break;

            // Reads GPIO via Filesystem
            //b <gpio no>
        case 'c':
            sscanf(&line[1], "%d", &a_arg1);
          //  pin_val=fs_gpio_get(a_arg1);
            printf("Value of pin is %d\n", pin_val);
            break;

            // Sets GPIO direction with mmap method
            // d <gpio> <1¦0>  (1 = out, 0= in)
        case 'd':
            sscanf(&line[1], "%d %d", &a_arg1, &a_arg2);
        //    mmap_gpio_direction(a_arg1, a_arg2);
            printf("Set pin %d to %d \n", a_arg1, a_arg2);
            break;
        // Sets GPIO pin with mmap method
        // e <gpio> <1¦0>
        case 'e':
            sscanf(&line[1], "%d %d", &a_arg1, &a_arg2);
         //   mmap_gpio_set(a_arg1, a_arg2);
            printf("Pin %d is set to: %d \n", a_arg1, a_arg2);
            break;

        case 'f': //Write Ctrl Register
        // f <reg addr 0..15> <data 0..15>
            //arg1 = Bus Number
            //arg2 = Write Data
            //arg3 = Device Address
            //arg4 = Register Address
            sscanf(&line[1], "%x %x %x %x", &a_arg1, &a_arg2, &a_arg3, &a_arg4);
            write_ctrl_register(a_arg1, a_arg2, a_arg3, a_arg4);
            printf("BusNo:%x Device:%x Register:%x is set to:%x \n", a_arg1, a_arg2, a_arg3, a_arg4);

            break;

        case 'g'://Read Ctrl Register
        // g <reg addr 0..15>
            //arg1= Device Address
            //arg2= Register Address
            sscanf(&line[1], "%x %x %x", &a_arg1, &a_arg2, &a_arg3);
            mcp_rd_val = read_ctrl_register(a_arg1,a_arg2, a_arg3);
            printf("BusNo:%d Device:%x Register:%x Contains:%x \n", a_arg1, a_arg2, a_arg3, mcp_rd_val);
            break;


        default:
            printf("Unknown command '%c'\n", line[0]);
        }
    }
    return(0);
}


