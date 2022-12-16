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


#define MAX_RING (1000)


void *tf_extern();
void *tf_ext_timer();
void *tf_main_fsm();


