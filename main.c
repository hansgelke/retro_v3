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
        if (strcmp(line, "x") == 0){
            return 0;
        }


        else if (strcmp(line, "wr_i2c") == 0){
            // f <reg addr 0..15> <data 0..15>
            //arg1 = Bus Number
            //arg2 = Write Data
            //arg3 = Device Address
            //arg4 = Register Address
            sscanf(&line[6], "%x %x %x %x", &a_arg1, &a_arg2, &a_arg3, &a_arg4);
            write_ctrl_register(a_arg1, a_arg2, a_arg3, a_arg4);
            printf("BusNo:%x Device:%x Register:%x is set to:%x \n", a_arg1, a_arg2, a_arg3, a_arg4);

        }


        else if (strcmp(line, "rd_i2c") == 0){
            // g <reg addr 0..15>
            //arg1= Device Address
            //arg2= Register Address
            sscanf(&line[6], "%x %x %x", &a_arg1, &a_arg2, &a_arg3);
            mcp_rd_val = read_ctrl_register(a_arg1,a_arg2, a_arg3);
            printf("BusNo:%d Device:%x Register:%x Contains:%x \n", a_arg1, a_arg2, a_arg3, mcp_rd_val);
        }
        else {
            printf("Unknown command '%c'\n", line[0]);
            return(0);
        }

    }
}

