
#include "main.h"
#include "extern.h"
#include "gpio.h"
#include "signals.h"

typedef enum {
    st_ext_idle,
    st_ext_ring,
    st_ext_accepted
} ext_fsm_state_t;

typedef struct{
    bool ring_timer_expired;
    bool loop_closed;
    bool ring_indicator;
} main_fsm_event_t;

//main_fsm_event_t *main_fsm_event;


ext_fsm_state_t ext_state = st_ext_idle ;
uint32_t ring_timer = 0;
main_fsm_event_t main_fsm_event, main_fsm_event_last;



void main_fsm()
{
    switch (ext_state) {
    case st_ext_idle:
        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 0, 4031);
        //sem_init(&sem_signal,0,0);
        printf("EXT IDLE\n");
        if (main_fsm_event.ring_timer_expired == 0){



            ext_state = st_ext_ring;
        }
        break;

    case st_ext_ring:
        printf("EXT RING\n");
        melody = gb_ring;
        sem_post(&sem_signal);

        if (loop_detected()){
            ext_state = st_ext_accepted;
        }
        else if (main_fsm_event.ring_timer_expired){
            ext_state = st_ext_idle;
        }

        else  {
            ext_state = st_ext_ring;
        }
        break;

    case st_ext_accepted:
        printf("EXT ACCEPTED\n");
        sem_init(&sem_signal,0,0);
        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 1, 4057);

        if (loop_detected()){
            ext_state = st_ext_accepted;
        }
        else {
            ext_state = st_ext_idle;
        }
        break;
    }

}

void ext_timer()
{
    if (ring_timer == 0){
        main_fsm_event.ring_timer_expired = true;
    }
    else {main_fsm_event.ring_timer_expired = false;}

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


void *tf_main_fsm()

{
    struct sched_param para_main_fsm;
    para_main_fsm.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_main_fsm);

    main_fsm_event.loop_closed = false;
    main_fsm_event.ring_timer_expired = false;
    main_fsm_event.ring_indicator = false;
    //    ring_timer = MAX_RING;


    main_fsm_event_last = main_fsm_event;
    ext_state = st_ext_idle ;


    while(1)
    {
        main_fsm_event_last = main_fsm_event;
        ext_timer();
        main_fsm_event.loop_closed = loop_detected();
        if (gpio_read(RING_INDICATOR_N) == 0)
        {
            main_fsm_event.ring_indicator = true;

        }
        else {
            main_fsm_event.ring_indicator = false;
        }

        usleep(10000);

        if ((main_fsm_event_last.loop_closed != main_fsm_event.loop_closed) ||
                (main_fsm_event_last.ring_timer_expired != main_fsm_event.ring_timer_expired)||
                (main_fsm_event_last.ring_indicator != main_fsm_event.ring_indicator))
        {
            main_fsm();
        }
        //usleep(10000);

    }


}



