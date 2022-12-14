#include "signals.h"
#include "pwm.h"

#include "gpio.h"


void *tf_generate_signals()
{
    struct sched_param para_ring;
    para_ring.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_ring);

    /* Define melody english ring melody */
    //  note_t *melody = gb_ring;
    // Index for playing tones
    int note_idx = 0 ;

    //uint32_t sleep_time;
    //gdouble freq_1;
    //gdouble freq_2;
    //bool ringer_on;
    //bool tone_on;

    while(1){ //Task waits for signal semaphore to be posted

        /*****************************************************************/
        //   CONTROL THE RINGING MELODY
        /******************************************************************/

        //Wait in this state for a request to play a tone
        sem_wait(&sem_signal);
        sem_post(&sem_signal);//Post for next cycle



        //pwm0_reg_write(PWM_CTL, 0x0);
        //write_mcp_bit(MCP_OLAT, CNTRL_REG, AC, 0);
        //write_mcp_bit(MCP_OLAT, CNTRL_REG, DC, 1);
        g_object_set (G_OBJECT (tone_src1), "volume", 0.4, NULL);
        g_object_set (G_OBJECT (tone_src2), "volume", 0.4, NULL);
        g_object_set (G_OBJECT (tone_src1), "wave", 0, NULL);
        g_object_set (G_OBJECT (tone_src2), "wave", 0, NULL);
        //set note_idx to 0 and read out parameters for index
        note_idx = 0;
        skip_flag = false;
        //This loop goes through the table which geneartes signals.
        //A sequence of max 8 segments is possible. If less then 8 requiered segments is
        //indicted with the skip flag
        while((note_idx <= 8) && !(skip_flag)) {
            //ringer_on = melody[note_idx].ringer_on;
            //tone_on = melody[note_idx].tone_on;
            //sleep_time = melody[note_idx].duration;
            skip_flag = melody[note_idx].skip;
            g_object_set (G_OBJECT (tone_src1), "freq", melody[note_idx].freq_1, NULL);
            g_object_set (G_OBJECT (tone_src2), "freq", melody[note_idx].freq_2, NULL);

            if (melody[note_idx].tone_on) {
                gst_element_set_state (tone_pipeline, GST_STATE_PLAYING);
            }
            else
            {gst_element_set_state (tone_pipeline, GST_STATE_NULL);}

            //If ringer flag is set, turn on AC for Ringer and PWM
            if (melody[note_idx].ringer_on) {
                //pwm_reg_write(PWM_CTL, 0x81);
                //write_ctrl_register(PHONE_AC, MCP_OLAT, hex2lines(1));
                //write_ctrl_register(PHONE_DC, MCP_OLAT, hex2notlines(1));
                //write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1);
                //sem_post(&sem_pwmon); // Post semaphore to start

            }



            else {
                //                //Turn PWM and AC off and withdraw semaphore
                pwm_reg_write(PWM_CTL, 0x00);
                write_ctrl_register(PHONE_AC, MCP_OLAT, hex2lines(0));
                write_ctrl_register(PHONE_DC, MCP_OLAT, hex2notlines(0));
                write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1);

            }

            // go to sleep for specified time after wakeup stop tone, stop ring
            usleep(melody[note_idx].duration);

            //Increment the index line of the cadence table
            note_idx = note_idx + 1 ;
        }


    }
}
