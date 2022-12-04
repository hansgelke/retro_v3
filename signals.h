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

bool skip_flag;


typedef struct {
    /* Duration */
    uint32_t duration;
    /* Freqeuenz_1 in Hz */
    gdouble freq_1;
    /* Freqeuenz_2 in Hz */
    gdouble freq_2;
    /* Turn Tone on */
    bool tone_on;
    // Turn AC on for Ringer
    bool ringer_on;
    /* Last tone, skip to beginning */
    bool skip;
} note_t;

note_t *melody;


static note_t gb_ring[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    450.0,   400.0,    true,   true,   false},
    {  200000,    450.0,   400.0,    false,   false,   false},
    {  400000,    450.0,   400.0,    true,   true,   false},
    {  2000000,    450.0,   400.0,    false,   false,   true},
};

static note_t ger_ring[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  1000000,    00.0,   400.0,    true,   true,   false},
    {  4000000,    450.0,   400.0,    false,   false,   true},
};

static note_t gb_dial[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    450.0,   350.0,    true,   false,   true},
};

static note_t ger_dial[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    0.0,   425.0,    true,   false,   true},
};

static note_t us_dial[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  400000,    440.0,   350.0,    true,   false,   true},

};

static note_t gb_engaged[] = {
    /*	     durat     freq_1 freq_2  tone_on ringer_on skip  */
    {  500000,    0.0,   400.0,    true,   false,   false},
    {  500000,    0.0,   400.0,    false,   false,   true},
};
