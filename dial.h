
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
#define stat_nodial (0x02)
#define stat_hangup (0x03)
#define stat_open (0x04)
#define stat_compl_rotary (0x05)
#define stat_compl_dtmf (0x06)


uint8_t number_dialed;
uint8_t number_dialed_accum;
uint8_t line_pointer;



typedef enum {
    st_rotary_idle,
    st_loop_open,
    st_loop_closed
} rotary_fsm_t;

rotary_fsm_t rotary_state;

//static uint8_t line2gpio[8] = {27, 22, 23, 24, 9, 25, 17, 5};

void *tf_rotary();
