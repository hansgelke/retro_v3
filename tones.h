#include <pthread.h>
#include <gst/gst.h>
#include <stdbool.h>
#include <stdint.h>



void *tf_tone_gen();
void *tf_play_dtmf();
int play_dtmf(int dial_no);



GstElement *queue_audio_1, *queue_audio_2;
GstElement *audio_resample;
GMainLoop	*loop;
GstPad *adder_pad_1, *adder_pad_2;
GstPad *queue_1_pad, *queue_2_pad;
GstElement	*tone_src1;
GstElement	*tone_src2;
GstElement *tone_adder;
GstElement	*tone_converter;
GstElement	*tone_sink;
GstElement	*tone_pipeline;

//Define Piüeline Elemenst for DTMF

GstElement	*dtmf_pipeline;
GstElement	*dtmf_src;
GstStructure *dtmf_structure_start;
GstStructure *dtmf_structure_stop;
GstEvent *dtmf_start;
GstEvent *dtmf_stop;
GstElement	*dtmf_conv1;
GstElement	*dtmf_sink;
