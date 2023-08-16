#include "menue.h"
#include "main.h"
#include "extern.h"
#include "gpio.h"
#include "signals.h"
#include "dial.h"

pthread_mutex_t dial_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_dial = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_dialcomplete = PTHREAD_COND_INITIALIZER;

pthread_mutex_t dtmf_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_dtmf;

extern uint8_t number_dialed;
uint32_t binary_lines;

uint8_t first_num;
uint8_t dialed_number;

extern bool debounce_flag;

ext_fsm_state_t next_state;
ext_fsm_state_t ext_state = st_idle ;

uint32_t ring_timer = 0;
uint8_t dial_status = 0;

extern uint8_t mfv_buffer[32];

uint32_t to_matrix;
uint32_t from_matrix;


void *tf_main_fsm()
{
    struct sched_param para_main;
    para_main.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_main);
    sched_setscheduler(0,SCHED_OTHER, NULL);
    sem_init(&sem_signal,0,0);


    while(1) {

        usleep(10000);
        if (test_mode == true)
        {
            test_menue();
        }
        else
        {

            ext_timer();
            next_state = ext_state;
            main_fsm();
            if     (ext_state != next_state){
                printf("State changed: '%x' Origin: '%x' \n", ext_state, origin_number);
            };




        }

    }
}


void ext_timer()
{
    //An active signal on ring indicator sets the debounce flag after 20ms
    if ((gpio_read(RING_INDICATOR_N) == 0) & (debounce_flag == false)  ){
        usleep(20000);
        debounce_flag = true;
    }
    // If ring indicator goes high while debounce flag is true, there was a glitch
    // and debounce flag is cleared
    else if ((gpio_read(RING_INDICATOR_N) == 1) & (debounce_flag == true)){
        debounce_flag = false;
    }
    //if the debounce flag is still true, while the indicator is still active
    // it is assumed that there is no glitch and MAX_RING is loaded
    else if ((gpio_read(RING_INDICATOR_N) == 0) & (debounce_flag == true)){
        ring_timer = MAX_RING;
        debounce_flag = false;
    }
    //count down delayed by usleep
    else if (ring_timer > 0) {
        ring_timer-- ;
    }
    else{
        ring_timer = 0;
    }
}


void main_fsm()

{
    //uint8_t iac;
    /******************************************************************************
    *                       State IDLE
    ******************************************************************************/

    switch (ext_state) {
    case st_idle:
        origin_number = line_requesting();
        //>>>>> GOTO st_ext_ring
        if (ring_timer > 0){
            melody = gb_ring;
            ac_on(true,1);
            sem_post(&sem_signal);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
            ext_state = st_ext_ring;
        }
        // Put Dail tone on requesting line
        else if (origin_number != 0xff) {
            melody = ger_dial ;
            sem_post(&sem_signal);
            //set matrix to output dial tone
            write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 1, 3097);
            write_mcp_bit(MATRIX_FROM, MCP_OLAT, origin_number, 1, 3098);

            //Arm rotary dial receiver
            pthread_mutex_lock(&dial_mutex);
            pthread_cond_signal(&cond_dial);
            pthread_mutex_unlock(&dial_mutex);

            ext_state = st_offhook;

        }
        else ext_state = st_idle;

        break;


        /******************************************************************************
     *                       State EXTERNAL RING
     ******************************************************************************/

    case st_ext_ring:

        if (mmap_gpio_test(PICK_UP_N) == true){
            //>>>>>>>> Go TO ACCEPTED  >>>>>>

            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 1, 4057);
            ac_on(false,1);
            ring_timer = 0;
            set_ext_connect(0);

            ext_state = st_ext_accepted;
        }
        else if (ring_timer == 0){
            //>>>>>>>>>>>>>>>>>GO TO IDLE >>>>>>>>>>>>>>>>>>>>
            //turn ringing of if it goes to idle state
            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);

            ext_state = st_idle;
        }

        else  {
            ext_state = st_ext_ring;
        }
        break;

        /******************************************************************************
         *                       State External Call Accepted
         ******************************************************************************/

    case st_ext_accepted:
        //If loop is opened, hang up external line
        if (line_requesting() == 0xff){
            //            //If loop is opened, hang up external line
            // >>>>>>>>>>  GO TO IDLE  >>>>>>>>>>>>>>>>>>>>
            return_to_idle();
            ext_state = st_idle;
        }
        else {
            ext_state = st_ext_accepted;
        }
        break;

        /******************************************************************************
        *                       State OFF HOOK
        ******************************************************************************/
    case st_offhook:
        // Wait for dial complete
        pthread_mutex_lock(&dial_mutex);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        pthread_mutex_unlock(&dial_mutex);

        first_num = number_dialed;

        //Evaluate return Message from rotary encoder
        switch (dial_status) {
        case stat_dial_complete:

            //REQUESTING PHONE IS DIALING OPERATOR TO GET OUTSIDE LINE
            if(first_num == 10){
                //Clear the Dial Buffer Indexes
                dtmf_rd_idx = 0;
                dtmf_wr_idx = 0;

                //Withdraw tokens from Tone-Signal Generation
                // after dialing 0, no tone should be audible
                sem_init(&sem_signal,0,0);
                {gst_element_set_state (tone_pipeline, GST_STATE_NULL);
                }


                // Clear the connection Matrix
                write_ctrl_register(MATRIX_FROM, MCP_OLAT, 0x00, 1111);
                write_ctrl_register(MATRIX_TO, MCP_OLAT, 0x00, 1112);
                write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_TO, 0, 3097);
                write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 0, 3097);
                write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_TO_ENABLE, 0, 4057);
                write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_FROM_ENABLE, 0, 4057);


                // Connects the Matrix for the speach channel
                set_ext_connect(origin_number);

                //The Tone Generator is connected directly to the EXT-output by the amplifier
                // Enableing the Matrix is therefore not required
                //Turn on Relay
                write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 1, 4057);


                //The write pointer for the DTMF signals is set to 0 when outside_line is entered
                dtmf_wr_idx = 0;
                ext_state = st_outsideline;
            }
            else {
                //>>>>GOTO st_second_number
                //Turn Tones off

                sem_init(&sem_signal,0,0);

                pthread_mutex_lock(&dial_mutex);
                pthread_cond_signal(&cond_dial);
                pthread_mutex_unlock(&dial_mutex);
                ext_state = st_second_number;


            }
            break;

        case stat_nodial:
            return_to_idle();
            ext_state = st_idle;

            break;

        case stat_hangup:
            return_to_idle();

            ext_state = st_idle;

            break;


        }
        break;

        /******************************************************************************
        *                       State Second Number
        ******************************************************************************/

    case st_second_number:
        pthread_mutex_lock(&dial_mutex);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        pthread_mutex_unlock(&dial_mutex);

        dialed_number = number_dialed;
        //>>>>> GOTO st_int_ring
        melody = gb_ring;
        ac_on(true,dialed_number);
        sem_post(&sem_signal);
        write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1, 5071);
        ext_state = st_int_ring;

        break;
        /******************************************************************************
            *                       State OUTSIDE LINE
           ******************************************************************************/

    case st_outsideline:

        //Arm rotary dial receiver
        pthread_mutex_lock(&dial_mutex);
        pthread_cond_signal(&cond_dial);
        pthread_mutex_unlock(&dial_mutex);

        //Wait for dial complete
        pthread_mutex_lock(&dial_mutex);
        pthread_cond_wait(&cond_dialcomplete, &dial_mutex);
        pthread_mutex_unlock(&dial_mutex);

        switch (dial_status) {
        //Normal completion of dial
        case stat_dial_complete:
            dialed_number = number_dialed;
            //Ready for the next number

            //When a number is dialed in st_ousideline, the number is stored in a buffer
            // The buffer is incremented for each number
            // ine tone.c the numbers are taken out of the buffer with dtmf_rd_idx
            // Post a semaphore sem_dtmf to unblock dtmf generation
            pthread_mutex_lock(&dtmf_mutex);
            mfv_buffer[dtmf_wr_idx] = number_dialed;
            dtmf_wr_idx++;
            // Post a token, which triggers to play a dtmf tone in *tf_play_dtmf (tones.c)
            sem_post(&sem_dtmf);
            pthread_mutex_unlock(&dtmf_mutex);


            ext_state = st_outsideline;
            break;

        case stat_nodial:
            //User waits more than 30s to dial
            //Here no Error, FSM stays in same state

            ext_state = st_outsideline;

            break;

        case stat_hangup:
            //User hung up instead of dialing
            // Turn off  relay, open all connections
            return_to_idle();
            ext_state = st_idle;

            break;

        }
        break;



        //>>>>> GOTO st_int_ring

        //if phone is hang up go to idle
        if (line_requesting() == 0xff){
            //>>>>>>>>>>  GO TO st_idle  >>>>>>>>>>>>>>>>>>>>
            //clear all connections
            return_to_idle();


            ext_state = st_idle;
        }
        else {
            ext_state = st_outsideline;

        }

        break;
        /***************************************************/
        //                      STATE HANG UP
        /****************************************************/
    case st_hang_up:


        usleep(1000);
        return_to_idle();
        ext_state = st_idle;
        break;

        /***************************************************/
        //                      STATE INTERN RING
        /****************************************************/


    case st_int_ring:
        origin_number = line_requesting();
        if (mmap_gpio_test(PICK_UP_N) == true){
            //>>>> GOTO st_int_accepted
            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
            //Turn all lines back to DC mode
            write_ctrl_register(PHONE_AC, MCP_OLAT, 0x00, 1113);
            write_ctrl_register(PHONE_DC, MCP_OLAT, 0xff, 1114);

            //Clear Matrix
            write_ctrl_register(MATRIX_FROM, MCP_OLAT, 0x00, 1139);
            write_ctrl_register(MATRIX_TO, MCP_OLAT, 0x00, 1140);
            write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 0, 3097);

            //Sets the connection matrix FROM TO
            set_connections(origin_number, number_dialed);
            from_matrix = read_ctrl_register(MATRIX_FROM,MCP_OLAT, 373);
            to_matrix = read_ctrl_register(MATRIX_TO,MCP_OLAT, 374);
            printf("FROM: '%x' TO: '%x' \n", from_matrix, to_matrix);

            ext_state = st_int_accepted;

        }
        else if (origin_number == 0xff){
            //>>>> GOTO st_idle

            sem_init(&sem_signal,0,0);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0, 5071);
            //Turn all lines back to DC mode
            return_to_idle();
            ext_state = st_idle;
        }
        else {
            ext_state = st_int_ring;
        }
        break;

        /***************************************************/
        //                      Inetrnal Call Accepted
        /****************************************************/

    case st_int_accepted:
        //if all participants are leaving call go to idle
        if (line_requesting() == 0xff){

            //>>>>>>>>>>  GO TO st_idle  >>>>>>>>>>>>>>>>>>>>

            return_to_idle();
            ext_state = st_idle;
        }
        else {
            ext_state = st_int_accepted;

        }
        break;

        /***************************************************/
        //                      SWITCH DEFAULT STATE
        /****************************************************/

    default:

        return_to_idle();
        ext_state = st_idle;

    }

}





