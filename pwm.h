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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <semaphore.h>



#define PWM_BASE_ADDR   (0xfe20c000)

#define PWM_CTL         (0x00000000)
#define PWM_STA         (0x00000004)
#define PWM_DMAC        (0x00000008)
#define PWM_RNG1        (0x00000010)
#define PWM_DAT1        (0x00000014)
#define PWM_FIF1        (0x00000018)
#define PWM_RNG2        (0x00000020)
#define PWM_DAT2        (0x00000024)


//Function declarations
int mmap_pwm();
void pwm_reg_write(u_int32_t pwm_reg_addr, uint32_t value);
uint32_t pwm_reg_read(uint32_t pwm_reg_addr);
void *tf_pwm();

void init_pwm();

