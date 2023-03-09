#include "menue.h"
#include "main.h"
#include "extern.h"
#include "gpio.h"
#include "signals.h"
#include "dial.h"

pthread_mutex_t dial_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_dial = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_dialcomplete = PTHREAD_COND_INITIALIZER;

extern uint8_t number_dialed;

uint8_t first_num;
ext_fsm_state_t next_state;
ext_fsm_state_t ext_state = st_idle ;

uint32_t ring_timer = 0;
uint8_t requesting_line = 0x0;
uint8_t dial_status = 0;
uint8_t dial_status_switch = 0;


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
    /******************************************************************************
    *                       State IDLE
    ******************************************************************************/

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




            ext_state = st_offhook;
        }


        break;

        /******************************************************************************
     *                       State EXTERNAL RING
     ******************************************************************************/

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

        /******************************************************************************
         *                       State External Call Accepted
         ******************************************************************************/

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

        /******************************************************************************
        *                       State OFF HOOK
        ******************************************************************************/
    case st_offhook:
        // Generate dial signal
        melody = ger_dial;
        sem_post(&sem_signal);

        pthread_cond_signal(&cond_dial);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        first_num = number_dialed;
        dial_status_switch = dial_status;
        pthread_mutex_unlock(&dial_mutex);

        // dial.c generates status codes, which are evaluated here

        switch (dial_status_switch){

        /***********************Evaluate returned Dial Status**************************************/

        case stat_dial_complete:
            //If a zero was dialed to get an outside line (10 pulses)
            if (first_num == 10){
                //Turn Dial Tone off
                sem_init(&sem_signal,0,0);
                gst_element_set_state (tone_pipeline, GST_STATE_NULL);
                ext_state = st_outsideline;
            }
            else {
                //avoids lockup situation main_fsm sent cond signal and waits for dial complete
                //and dial_fsm is waiting for cond_dial
                usleep(500);
                //remove semaphore to stop dial tone
                sem_init(&sem_signal,0,0);
                gst_element_set_state (tone_pipeline, GST_STATE_NULL);
                ext_state = st_second_number;
            }
            break;


            //        case stat_dial_timeout:
            //            //Timeout if dial is discontinued
            //            sem_init(&sem_signal,0,0);
            //            gst_element_set_state (tone_pipeline, GST_STATE_NULL);
            //            ext_state = st_no_dial;

            //            break;



        case stat_hangup:
            //someone hangs up right after dialing
            // Not implemented, maybe not necessary works anyway
            sem_init(&sem_signal,0,0);
            gst_element_set_state (tone_pipeline, GST_STATE_NULL);
            ext_state = st_hang_up;

            break;

            /******************************************************************************
            *                       State OUTSIDE LINE
           ******************************************************************************/

        case st_outsideline:


            break;
            /***************************************************/
            //                      STATE HANG UP
            /****************************************************/
        case st_hang_up:


            usleep(1000);
            ext_state = st_idle;
            break;


        }

        break;
    }


}




