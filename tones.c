#include "tones.h"
#include <gst/gst.h>


/****************************************************************
 *                   GENERATION FOR ALL SIGNAL TONES
 ****************************************************************/
void *tf_tone_gen()
{
    struct sched_param para_tone;
    para_tone.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_tone);
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

    tone_src1 = gst_element_factory_make ("audiotestsrc", "src1");
    g_object_set (G_OBJECT (tone_src1), "wave", 0, NULL);
    g_object_set (G_OBJECT (tone_src1), "volume", 0.0, NULL);
    // frequency object set is later reassigned in ring controller
    //g_object_set (G_OBJECT (tone_src1), "freq", 450.0, NULL);

    tone_src2 = gst_element_factory_make ("audiotestsrc", "src2");
    g_object_set (G_OBJECT (tone_src2), "wave", 0, NULL);
    g_object_set (G_OBJECT (tone_src2), "volume", 0.4, NULL);
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
    //    /******************************************/
        //    /* PLAY TONES PIPELINE */
        //    /******************************************/

        // gst_element_set_state (tone_pipeline, GST_STATE_PLAYING);

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
