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
#include "gpio.h"
#include "pwm.h"
#include "main.h"
#include "tones.h"
#include <semaphore.h>

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
    tones(0, 0);

    pthread_t t_pwm, t_menue;
    int iret1;
    int i_dds = 0;
    float sine_array[] = {0, 0.19, 0.38, 0.56, 0.71, 0.83, 0.92, 0.98, 1.0, 0.98, 0.92, 0.83, 0.71, 0.56, 0.38,
                          0.19, 0, -0.19, -0.38, -0.56, -0.71, -0.83, -0.92, -0.98, -1.0, -0.98, -0.92, -0.83, -0.71, -0.56, -0.38, -0.19 };

    iret1 = pthread_create(&t_pwm, NULL, &tf_pwm, NULL);
    iret1 = pthread_create(&t_menue, NULL, &tf_menue, NULL);

    pthread_join(t_pwm, NULL);
    pthread_join(t_menue, NULL);


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
        printf("Command > ");
        fgets(line,100,stdin);


        if (strcmp(line, "x") == 0){
            return 0;
        }

        else if (strcmp(line, "wri\n") == 0){

            printf("Device_Addr, Register_Addr, Write_Data:\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x %x", &a_arg1, &a_arg2, &a_arg3);
            write_ctrl_register(a_arg1, a_arg2, a_arg3);
            printf("Device:%x Register:%x is set to:%x \n", a_arg1, a_arg2, a_arg3);
        }



        else if (strcmp(line, "rdi\n") == 0){
            printf("Device_Addr, Register_Addr:\n");
            fgets(line,100,stdin);
            if (strcmp(line, "x\n") == 0){
                return 0;
            }
            sscanf(&line[0], "%x %x", &a_arg1, &a_arg2);
            mcp_rd_val = read_ctrl_register(a_arg1,a_arg2);
            printf("Device:%x Register:%x Contains:%x \n", a_arg1, a_arg2, mcp_rd_val);
        }

        else if (strcmp(line, "con\n") == 0){
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
        else if (strcmp(line, "wrpwm\n") == 0){
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
        else if (strcmp(line, "rdpwm\n") == 0){
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

        else if (strcmp(line, "pwms\n") == 0){
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

        else if (strcmp(line, "pwmp\n") == 0){
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

        else {
            printf("Unknown command '%c'\n", line[0]);
                    }
                }
            }





