/* ------------------------------------------------------------------
 * --  _____       ______  _____                                    -
 * -- |_   _|     |  ____|/ ____|                                   -
 * --   | |  _ __ | |__  | (___    Institute of Embedded Systems    -
 * --   | | | '_ \|  __|  \___ \   Zurich University of             -
 * --  _| |_| | | | |____ ____) |  Applied Sciences                 -
 * -- |_____|_| |_|______|_____/   8401 Winterthur, Switzerland     -
 * ------------------------------------------------------------------
 * --
 * -- File:	cli.c
 * -- Date:	05.02.2017
 * -- Author:	rosn
 * --
 * ------------------------------------------------------------------
 */
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include "gpio.h"
#include "pwm.h"
#include "main.h"
#include "tones.h"
#include "signals.h"
#include "dial.h"
#include "extern.h"



// Hardware definitions
#define GPIO_IN 0x0
#define GPIO_OUT 0x1
#define GPIO_EDGE_FALLING 0x0
#define GPIO_EDGE_RISING 0x1
#define GPIO_HIGH_DETECT 0x2
#define GPIO_LOW_DETECT 0x3

void *virtual_gpio_base;

int main(void) {

    init_gpios();
    init_pwm();
    sem_init(&sem_pwmon,0,0);
    sem_init(&sem_ring,0,0);

    /************************************************************
     *************** Initialize Tasks ***************************
     ************************************************************/

    pthread_t t_pwm, t_menue, t_tone_gen, t_generate_signals, t_rotary, t_main_fsm ;
    uint8_t iret1;

    iret1 = pthread_create(&t_pwm, NULL, &tf_pwm, NULL);
    iret1 = pthread_create(&t_rotary, NULL, &tf_rotary, NULL);
    iret1 = pthread_create(&t_menue, NULL, &tf_menue, NULL);
    iret1 = pthread_create(&t_tone_gen, NULL, &tf_tone_gen, NULL);
    iret1 = pthread_create(&t_generate_signals, NULL, &tf_generate_signals, NULL);
    iret1 = pthread_create(&t_main_fsm, NULL, &tf_main_fsm, NULL);



    pthread_join(t_pwm, NULL);
    pthread_join(t_rotary, NULL);
    pthread_join(t_menue, NULL);
    pthread_join(t_tone_gen, NULL);
    pthread_join(t_generate_signals, NULL);
    pthread_join(t_main_fsm, NULL);






}

void
*tf_menue()
{
    struct sched_param para_menue;
    para_menue.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_menue);

    char line[100];
    int  a_arg1 = 0, a_arg2 = 0, a_arg3 = 0;
    char s_arg[100];
    int mcp_rd_val = 0;

    uint32_t pwm0_ctl_reg = 0;
    uint32_t pwm0_sta_reg = 0;
    uint32_t pwm0_dmac_reg = 0;
    uint32_t pwm0_rng1_reg = 0;
    uint32_t pwm0_dat1_reg = 0;
    uint32_t pwm0_fif1_reg = 0;
    uint32_t pwm0_rng2_reg = 0;
    uint32_t pwm0_dat2_reg = 0;

    printf("Started Menu threat");


    while(1) {
        printf("rdiic - Read I2C Device Register\n");
        printf("wriic - Write I2C\n");
        printf("conn - Connect two devices\n");
        printf("pwmwr - Write pwm register\n");
        printf("pwmrd - Read PWM register\n");
        printf("ring - Ring a phone\n");
        printf("rings - Stop Ringing\n");
        printf("pwmon - Turn PWM on\n");
        printf("pwmoff - Turn PWM off\n");
        printf("exton - Connects to Edxternal line\n");
        printf("extoff - Disconnects external line\n");
        printf("sinus - Generate Sinus Signal and send to a phone\n");
        printf("gbring - Generate GB ring signal and send to phone\n");
        printf("\n");
        printf("\n");

        printf("Command > ");
        fgets(line,100,stdin);


        if (strcmp(line, "x") == 0){
            return 0;
        }

        else if (strcmp(line, "wriic\n") == 0){

            printf("Device_Addr, Register_Addr, Write_Data:\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x %x", &a_arg1, &a_arg2, &a_arg3);
            write_ctrl_register(a_arg1, a_arg2, a_arg3);
            printf("Device:%x Register:%x is set to:%x \n", a_arg1, a_arg2, a_arg3);
        }



        else if (strcmp(line, "rdiic\n") == 0){
            printf("Device_Addr, Register_Addr:\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x", &a_arg1, &a_arg2);
            mcp_rd_val = read_ctrl_register(a_arg1,a_arg2);
            printf("Device:%x Register:%x Contains:%x \n", a_arg1, a_arg2, mcp_rd_val);
        }

        else if (strcmp(line, "conn\n") == 0){
            printf("From, To:\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x", &a_arg1, &a_arg2);
            set_connections(a_arg1,a_arg2);
            printf("From Phone:%x To Phone:%x \n", a_arg1, a_arg2);
        }

        // Write PWM Register
        else if (strcmp(line, "pwmwr\n") == 0){
            printf("PWM Register Address, Write Value\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x", &a_arg1, &a_arg2);

            pwm_reg_write(a_arg1, a_arg2);
            pwm0_ctl_reg = pwm_reg_read (0x0);
            pwm0_sta_reg = pwm_reg_read (0x4);
            pwm0_dmac_reg = pwm_reg_read (0x8);
            pwm0_rng1_reg = pwm_reg_read (0x10);
            pwm0_dat1_reg = pwm_reg_read (0x14);
            pwm0_fif1_reg = pwm_reg_read (0x18);
            pwm0_rng2_reg = pwm_reg_read (0x20);
            pwm0_dat2_reg = pwm_reg_read (0x24);


            printf("PWM0 CTL(0x0)  Register: 0x%08x \n", pwm0_ctl_reg);
            printf("PWM0 STA (0x4) Register: 0x%08x \n", pwm0_sta_reg);
            printf("PWM0 DMAC(0x8) Register: 0x%08x \n", pwm0_dmac_reg);
            printf("PWM0 RNG1(0x10) Register: 0x%08x \n", pwm0_rng1_reg);
            printf("PWM0 DAT1(0x14) Register: 0x%08x \n", pwm0_dat1_reg);
            printf("PWM0 FIF1(0x18) Register: 0x%08x \n", pwm0_fif1_reg);
            printf("PWM0 RNG2(0x20) Register: 0x%08x \n", pwm0_rng2_reg);
            printf("PWM0 DAT2 (0x024)Register: 0x%08x \n", pwm0_dat2_reg);

        }

        //Dump all PWM0 Registers
        else if (strcmp(line, "pwmrd\n") == 0){
            pwm0_ctl_reg = pwm_reg_read (0x0);
            pwm0_sta_reg = pwm_reg_read (0x4);
            pwm0_dmac_reg = pwm_reg_read (0x8);
            pwm0_rng1_reg = pwm_reg_read (0x10);
            pwm0_dat1_reg = pwm_reg_read (0x14);
            pwm0_fif1_reg = pwm_reg_read (0x18);
            pwm0_rng2_reg = pwm_reg_read (0x20);
            pwm0_dat2_reg = pwm_reg_read (0x24);


            printf("PWM0 CTL(0x0)  Register: 0x%08x \n", pwm0_ctl_reg);
            printf("PWM0 STA (0x4) Register: 0x%08x \n", pwm0_sta_reg);
            printf("PWM0 DMAC(0x8) Register: 0x%08x \n", pwm0_dmac_reg);
            printf("PWM0 RNG1(0x10) Register: 0x%08x \n", pwm0_rng1_reg);
            printf("PWM0 DAT1(0x14) Register: 0x%08x \n", pwm0_dat1_reg);
            printf("PWM0 FIF1(0x18) Register: 0x%08x \n", pwm0_fif1_reg);
            printf("PWM0 RNG2(0x20) Register: 0x%08x \n", pwm0_rng2_reg);
            printf("PWM0 DAT2 (0x024)Register: 0x%08x \n", pwm0_dat2_reg);
        }

        else if (strcmp(line, "ring\n") == 0){
            printf("Ring Phone (1-8):");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x", &a_arg1);

            pwm_reg_write(PWM_CTL, 0x81);

            write_ctrl_register(PHONE_AC, MCP_OLAT, hex2lines(a_arg1));
            write_ctrl_register(PHONE_DC, MCP_OLAT, hex2notlines(a_arg1));
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1);
            sem_post(&sem_pwmon); // Post semaphore to start


        }

        else if (strcmp(line, "rings\n") == 0){
            pwm_reg_write(PWM_CTL, 0x00);
            write_ctrl_register(PHONE_AC, MCP_OLAT, 0x00);
            write_ctrl_register(PHONE_DC, MCP_OLAT, 0xff);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0);
            sem_init(&sem_pwmon,0,0);

        }



        else if (strcmp(line, "pwmon\n") == 0){
            pwm_reg_write(PWM_CTL, 0x81);
            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 1);

        }

        else if (strcmp(line, "pwmoff\n") == 0){
            sem_init(&sem_pwmon,0,0);
            pwm_reg_write(PWM_CTL, 0x00);

            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, RINGER_ENABLE, 0);
        }

        else if (strcmp(line, "exton\n") == 0){
            pwm_reg_write(PWM_CTL, 0x00);

            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 1);

        }

        else if (strcmp(line, "extoff\n") == 0){
            pwm_reg_write(PWM_CTL, 0x00);

            write_mcp_bit(CONNECT_CTRL, MCP_OLAT, EXT_LINE_RELAY, 0);

        }

        else if (strcmp(line, "sinus\n") == 0){
            printf("Send Tone to Lines (1-8):");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x", &a_arg1);


            melody = ger_dial;
            sem_post(&sem_signal);
            write_mcp_bit(DTMF_READ, MCP_OLAT, SIGNAL_B_FROM, 1);
            write_ctrl_register(MATRIX_FROM, MCP_OLAT, hex2lines(a_arg1));

        }

        else if (strcmp(line, "gbring\n") == 0){
            melody = gb_ring;
            sem_post(&sem_signal);
            sem_post(&sem_ring);

        }

        else {
            printf("Unknown command '%c'\n", line[0]);
        }
    }
}





