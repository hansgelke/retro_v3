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
#include "pwm.h"


void *virtual_pwm_base;
int fd;

/****************************************************************
 * mmap virtual base calculation
 ****************************************************************/
int mmap_pwm()
{
    int  m_mfd;

    if ((m_mfd = open("/dev/mem", O_RDWR)) < 0)
    {
        printf("FAIL by open /dev/mem\n");
        return m_mfd;
    }
    // http://man7.org/linux/man-pages/man2/mmap.2.html
    virtual_pwm_base = (void *)mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_WRITE, MAP_SHARED, m_mfd, PWM_BASE_ADDR);
    close(m_mfd);

    if (virtual_pwm_base == MAP_FAILED)
    {
        return errno;
    }

    return 0;
}

 uint32_t pwm_reg_read( uint32_t pwm_reg_addr)
{
    uint32_t reg_content;
    uint32_t *pwm_virt_addr;
    pwm_virt_addr = (uint32_t *) (virtual_pwm_base + pwm_reg_addr);

    reg_content = *pwm_virt_addr;
    return reg_content;

}


void pwm_reg_write( uint32_t pwm_reg_addr, uint32_t value)
{
    uint32_t *pwm_virt_addr;
    pwm_virt_addr = (uint32_t *) (virtual_pwm_base + pwm_reg_addr);
    *pwm_virt_addr = value;
}

/****************************************************************
 * Generate Sinus for PWM Ringing Voltage Generator
 ****************************************************************/

void *tf_pwm()
{
    struct sched_param para_pwm;
    para_pwm.sched_priority = 40;
    sched_setscheduler(0,SCHED_RR, &para_pwm);
    int i_dds = 0;
    //sinus shape for ringer pwm
    float sine_array[] = {0, 0.19, 0.38, 0.56, 0.71, 0.83, 0.92, 0.98, 1.0, 0.98, 0.92, 0.83, 0.71, 0.56, 0.38,
                          0.19, 0, -0.19, -0.38, -0.56, -0.71, -0.83, -0.92, -0.98, -1.0, -0.98, -0.92, -0.83, -0.71, -0.56, -0.38, -0.19 };
    printf("Started pwm threat");
    while(1){
        // sem_wait(&sem_pwmon); // wait for pwm to be turned on
        //sem_post(&sem_pwmon); // Post semaphore for next cycle
        for (i_dds = 0; i_dds < 32; ++i_dds) {
            pwm_reg_write(PWM_DAT1, (1280+(1280*sine_array[i_dds])));
            //usleep(1250); //CH
            usleep(1562); //US
        }
    }
}

void
init_pwm(){
    int ret;
    ret =mmap_pwm();
    if (ret != 0) {
        printf("Error: Failed to initialize PWM: %s\n", strerror(abs(ret)));
        exit(ret);
    }
    pwm_reg_write(PWM_CTL, 0x0);
    pwm_reg_write(PWM_STA, 0x202);
    pwm_reg_write(PWM_DMAC, 0x707);
    pwm_reg_write(PWM_RNG1, 0x9ff);
    pwm_reg_write(PWM_DAT1, 0x4ff);
    pwm_reg_write(PWM_FIF1, 0x70776d30);
    pwm_reg_write(PWM_RNG2, 0x20);
    pwm_reg_write(PWM_DAT2, 0x00);


}


