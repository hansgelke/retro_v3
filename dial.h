
 /* --
 * -- File:	cli.c
 * -- Date:	10.12.2022
 * -- Author:	gelk
 * --
 * ------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <gst/gst.h>

#define loop_high_error (0xfc)
#define loop_low_error (0xfb)
#define hangup_error (0xfd)
#define dial_no_dial (0xfa)
#define no_error (0x00)

uint8_t dial_error;
uint8_t number_dialed;
uint8_t number_dialed_accum;
uint8_t trigger;


typedef enum {
    st_rotary_idle,
    st_timer_hangup,
    st_timer_dialcompl
} rotary_fsm_t;

void *tf_rotary();
