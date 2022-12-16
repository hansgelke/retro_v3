
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
            if (ring_timer > 0){
                ext_state = st_ext_ring;

            }


            break;

        case st_ext_ring:
            if (ring_timer == 0){
                ext_state = st_ext_idle;}
            else if (loop_detected()){
                ext_state = st_ext_accepted;
            }
            else  {
                ext_state = st_ext_accepted;

            }

            break;

        case st_ext_accepted:

            ext_state = st_ext_idle;

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
        // if (mmap_gpio_read(RING_INDICATOR) == 1) {
        if (gpio_read(LOOP_CLOSED_N_1) == 1){
            ring_timer = MAX_RING;
        }
        //count down delayed by usleep
        if (ring_timer > 0) {
            usleep(500000);
            ring_timer-- ;
        }
        else{
            ring_timer = 0;
        }

    }






}
