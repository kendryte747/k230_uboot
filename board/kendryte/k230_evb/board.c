#include <asm/asm.h>
#include <asm/io.h>
#include <asm/spl.h>
#include <asm/types.h>
#include <command.h>
#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <env_internal.h>
#include <image.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <lmb.h>
#include <stdio.h>

#include <kendryte/k230_platform.h>

#include "board_common.h"

int ddr_init_training(void) {
  if (0x00 != (readl((const volatile void __iomem *)0x980001bcULL) & 0x1)) {
    // have init ,not need reinit;
    return 0;
  }

  board_ddr_init();

  return 0;
}

int board_early_init_f(void) {
  return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void) {
    u32 usb0_otg_en_gpio52_dir = readl((void*)(GPIO_BASE_ADDR1 + 0x4));
    usb0_otg_en_gpio52_dir |= 1 << (52 - 32);
    writel(usb0_otg_en_gpio52_dir, (void*)(GPIO_BASE_ADDR1 + 0x4));

    u32 usb0_otg_en_gpio52_data = readl((void*)(GPIO_BASE_ADDR1 + 0x0));
    usb0_otg_en_gpio52_data |= 1 << (52 - 32);
    writel(usb0_otg_en_gpio52_data, (void*)(GPIO_BASE_ADDR1 + 0x0));

  return 0;
}
#endif
