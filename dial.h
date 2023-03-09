
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
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <gst/gst.h>



#define stat_extline (0x01)
#define stat_dial_timeout (0x02)
#define stat_hangup (0x03)
#define dial_complete (0x04)

uint8_t number_dialed;
uint8_t number_dialed_accum;
uint8_t trigger;


typedef enum {
    st_rotary_idle,
    st_timer_hangup,
    st_timer_dialcompl
} rotary_fsm_t;


void *tf_rotary();
