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
uint8_t destination_number;


ext_fsm_state_t next_state;
ext_fsm_state_t ext_state = st_idle ;

uint32_t ring_timer = 0;
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

        usleep(10000);
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
                printf("State changed: '%x' Origin: '%x' \n", ext_state, origin_number);
            };




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
    //uint8_t iac;
    /******************************************************************************
    *                       State IDLE
    ******************************************************************************/

    switch (ext_state) {
    case st_idle:
        origin_number = line_requesting();
        //>>>>> GOTO st_ext_ring
        if (ring_timer > 0){
            melody = gb_ring;
            ac_on(true,1);
            sem_post(&sem_signal);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
            ext_state = st_ext_ring;
        }
        else if (origin_number != 0xff) {
            melody = ger_dial;
            sem_post(&sem_signal);
            //set matrix to output dial tone
            write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 1, 3097);
            write_mcp_bit(MATRIX_FROM, MCP_OLAT, origin_number, 1, 3098);
            pthread_mutex_lock(&dial_mutex);
            pthread_cond_signal(&cond_dial);
            pthread_mutex_unlock(&dial_mutex);



            //ext_state = st_debounce;
            ext_state = st_offhook;

        }
        else ext_state = st_idle;

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
//TEMPORARY DISABLED
//        if (mmap_gpio_test(LOOP_CLOSED_N_1) == true){
//            //If loop is opened, hang up external line
//            //>>>>>>>>>>  GO TO IDLE  >>>>>>>>>>>>>>>>>>>>
//            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 0, 4057);
//            return_to_idle();
//            ext_state = st_idle;
//        }
//        else {
            ext_state = st_ext_accepted;

//        }
//        break;

        /******************************************************************************
        *                       State OFF HOOK
        ******************************************************************************/
    case st_offhook:

        pthread_mutex_lock(&dial_mutex);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        pthread_mutex_unlock(&dial_mutex);

        first_num = number_dialed;
        dial_status_switch = dial_status;

        switch (dial_status_switch) {
        case stat_dial_complete:


            if(first_num == 0x10){
                ext_state = st_outsideline;
            }
            else {
                //>>>>GOTO st_second_number
                //Turn Tones off

                sem_init(&sem_signal,0,0);

                pthread_mutex_lock(&dial_mutex);
                pthread_cond_signal(&cond_dial);
                pthread_mutex_unlock(&dial_mutex);
                ext_state = st_second_number;


            }
            break;

        case stat_nodial:
            return_to_idle();
            ext_state = st_idle;

            break;

        case stat_hangup:
            return_to_idle();

            ext_state = st_idle;

            break;


        }
        break;

        /******************************************************************************
        *                       State Second Number
        ******************************************************************************/

    case st_second_number:
        pthread_mutex_lock(&dial_mutex);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        pthread_mutex_unlock(&dial_mutex);

        destination_number = number_dialed;
        dial_status_switch = dial_status;
        //>>>>> GOTO st_int_ring
        melody = gb_ring;
        ac_on(true,destination_number);
        sem_post(&sem_signal);
        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
        ext_state = st_int_ring;


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
        return_to_idle();
        ext_state = st_idle;
        break;

        /***************************************************/
        //                      STATE INTERN RING
        /****************************************************/


    case st_int_ring:
        if (mmap_gpio_test(PICK_UP_N) == true){
            //>>>> GOTO st_int_accepted
            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
            //Turn all lines back to DC mode
            write_ctrl_register(PHONE_AC, MCP_OLAT, 0x00);
            write_ctrl_register(PHONE_DC, MCP_OLAT, 0xff);

            set_connections(origin_number, destination_number);

            ext_state = st_int_accepted;

        }
        else if (line_requesting() == 0xff){
            //>>>> GOTO st_idle

            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
            //Turn all lines back to DC mode
            return_to_idle();
            ext_state = st_idle;
        }
        else {
            ext_state = st_int_ring;
        }
        break;

        /***************************************************/
        //                      Inetrnal Call Accepted
        /****************************************************/

    case st_int_accepted:
        //if all participants are leaving call go to idle
        if (line_requesting() == 0xff){

            //>>>>>>>>>>  GO TO st_idle  >>>>>>>>>>>>>>>>>>>>

            return_to_idle();
            ext_state = st_idle;
        }
        else {
            ext_state = st_int_accepted;

        }
        break;

        /***************************************************/
        //                      SWITCH DEFAULT STATE
        /****************************************************/

    default:

        return_to_idle();
        ext_state = st_idle;

    }

}





