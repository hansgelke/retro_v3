#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <gst/gst.h>
#include "main.h"
#include "tones.h"


void *tf_generate_signals();
void *tf_play_announcements();

//Define Annoucement Pipeline
GstElement	*announcement_pipeline;
GstElement	*announcement_src;
GstCaps *announce_caps;
GMainLoop	*announce_loop;



bool skip_flag;


typedef struct {
    /* Duration */
    uint32_t duration;
    /* Freqeuenz_1 in Hz */
    gdouble freq_1;
    /* Freqeuenz_2 in Hz */
    gdouble freq_2;
    /* Define volume of tone */
    gdouble vol;
    /* Turn Tone on */
    bool tone_on;
    // Turn AC on for Ringer
    bool ringer_on;
    /* Last tone, skip to beginning */
    bool skip;
} note_t;

note_t *melody;

typedef enum {
    us, //US phone
    gb, //GB phone
    eu  //European phone
} country_t;

//Define here, which phone is from which country
static country_t country_set[8] = {gb, gb, gb, gb, eu, eu, eu, us};

static note_t gb_ring[4] = {
    /* durat     freq_1   freq_2     vol    tone_on ringer_on skip  */
    {  400000,    450.0,   400.0,    0.5,     true,   true,   false},
    {  200000,    450.0,   400.0,    0.5,     false,  false,  false},
    {  400000,    450.0,   400.0,    0.5,     true,   true,   false},
    {  2000000,    450.0,  400.0,    0.5,     false,  false,  true}
};

static note_t ger_ring[2] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  1000000,    00.0,   400.0,   1.0, true,   true,   false},
    {  4000000,    00.0,   400.0,  1.0,  false,   false,   true}
};

static note_t ger_enga[2] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  500000,    00.0,   400.0,   1.0, true,   false,   false},
    {  500000,    00.0,   400.0,  1.0,  false,   false,   true}
};

static note_t gb_dial[1] = {
    /* durat  freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,   450.0,  350.0,  0.5,    true,   false,   true}
};
//German tone is 425Hz but gst not precise, therefore set to 415Hz
static note_t ger_dial[1] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    0.0,   415.0, 1.0 ,   true,   false,   true}
};

static note_t us_dial[1] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    440.0,   350.0, 0.5,   true,   false,   true}

};


