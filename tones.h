#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>

void *tf_tone_gen();

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
