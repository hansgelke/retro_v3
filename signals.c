#include "signals.h"
#include "pwm.h"

#include "gpio.h"

pthread_mutex_t signals_i2c = PTHREAD_MUTEX_INITIALIZER;


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

        //set note_idx to 0 and read out parameters for index
        note_idx = 0;
        skip_flag = false;
        //This loop goes through the table which geneartes signals.
        //A sequence of max 8 segments is possible. If less then 8 requiered segments is
        //indicated with the skip flag
        while((note_idx <= 8) && !(skip_flag)) {
            //ringer_on = melody[note_idx].ringer_on;
            //tone_on = melody[note_idx].tone_on;
            //sleep_time = melody[note_idx].duration;
            skip_flag = melody[note_idx].skip;

            g_object_set (G_OBJECT (tone_src1), "wave", 0, NULL);
            g_object_set (G_OBJECT (tone_src2), "wave", 0, NULL);
            g_object_set (G_OBJECT (tone_src1), "volume", melody[note_idx].vol, NULL);
            g_object_set (G_OBJECT (tone_src2), "volume", melody[note_idx].vol, NULL);
            g_object_set (G_OBJECT (tone_src1), "freq", melody[note_idx].freq_1, NULL);
            g_object_set (G_OBJECT (tone_src2), "freq", melody[note_idx].freq_2, NULL);

            if (melody[note_idx].tone_on) {
                gst_element_set_state (tone_pipeline, GST_STATE_PLAYING);
            }
            else {
                gst_element_set_state (tone_pipeline, GST_STATE_NULL);
            }
            //If ringer flag is set, turn on Bridge for AC generation
            if (melody[note_idx].ringer_on) {
                //write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
                pwm_reg_write(PWM_CTL, 0x81);
            }
            else {
                //write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5077);
                pwm_reg_write(PWM_CTL, 0x00);
            }
            // go to sleep for specified time after wakeup stop tone, stop ring
            usleep(melody[note_idx].duration);

            //Increment the index line of the cadence table
            note_idx = note_idx + 1 ;
        }


    }
}

/***********************************************/
/*          PLAY ANNOUCEMENTS                  */
/***********************************************/

void
*tf_play_announcements()
{
    struct sched_param para_announce;
    para_announce.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_announce);
    GstElement	*conv1;
    GstElement	*parse;
    GstElement	*resample;
    GstElement	*sink;
    //GMainLoop	*loop;
    //GstCaps *caps;

    gst_init (NULL,NULL);

    announcement_pipeline = gst_pipeline_new ("announcement_pipeline");

      resample = gst_element_factory_make ("audioresample", "resample_1");

    announcement_src = gst_element_factory_make ("filesrc", "inst_filesrc");
    //g_object_set (G_OBJECT (src), "location", "./announcement/kein_anschluss_D.wav", NULL);

    conv1 = gst_element_factory_make ("audioconvert", "inst_audioconvert1");

    parse = gst_element_factory_make ("wavparse", "inst_wavparse1");

    sink = gst_element_factory_make ("alsasink", "inst_alsasink");
    g_object_set (G_OBJECT (sink), "device", "hw:1,0", NULL);

    gst_bin_add_many (GST_BIN (announcement_pipeline), announcement_src, parse, resample, conv1, sink, NULL);

    announce_caps = gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, 44100, NULL);

    /* we link the elements together */
    if (
            !gst_element_link (announcement_src, parse) ||
            !gst_element_link (parse, conv1) ||
            /*!gst_element_link (conv1, resample) ||*/
            /*!gst_element_link (resample, sink)*/

            !gst_element_link_filtered (conv1, sink,announce_caps)


            ) {
        fprintf (stderr, "can't link elements\n");
        exit (1);
    }

    //gst_element_set_state (announcement_pipeline, GST_STATE_PLAYING);

    /* we need to run a GLib main loop to get the messages */
    announce_loop = g_main_loop_new (NULL, FALSE);

    /* Runs a main loop until g_main_loop_quit() is called on the loop */
    g_print ("Running_Announcements_...\n");
    g_main_loop_run (announce_loop);

    /* Out of the main loop, clean up nicely */
    gst_caps_unref(announce_caps);
    g_main_loop_unref (announce_loop);
    gst_object_unref (announcement_pipeline);

    return 0;
}
