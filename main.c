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
#include <pthread.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include "gpio.h"
#include "pwm.h"
#include "main.h"
#include "tones.h"
#include "signals.h"
#include "dial.h"
#include "extern.h"
#include "menue.h"



// Hardware definitions
#define GPIO_IN 0x0
#define GPIO_OUT 0x1
#define GPIO_EDGE_FALLING 0x0
#define GPIO_EDGE_RISING 0x1
#define GPIO_HIGH_DETECT 0x2
#define GPIO_LOW_DETECT 0x3

void *virtual_gpio_base;

int main(void) {

    init_gpios();
    init_pwm();
    test_mode = true;

    /************************************************************
     *************** Initialize Tasks ***************************
     ************************************************************/

    pthread_t t_pwm, t_tone_gen, t_generate_signals, t_rotary, t_main_fsm ;
    uint8_t iret1;

    iret1 = pthread_create(&t_pwm, NULL, &tf_pwm, NULL);
    iret1 = pthread_create(&t_rotary, NULL, &tf_rotary, NULL);
    iret1 = pthread_create(&t_main_fsm, NULL, &tf_main_fsm, NULL);
    iret1 = pthread_create(&t_tone_gen, NULL, &tf_tone_gen, NULL);
    iret1 = pthread_create(&t_generate_signals, NULL, &tf_generate_signals, NULL);
    usleep(10000);


    pthread_join(t_pwm, NULL);
    pthread_join(t_rotary, NULL);
    pthread_join(t_main_fsm, NULL);
    pthread_join(t_tone_gen, NULL);
    pthread_join(t_generate_signals, NULL);

}





