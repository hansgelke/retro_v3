#include "dial.h"
#include "gpio.h"
#include "extern.h"

//The followig are defined in extern.c
extern pthread_mutex_t dial_mutex ;
extern pthread_cond_t cond_dial ;
extern pthread_cond_t cond_dialcomplete ;

extern uint8_t dial_status;


void *tf_rotary()
{

    struct sched_param para_rotary;
    para_rotary.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_rotary);

    //static rotary_fsm_t rotary_state = st_rotary_idle;
    static uint8_t loop_interrupt = 0;
    static uint32_t tv_sec = 0;
    static uint32_t tv_usec = 0;
    static bool timeout = false;
    //clear any interrupt
    trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP, 118);


    while(1){ //loop and wait for interrupts
        //loop only if semaphore is set
//        pthread_mutex_lock(&dial_mutex);
//        pthread_cond_wait(&cond_dial, &dial_mutex);
//        pthread_mutex_unlock(&dial_mutex);
        switch (rotary_state) {

        /************************************************************************
        *                    ROTARY IDLE
        ***********************************************************************/
        //Wait until a DC_LOOP_INT occured (rising edgeof MCP Interrupt line)
        // A timeout means the user blocks by leaving receiver of hook(not yet implemented)
        // loop_int > 0 load first pulse into number_dialed_accum then go to hangup
        // In hangup it is checked, if another pulse comes
        case st_rotary_idle:
            //Arm interrupt without currently no timeout for first pulse
            timeout = false;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP_INT, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP, 1031);
            if (loop_interrupt > 0) {
                //Change on the loop_int line occured - switch to check timer_hangup, needs timeout
                number_dialed_accum = 1;

                rotary_state = st_timer_hangup;
            }
            else if (loop_interrupt == 0) {

                rotary_state = st_rotary_idle;
                dial_status = stat_dial_timeout;
            }
            else {
                rotary_state = st_rotary_idle;}
            break;
            /************************************************************************
*                     TIMER HANGUP
***********************************************************************/


        case st_timer_hangup:
            //Enable interrupt with timeout. A timeout means user hangup instead of dialing.
            // wait_select return: -1 Failure, 0 = timeout, 0> success
            // In st_dialcomplete the number is accumulated, or if no more pulses come, a completion
            // of the dial sequence is entered
            tv_sec = 1;
            tv_usec = 000000;
            timeout = true;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP_INT, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP, 1053);
            // if interrupt occured, wait in dialcompl until no more pulses come
            if (loop_interrupt > 0) {
                rotary_state = st_timer_dialcompl;
            }
            // Loop was open to long, origin hang up.
            else if (loop_interrupt == 0) {
                dial_status = stat_hangup;
                rotary_state = st_rotary_idle;
            }
            // return code ff error, post semaphore for next ring cycle

            else {
                rotary_state = st_timer_hangup;
            }
            break;

            /************************************************************************
            *                   DIAL COMPLETE
            ***********************************************************************/

        case st_timer_dialcompl:
            //Arm interrupt with timeout. A timeout means, there are no more dial pulses
            //dialing is complete
            tv_sec = 1;
            tv_usec = 500000;
            timeout = true;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP_INT, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP, 1078);

            if (loop_interrupt > 0) {
                number_dialed_accum = number_dialed_accum + 1;
                rotary_state = st_timer_hangup;
            }

            //Timeout no more dial pulses
            else if (loop_interrupt == 0) {

                number_dialed = number_dialed_accum;
                dial_status = dial_complete;
                //Signal main FSM that the number is complete now
                rotary_state = st_rotary_idle;
                dial_elapsed = true;

            }
            // else continue waiting for loop interrupt

            else {
                rotary_state = st_timer_dialcompl;
            }

            break;
        }
    }
}
