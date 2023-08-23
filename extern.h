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



#define MAX_RING (500)

sem_t sem_dial;
sem_t sem_dial_complete;
uint8_t origin_number;
uint8_t dtmf_rd_idx;
uint8_t dtmf_wr_idx;
//bool debounce_flag = false;




typedef enum {
    st_idle,//(0)
    st_ext_ring, // (1)External call comes in
    st_ext_accepted, //(2)local lifted receiver
    st_offhook, //(3)initiator lifted receiver to dial
    st_noerror,//(4)
    st_second_number,//(5) wait for second number
    st_outsideline_rotary,//(6) get outside line for rotary phone
    st_outsideline_dtmf,//(7) get outside line for dtmf phone
    st_no_dial,//(8)
    st_hang_up,//(9)
    st_int_ring, // (10) Internal ring
    st_int_accepted, // (11) An inernal connection was accepted
    st_debounce //(12)
} ext_fsm_state_t;


void *tf_extern();
void *tf_ext_timer();
void *tf_main_fsm();
void main_fsm();
void ext_timer();
