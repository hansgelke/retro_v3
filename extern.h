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


typedef enum {
    st_idle,
    st_ext_ring, // (1)External call comes in
    st_ext_accepted, //(2)local lifted receiver
    st_offhook, //(3)initiator lifted receiver to dial
    st_noerror,
    st_second_number,//(5) wait for second number
    st_outsideline,//(6) a zero was dialed to get outside line
    st_no_dial,
    st_hang_up,
    st_int_ring, // (9) Internal ring
    st_int_established // (10) An inernal connection is establishe
} ext_fsm_state_t;


void *tf_extern();
void *tf_ext_timer();
void *tf_main_fsm();
void main_fsm();
void ext_timer();
