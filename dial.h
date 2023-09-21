
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
static uint8_t invalid_no[100] = {
    /*0*/    0,0,0,0,0,0,0,0,0,0,
    /*1*/    0,0,0,0,0,0,0,0,0,19,
    /*2*/    20,21,22,23,24,25,26,27,28,29,
    /*3*/    30,31,33,33,34,35,36,37,38,39,
    /*4*/    40,41,44,43,44,45,46,47,48,49,
    /*5*/    50,51,55,53,54,55,56,57,58,59,
    /*6*/    60,61,66,63,64,65,66,67,68,69,
    /*7*/    70,71,77,73,74,75,76,77,78,79,
    /*8*/    80,81,88,83,84,85,86,87,88,89,
    /*9*/    90,91,99,93,94,95,96,97,98,99,
};

void *tf_rotary();
