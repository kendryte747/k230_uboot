/* Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <asm/asm.h>
#include <asm/io.h>
#include <asm/spl.h>
#include <asm/types.h>
#include <command.h>
#include <common.h>
#include <cpu_func.h>
#include <env_internal.h>
#include <gzip.h>
#include <image.h>
#include <linux/kernel.h>
#include <lmb.h>
#include <malloc.h>
#include <memalign.h>
#include <stdio.h>
#include <u-boot/crc.h>

#include <kendryte/k230_platform.h>

#include "board_common.h"

int mmc_get_env_dev(void) {
  return (BOOT_MEDIUM_SDIO1 == g_boot_medium) ? 1 : 0;
}

enum env_location arch_env_get_location(enum env_operation op, int prio) {
  if (0 != prio) {
    return ENVL_UNKNOWN;
  }

#ifdef CONFIG_ENV_IS_NOWHERE
  return ENVL_NOWHERE;
#endif

  if (g_boot_medium == BOOT_MEDIUM_NORFLASH) {
    return ENVL_SPI_FLASH;
  }

  if (g_boot_medium == BOOT_MEDIUM_NANDFLASH) {
    return ENVL_SPINAND;
  }

  return ENVL_MMC;
}

#ifndef CONFIG_SPL_BUILD
int __weak kd_board_init(void)
{
  return 0;
}

int board_init(void) {
  if ((BOOT_MEDIUM_SDIO0 == g_boot_medium) ||
      (BOOT_MEDIUM_SDIO1 == g_boot_medium)) {
#define SD_HOST_REG_VOL_STABLE (1 << 4)
#define SD_CARD_WRITE_PROT (1 << 6)

    u32 sd0_ctrl = readl((void *)SD0_CTRL);
    sd0_ctrl |= SD_HOST_REG_VOL_STABLE | SD_CARD_WRITE_PROT;
    writel(sd0_ctrl, (void *)SD0_CTRL);
  }

  return kd_board_init();
}

#define K230_SET_BIT(val, bit) ((val) | (1 << (bit)))
#define K230_CLER_BIT(val, bit) ((val) & ~(1 << (bit)))
#define K230_GET_BIT(val, bit) (((val) >> (bit)) & 1)
#define GPIO_EXT_PORTA 0x50

int k230_gpio(char opt, int pin, char *value)
{
    int ret = 0;

    volatile u32 * gpio_dr = (volatile int *)(GPIO_BASE_ADDR0 + pin/32*0x1000);
    volatile u32 * gpio_ddr = (volatile int *)(gpio_dr+1);
    volatile u32 * gpio_ctrl = (volatile int *)(gpio_dr+2);
    volatile u32 * gpio_value_in = (volatile int *)(GPIO_BASE_ADDR0+GPIO_EXT_PORTA + pin/32*4);
    u32 reg_org, reg_set;

    printf("pin=%d  org reg gpio_dr%x=%x gpio_ddr=%x %x\n", pin,  gpio_dr,*gpio_dr, *gpio_ddr, *gpio_ctrl);

    if(pin > 71)
        return -1;

    if(opt == 's') {
        if(value == NULL) {
            printf("value is NULL\n");
            return -1;
        }
        reg_org =  *gpio_dr;
        if(*value == 0) {
            *gpio_dr = K230_CLER_BIT(reg_org, pin % 32);

        } else{
            *gpio_dr = K230_SET_BIT(reg_org, pin % 32);
        }
    } else if(opt == 'g') {
        reg_org = *gpio_value_in;
        *value = K230_GET_BIT(reg_org, pin % 32);
    } else if(opt == 'i') { //set 0;
        reg_org = *gpio_ddr;
        *gpio_ddr = K230_CLER_BIT(reg_org, pin % 32);
    } else if(opt == 'o') {
        reg_org = *gpio_ddr;
        *gpio_ddr = K230_SET_BIT(reg_org, pin % 32);
        if( value && (*value) ) {
            *gpio_dr = K230_SET_BIT(reg_org, pin % 32);
        } else  if( value && (*value == 0)) {
            *gpio_dr = K230_CLER_BIT(reg_org, pin % 32);
        }
    } else {
        printf("opt %c is invalid\n", opt);
        return -1;
    }
    printf("pin=%d %c  after reg gpio_dr=%x gpio_ddr=%x ctl%x\n", pin,  opt, *gpio_dr, *gpio_ddr, *gpio_ctrl);
    return ret;

}
static int do_k230_gpio(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;
    if(argc < 2) {
        printf("usage: k230_gpio set/get/in/out pin [value]\n");
        return -1;
    }

    int pin = simple_strtoul(argv[2], NULL, 0);
    char value=0;
    if(argc > 3) {
        value = simple_strtoul(argv[3], NULL, 0);
    }
    ret = k230_gpio(argv[1][0], pin, &value);
    printf("%c pin %d value %d \n", argv[1][0], pin, value);
    return ret;
}
U_BOOT_CMD(
	k230_gpio, CONFIG_SYS_MAXARGS, 0, do_k230_gpio,
	"k230_gpio set/get/in/out pin [value]",
	"k230_gpio set/get/in/out pin [value]"
);
#endif
