#include "dial.h"
#include "gpio.h"
#include "extern.h"

//The followig are defined in extern.c
extern pthread_mutex_t dial_mutex ;
extern pthread_cond_t cond_dial ;
extern pthread_cond_t cond_dialcomplete ;

extern uint8_t dial_status;
uint8_t line2gpio[8] = {27, 22, 23, 24, 9, 25, 17, 5};
uint8_t gpio_port;


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



    while(1){ //loop and wait for interrupts
        //loop only if semaphore is set

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


            pthread_mutex_lock(&dial_mutex);
            pthread_cond_wait(&cond_dial, &dial_mutex);
            pthread_mutex_unlock(&dial_mutex);


            timeout = true;
            tv_sec = 30;
            tv_usec = 000000;
            loop_interrupt = wait_select(tv_sec, tv_usec, line2gpio[origin_number], timeout);

            //The interrupt was caused by hangup the receiver without dialing

            if (loop_interrupt > 0) {
                //Change on the loop_int line occured - switch to check timer_hangup, needs timeout
                number_dialed_accum = 1;
                dial_status = stat_open;
                // Message to calling function that dial is complete
                //pthread_cond_signal(&cond_dialcomplete);
                rotary_state = st_loop_open;
            }
            //if the receiver is lifted without dialing for 30s an Alarm is generated
            else if (loop_interrupt == 0) {
                dial_status = stat_nodial;
                //Message to callling function that no dial occured
                pthread_cond_signal(&cond_dialcomplete);
                rotary_state = st_rotary_idle;
            }
            else {
                rotary_state = st_rotary_idle;}
            break;

/************************************************************************
*                     LOOP OPEN
***********************************************************************/


        case st_loop_open:
            //Enable interrupt with timeout. A timeout means user hangup instead of dialing.
            // wait_select return: -1 Failure, 0 = timeout, 0> success
            // In st_loop_closed the number is accumulated, or if no more pulses come, a completion
            // of the dial sequence is entered
            tv_sec = 0;
            tv_usec =150000;
            timeout = true;
            loop_interrupt = wait_select(tv_sec, tv_usec, line2gpio[origin_number], timeout);

            // if interrupt occured, wait in dialcompl until no more pulses come
            if (loop_interrupt > 0) {
                rotary_state = st_loop_closed;
            }
            // Loop was open longer than 150 ms, out of spec for dial pulse ->origin hang up.
            else if (loop_interrupt == 0) {
                dial_status = stat_hangup;
                pthread_cond_signal(&cond_dialcomplete);
                rotary_state = st_rotary_idle;
            }
            // return code ff error, post semaphore for next ring cycle

            else {
                rotary_state = st_loop_open;
            }
            break;

            /************************************************************************
            *                   LOOP CLOSED
            ***********************************************************************/

        case st_loop_closed:
            //Arm interrupt with timeout. A timeout means, there are no more dial pulses
            //dialing is complete
            tv_sec = 0;
            tv_usec = 150000;
            timeout = true;
            loop_interrupt = wait_select(tv_sec, tv_usec, line2gpio[origin_number], timeout);


            if (loop_interrupt > 0) {
                number_dialed_accum = number_dialed_accum + 1;
                rotary_state = st_loop_open;
            }

            //Timeout no more dial pulses
            else if (loop_interrupt == 0) {

                //>>>>>>>> GOTO st_rotary_idle

                number_dialed = number_dialed_accum;
                dial_status = stat_dial_complete;
                //Signal main FSM that the number is complete now
                pthread_cond_signal(&cond_dialcomplete);

                rotary_state = st_rotary_idle;

            }
            // else continue waiting for loop interrupt

            else {
                rotary_state = st_loop_closed;
            }

            break;
        }
    }
}
