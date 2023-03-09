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
#define stat_dial_complete (0x01)
#define stat_dial_timeout (0x02)
#define stat_hangup (0x03)

sem_t sem_dial;
sem_t sem_dial_complete;

typedef enum {
    st_idle,
    st_ext_ring,
    st_ext_accepted,
    st_offhook,
    st_noerror,
    st_second_number,
    st_outsideline,
    st_no_dial,
    st_hang_up
} ext_fsm_state_t;


void *tf_extern();
void *tf_ext_timer();
void *tf_main_fsm();
void main_fsm();
void ext_timer();
