#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include <semaphore.h>
#include "dial.h"
#include "tones.h"
#include "main.h"
#include "extern.h"
#include "gpio.h"

gfloat freq_low;
gfloat freq_high;

uint8_t mfv_buffer[32];
//Outputs correct tones
static gdouble dtmf_freq_low[11] = {918.0, 680.0, 680.0, 680.0, 752.0, 752.0, 752.0, 831.0, 831.0, 831.0, 918.0};
static gdouble dtmf_freq_high[11] = {1304.0, 1180.0, 1304.0, 1441.0, 1180.0, 1304.0, 1441.0, 1180.0, 1304.0, 1441.0, 1304.0};

//Temporary to test on Fritzbox
//static gdouble dtmf_freq_low[11] = {918.0, 680.0, 680.0, 680.0, 752.0, 752.0, 752.0, 831.0, 831.0, 918.0, 918.0};
//static gdouble dtmf_freq_high[11] = {1304.0, 1180.0, 1304.0, 1441.0, 1180.0, 1304.0, 1441.0, 1180.0, 1304.0, 1180.0, 1304.0};


//Otriginal Frequencies 9=*
//static gdouble dtmf_freq_low[11] = {941.0, 697.0, 697.0, 697.0, 770.0, 770.0, 770.0, 852.0, 852.0, 941.0, 941.0};
//static gdouble dtmf_freq_high[11] = {1336.0, 1209.0, 1336.0, 1477.0, 1209.0, 1336.0, 1477.0, 1209.0, 1336.0, 1209.0, 1336.0};



/****************************************************************
 *                   GENERATION FOR ALL SIGNAL TONES
 ****************************************************************/
void *tf_tone_gen()
{
    struct sched_param para_tone;
    para_tone.sched_priority = 50;
    sched_setscheduler(0,SCHED_FIFO, &para_tone);
    gst_init(NULL,NULL);

    //wait for dial.c to send condition and parameters
    //                pthread_mutex_lock(&tones_mutex);
    //                //pthread_cond_wait(&cond_generate_tone, &tones_mutex);
    //                freq_1 = glob_freq_1;
    //                freq_2 = glob_freq_2;
    //                pthread_mutex_unlock(&tones_mutex);


    /***************************************************************/
    /*              CREATE AND SET ELEMENTS      */
    /*****************************************************************/

    queue_audio_1 = gst_element_factory_make("queue", "queue_audio_1");
    queue_audio_2 = gst_element_factory_make("queue", "queue_audio_2");
    usleep (700000);

    tone_src1 = gst_element_factory_make ("audiotestsrc", "src1");
    g_object_set (G_OBJECT (tone_src1), "wave", 0, NULL);
    g_object_set (G_OBJECT (tone_src1), "volume", 0.0, NULL);
    // frequency object set is later reassigned in ring controller
    //g_object_set (G_OBJECT (tone_src1), "freq", 450.0, NULL);

    tone_src2 = gst_element_factory_make ("audiotestsrc", "src2");
    g_object_set (G_OBJECT (tone_src2), "wave", 0, NULL);
    g_object_set (G_OBJECT (tone_src2), "volume", 0.0, NULL);
    // frequency object set is later reassigned in ring controller
    //g_object_set (G_OBJECT (tone_src2), "freq", 400.0, NULL);

    tone_adder = gst_element_factory_make("adder", "tone_adder");

    tone_converter = gst_element_factory_make ("audioconvert", "inst_audioconvert1");


    tone_sink = gst_element_factory_make ("alsasink", "tone_sink");
    g_object_set (G_OBJECT (tone_sink), "device", "hw:1,0", NULL);

    audio_resample = gst_element_factory_make ("audioresample", "audio_resample");


    /***************************************************/
    /* BUILD PIPELINE */
    /**************************************************/

    tone_pipeline = gst_pipeline_new ("tone_pipeline");


    if (!tone_pipeline || !tone_src1 || !tone_src2 || !tone_adder || !audio_resample || !queue_audio_1 || !queue_audio_2 || !tone_converter || !tone_sink)

    {
        fprintf (stderr, "not all elements could be created\n");
        exit (1);
    }


    gst_bin_add_many (GST_BIN (tone_pipeline), tone_src1, tone_src2, tone_adder, audio_resample, queue_audio_1, queue_audio_2, tone_converter, tone_sink, NULL);

    /**********************************************/
    /* LINK ALL ELEMENTS  */
    /**********************************************/

    if (
            gst_element_link_many (tone_src1, queue_audio_1, NULL) != TRUE  ||
            gst_element_link_many (tone_src2, queue_audio_2, NULL) != TRUE ||
            gst_element_link_many (tone_adder, tone_converter, audio_resample, tone_sink, NULL) != TRUE )
    {
        fprintf (stderr, "elements could not be linked\n");
        exit (1);
    }

    /*****************************************************/
    /*MANUALLY Link REQUEST PADS FOR ADDDER */
    /*****************************************************/

    /* define the pads */
    adder_pad_1 = gst_element_get_request_pad(tone_adder,"sink_%u");
    //        g_print ("Obtained request pad %s for adder_pad_1 branch.\n", gst_pad_get_name (adder_pad_1));
    adder_pad_2 = gst_element_get_request_pad(tone_adder,"sink_%u");
    g_print ("Obtained request pad %s for adder_pad_2 branch.\n", gst_pad_get_name (adder_pad_2));

    queue_1_pad = gst_element_get_static_pad(queue_audio_1,"src");
    queue_2_pad = gst_element_get_static_pad(queue_audio_2,"src");

    if (gst_pad_link (queue_1_pad, adder_pad_1) != GST_PAD_LINK_OK ||
            gst_pad_link (queue_2_pad, adder_pad_2) != GST_PAD_LINK_OK)
    {
        fprintf (stderr, "pads could not be linked\n");
        exit (1);
    }
    /******************************************/
    /* PLAY TONES PIPELINE */
    /******************************************/


    /* we need to run a GLib main loop to get the messages */
    loop = g_main_loop_new (NULL, FALSE);

    /* Runs a main loop until g_main_loop_quit() is called on the loop */
    g_print ("Running_Tones_...\n");
    g_main_loop_run (loop);




    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping Tones\n");
    //gst_element_set_state (tone_pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (tone_pipeline);
    gst_object_unref (adder_pad_1);
    gst_object_unref (adder_pad_2);
}

/******************************************/
/* DTMF BUFFER TASK */
/******************************************/

//loops and waits for a semaphore. If there is a semaphore it calls a function to play a dtmf tone



void
*tf_play_dtmf()

{
    struct sched_param para_dtmf;
    para_dtmf.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_dtmf);
    while(1)
    {

        sem_wait(&sem_dtmf); //wait for semaphore
        //Make audio connections to output the DTMF Tones
        //Firts clear the Matrix
        write_ctrl_register(MATRIX_FROM, MCP_OLAT, 0x00, 1111);
        write_ctrl_register(MATRIX_TO, MCP_OLAT, 0x00, 1112);
        //Turn on connection DTMF played, Turn off Signal_B DTMF source

        write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_TO, 1, 3097);
        write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_A_TO, 0, 3097);
        write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_A_FROM, 0, 3097);
        write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 0, 3097);

        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_TO_ENABLE, 1, 4057);
        connection_check();


        //Now play dtmf tone
        //More Volume then 0.5V (Alsa 53db) causes distortion by FET Switches
        //More Volume then 0.3V (Alsa 71db) causes distortion by FET Switches

        g_object_set (G_OBJECT (tone_src1), "volume", 0.3, NULL);
        g_object_set (G_OBJECT (tone_src2), "volume", 0.3, NULL);
        g_object_set (G_OBJECT (tone_src1), "wave", 0, NULL);
        g_object_set (G_OBJECT (tone_src2), "wave", 0, NULL);

        freq_low = dtmf_freq_low[mfv_buffer[dtmf_rd_idx]];
        freq_high = dtmf_freq_high[mfv_buffer[dtmf_rd_idx]];

        g_object_set (G_OBJECT (tone_src1), "freq", dtmf_freq_low[mfv_buffer[dtmf_rd_idx]], NULL);
        g_object_set (G_OBJECT (tone_src2), "freq", dtmf_freq_high[mfv_buffer[dtmf_rd_idx]], NULL);

        gst_element_set_state (tone_pipeline, GST_STATE_PLAYING);
        usleep (200000);
        gst_element_set_state (tone_pipeline, GST_STATE_NULL);
        usleep (200000);

        //After DTMF played, Turn off Signal_B DTMF source
        write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_TO, 0, 3097);
        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_TO_ENABLE, 0, 4057);
        //And turn on Audio link between line internal and etxternal after dial is completed
        origin_number = line_requesting();
        set_ext_connect(origin_number);

        connection_check();


        dtmf_rd_idx++; //increment read index

    }

}
