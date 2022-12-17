
#include "main.h"
#include "extern.h"
#include "gpio.h"

typedef enum {
    st_ext_idle,
    st_ext_ring,
    st_ext_accepted
} ext_fsm_state_t;

ext_fsm_state_t ext_state = st_ext_idle ;
uint32_t ring_timer = MAX_RING;


void *tf_main_fsm()
{
    struct sched_param para_main_fsm;
    para_main_fsm.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_main_fsm);

    while(1){
        switch (ext_state) {
        case st_ext_idle:
            sem_init(&sem_signal,0,0);
            //printf("EXT IDLE\n");
            if (ring_timer > 0){
                ext_state = st_ext_ring;
            }
            break;

        case st_ext_ring:
            sem_post(&sem_signal);
            //printf("EXT RING\n");
            if (loop_detected()){
                ext_state = st_ext_accepted;
            }
            else if (ring_timer == 0){
                ext_state = st_ext_idle;
            }

            else  {
                ext_state = st_ext_ring;
            }
            break;

        case st_ext_accepted:
            //printf("EXT ACCEPTED\n");
            sem_init(&sem_signal,0,0);

            if (loop_detected()){
                ext_state = st_ext_accepted;
            }
            else {
                ext_state = st_ext_idle;
            }
            break;
        }

    }
}


void *tf_ext_timer()
{
    struct sched_param para_extern;
    para_extern.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_extern);



    while(1){


        //If Ring indicator reports ring signal set ring counter

        //if (mmap_gpio_read(LOOP_CLOSED_N_1) == 1) {

        usleep(10000);
        if (gpio_read(RING_INDICATOR_N) == 0){
            ring_timer = MAX_RING;
        }
        //count down delayed by usleep
        else if (ring_timer > 0) {
            ring_timer-- ;
        }
        else{
            ring_timer = 0;
        }
    }
}
