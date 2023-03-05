#include "menue.h"
#include "main.h"
#include "extern.h"
#include "gpio.h"
#include "signals.h"



ext_fsm_state_t next_state;
ext_fsm_state_t ext_state = st_idle ;

uint32_t ring_timer = 0;
uint8_t requesting_line = 0x0;


void *tf_main_fsm()
{
    struct sched_param para_main;
    para_main.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_main);
    sched_setscheduler(0,SCHED_OTHER, NULL);
    sem_init(&sem_signal,0,0);


while(1) {
    if (test_mode == true)
    {
        test_menue();
    }
    else
    {

        ext_timer();
        next_state = ext_state;
        requesting_line = line_requesting();
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
            case st_idle:
                if (ring_timer > 0){       
              //>>>>>>>> Go TO EXT_RING  >>>>>

                    melody = gb_ring;
                    ac_on(true,1);
                    sem_post(&sem_signal);
                    write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
                    ext_state = st_ext_ring;
                }
                else if (requesting_line != 0xff) {
                    //>>>>>>>> Go TO LIFTED  >>>>>>




                    ext_state = st_lifted;
                }


                break;

            case st_ext_ring:

               if (mmap_gpio_test(PICK_UP_N) == true){
                //>>>>>>>> Go TO ACCEPTED  >>>>>>

                   sem_init(&sem_signal,0,0);
                   write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
                   write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 1, 4057);
                   ac_on(false,1);
                   ring_timer = 0;
                   set_ext_connect(0);

                  ext_state = st_ext_accepted;
                }
                else if (ring_timer == 0){
                   //>>>>>>>>>>>>>>>>>GO TO IDLE >>>>>>>>>>>>>>>>>>>>
                    //turn ringing of if it goes to idle state
                       sem_init(&sem_signal,0,0);
                       write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);

                    ext_state = st_idle;
                }

                else  {
                    ext_state = st_ext_ring;
                }
                break;

            case st_ext_accepted:

                if (mmap_gpio_test(LOOP_CLOSED_N_1) == true){
                    //If loop is opened, hang up external line
                    //>>>>>>>>>>  GO TO IDLE  >>>>>>>>>>>>>>>>>>>>
                    write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 0, 4057);
                    ext_state = st_idle;
                }
                else {
                     ext_state = st_ext_accepted;

                }
                break;

            case st_lifted:
                if (requesting_line == 0xff) {

                    ext_state = st_idle;

                }

                break;
            }


        }




