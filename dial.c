#include "dial.h"
#include "gpio.h"


void *tf_rotary()
{

    struct sched_param para_rotary;
    para_rotary.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_rotary);

    rotary_fsm_t rotary_state = st_rotary_idle;
    uint8_t loop_interrupt = 0;
    uint32_t tv_sec = 0;
    uint32_t tv_usec = 0;

    while(1){ //loop and wait for interrupts
        if (rotary_state == st_rotary_idle) {
            loop_interrupt = wait_select_notime(DC_LOOP);
        }
        loop_interrupt = wait_select(tv_sec, tv_usec, DC_LOOP);

        switch (rotary_state) {

        case st_rotary_idle:
            if (loop_interrupt > 0) {

                number_dialed_accum = number_dialed_accum + 1;
                tv_sec = 0;
                tv_usec = 72000;
                rotary_state = st_timer_hangup;}
            else if (loop_interrupt == 0) {rotary_state = st_rotary_idle;
                dial_error = dial_no_dial;
            }
            else {rotary_state = st_rotary_idle;}
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);

            break;

        case st_timer_hangup:

            // rising edge occured
            if (loop_interrupt > 0) {rotary_state = st_timer_dialcompl;
            }
            // Loop was open to long, origin hang up.
            else if (loop_interrupt == 0) {rotary_state = st_rotary_idle;
                dial_error = hangup_error;
            }
            // return code ff error, post semaphore for next ring cycle

            else {
                tv_sec = 0;
                tv_usec = 72000;
                rotary_state = st_timer_hangup;
            }
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);

            break;

        case st_timer_dialcompl:


            if (loop_interrupt > 0) {
                number_dialed_accum = number_dialed_accum + 1;
                rotary_state = st_timer_hangup;
            }

            //Timeout no more dial pulses
            else if (loop_interrupt == 0) {

                number_dialed = number_dialed_accum;
                dial_error = no_error;

                rotary_state = st_rotary_idle;

            }
            // else continue waiting for loop interrupt

            else {
                tv_sec = 0;
                tv_usec = 200000;
                rotary_state = st_timer_dialcompl;
            }
            trigger = read_ctrl_register(LOOP_DETECT, MCP_INTCAP);

            break;
        }
    }
}
