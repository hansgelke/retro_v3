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
#include "dtmf.h"
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

    //To get test menues, set test_mode true
    test_mode = false;
    init_gpios();
    init_pwm();
    sem_init(&sem_dtmf,0,0);
    sem_init(&sem_signal,0,0);
    return_to_idle();

    write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 1, 3097);




    /************************************************************
     *************** Initialize Tasks ***************************
     ************************************************************/

    pthread_t t_pwm, t_tone_gen, t_generate_signals, t_rotary, t_dtmf, t_main_fsm, t_play_dtmf;
    uint8_t iret1;

    iret1 = pthread_create(&t_pwm, NULL, &tf_pwm, NULL);
    iret1 = pthread_create(&t_rotary, NULL, &tf_rotary, NULL);
    iret1 = pthread_create(&t_dtmf, NULL, &tf_dtmf, NULL);
    iret1 = pthread_create(&t_main_fsm, NULL, &tf_main_fsm, NULL);
    iret1 = pthread_create(&t_tone_gen, NULL, &tf_tone_gen, NULL);
    iret1 = pthread_create(&t_generate_signals, NULL, &tf_generate_signals, NULL);
    iret1 = pthread_create(&t_play_dtmf, NULL, &tf_play_dtmf, NULL);
    usleep(10000);


    pthread_join(t_pwm, NULL);
    pthread_join(t_rotary, NULL);
    pthread_join(t_dtmf, NULL);
    pthread_join(t_main_fsm, NULL);
    pthread_join(t_tone_gen, NULL);
    pthread_join(t_generate_signals, NULL);
    pthread_join(t_play_dtmf, NULL);

}





