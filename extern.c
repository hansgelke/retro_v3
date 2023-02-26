#include "menue.h"
#include "main.h"
#include "extern.h"
#include "gpio.h"
#include "signals.h"



ext_fsm_state_t next_state;
ext_fsm_state_t ext_state = st_ext_idle ;

uint32_t ring_timer = 0;


void *tf_main_fsm()
{
    struct sched_param para_main;
    para_main.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_main);
    sched_setscheduler(0,SCHED_OTHER, NULL);

while(1) {
    if (test_mode == true)
    {
        test_menue();
    }
    else
    {

        ext_timer();
        next_state = ext_state;
        main_fsm();
        if     (ext_state != next_state){
            printf("State changed: '%x'\n", ext_state);
        };


        usleep(10000);

    }

}
}


void ext_timer()
{
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


void main_fsm()

{

            switch (ext_state) {
            case st_ext_idle:
                write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 0, 4031);
             //   sem_init(&sem_signal,0,0);
                if (ring_timer > 0){
                    ext_state = st_ext_ring;
                }
                break;

            case st_ext_ring:
                melody = gb_ring;
                write_ctrl_register(PHONE_AC, MCP_OLAT, hex2lines(1));
              write_ctrl_register(PHONE_DC, MCP_OLAT, hex2notlines(1));
              //  sem_post(&sem_signal);

//                if (mmap_gpio_test(PICK_UP_N)){
//                    ext_state = st_ext_accepted;
                //}
                if (ring_timer == 0){
                    ext_state = st_ext_idle;
                }

                else  {
                    ext_state = st_ext_ring;
                }
                break;

            case st_ext_accepted:
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




