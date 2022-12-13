#include "dial.h"
#include "gpio.h"


void *tf_rotary()
{

    struct sched_param para_rotary;
    para_rotary.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_rotary);

    static rotary_fsm_t rotary_state = st_rotary_idle;
    static uint8_t loop_interrupt = 0;
    static uint32_t tv_sec = 0;
    static uint32_t tv_usec = 0;
    static bool timeout = false;
    //clear any interrupt
    trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);


    while(1){ //loop and wait for interrupts
      //  loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP, timeout);

        switch (rotary_state) {

        case st_rotary_idle:
            //Arm interrupt without timeout for first pulse
            timeout = false;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);
            if (loop_interrupt > 0) {
                //Change on the loop_int line occured - switch to check timer_hangup, needs timeout
                number_dialed_accum = 1;

                rotary_state = st_timer_hangup;
            }
            else if (loop_interrupt == 0) {
                rotary_state = st_rotary_idle;
                dial_error = dial_no_dial;
            }
            else {
                rotary_state = st_rotary_idle;}
            break;

        case st_timer_hangup:
            //Arm interrupt with timeout for subsequent pulse
            tv_sec = 0;
            tv_usec = 72000;
            timeout = false;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);
            // if interrupt occured, wait in dialcompl until no more pulses come
            if (loop_interrupt > 0) {
                rotary_state = st_timer_dialcompl;
            }
            // Loop was open to long, origin hang up.
            else if (loop_interrupt == 0) {
                dial_error = hangup_error;
                rotary_state = st_rotary_idle;
            }
            // return code ff error, post semaphore for next ring cycle

            else {
                rotary_state = st_timer_hangup;
            }
            break;

        case st_timer_dialcompl:
            //Arm interrupt with timeout for following pulse
            tv_sec = 0;
            tv_usec = 80000;
            timeout = true;
            loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP, timeout);
            //Clear MCP chip interrupt line
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);

            if (loop_interrupt > 0) {
                number_dialed_accum = number_dialed_accum + 1;
                rotary_state = st_timer_hangup;
            }

            //Timeout no more dial pulses
            else if (loop_interrupt == 0) {

                number_dialed = number_dialed_accum;
                dial_error = dial_complete;
                rotary_state = st_rotary_idle;

            }
            // else continue waiting for loop interrupt

            else {
                rotary_state = st_timer_dialcompl;
            }

            break;
        }
    }
}
