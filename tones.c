#include "tones.h"
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
#include <gst/gst.h>



int
tones (int argc, char *argv[])
{
    GstElement	*bin;
    GstElement	*src;
        GstElement	*conv1;
        GstElement	*sink;
    GMainLoop	*loop;

    gst_init (&argc, &argv);

    bin = gst_pipeline_new ("bin");

    src = gst_element_factory_make ("audiotestsrc", "src");
    g_object_set (G_OBJECT (src), "wave", 0, NULL);
        g_object_set (G_OBJECT (src), "freq", 1000.0, NULL);

    conv1 = gst_element_factory_make ("audioconvert", "inst_audioconvert1");

    sink = gst_element_factory_make ("alsasink", "sink");
    g_object_set (G_OBJECT (sink), "device", "hw:1,0", NULL);

    gst_bin_add_many (GST_BIN (bin), src, conv1, sink, NULL);


    /* we link the elements together */
    if (
                !gst_element_link (src, conv1) ||
        !gst_element_link (conv1, sink)
           ) {
        fprintf (stderr, "can't link elements\n");
        exit (1);
    }


    gst_element_set_state (bin, GST_STATE_PLAYING);

    /* we need to run a GLib main loop to get the messages */
    loop = g_main_loop_new (NULL, FALSE);

    /* Runs a main loop until g_main_loop_quit() is called on the loop */
    g_print ("Running_x_...\n");
    g_main_loop_run (loop);

    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (bin, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (bin);

    return 0;
}
