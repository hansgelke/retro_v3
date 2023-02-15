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

typedef enum {
    st_ext_idle,
    st_ext_ring,
    st_ext_accepted
} ext_fsm_state_t;


void *tf_extern();
void *tf_ext_timer();
void *tf_main_fsm();
void main_fsm();
void ext_timer();
