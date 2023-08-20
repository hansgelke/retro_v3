#include "dtmf.h"
#include "gpio.h"
#include "extern.h"

//The followig are defined in extern.c
extern pthread_mutex_t dial_mutex ;
extern pthread_cond_t cond_dtmf_dial ;
extern pthread_cond_t cond_dialcomplete ;

extern uint8_t dial_status;
uint8_t gpio_port;


void *tf_dtmf()
{

    struct sched_param para_dtmf;
    para_dtmf.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_dtmf);

    //static dtmf_fsm_t dtmf_state = st_dtmf_idle;
    static uint8_t loop_interrupt = 0;
    static uint32_t tv_sec = 0;
    static uint32_t tv_usec = 0;
    static bool timeout = false;



    while(1){ //loop and wait for interrupts
        //loop only if cond_dial is true



        /************************************************************************
        *                    dtmf IDLE
        ***********************************************************************/
        //Use wait_select until a DC_LOOP_INT occured (rising edgeof MCP Interrupt line)
        // A timeout means the user blocks by leaving receiver of hook more than 30s
        // loop_int > 0 load first pulse into number_dialed_accum then go to hangup
        // In hangup it is checked, if another pulse comes

        // wait for cond_dial
            pthread_mutex_lock(&dial_mutex);
            pthread_cond_wait(&cond_dtmf_dial, &dial_mutex);
            pthread_mutex_unlock(&dial_mutex);

            timeout = true;
            tv_sec = 30;
            tv_usec = 000000;
            loop_interrupt = wait_select(tv_sec, tv_usec, DTMF_INT, timeout);

            if (loop_interrupt > 0) {
                // Normal detection of interrupt (only LSB 4 bits)
                number_dialed = (0xf) & read_ctrl_register(DTMF_READ,MCP_GPIO, 5555);
                dial_status = stat_dial_complete;
                pthread_cond_signal(&cond_dialcomplete);
            }
            //if the receiver is lifted without dialing for 30s an Alarm is generated
            else if (loop_interrupt == 0) {
                dial_status = stat_nodial;
                //Message to callling function that no dial occured
                pthread_cond_signal(&cond_dialcomplete);
            }
            else {
                usleep(10000);
            }




    }
}
