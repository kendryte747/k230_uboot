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

static int k230_boot_prepare_args(int argc, char *const argv[], ulong buff,
                                  en_boot_sys_t *sys, boot_medium_e *bootmod) {
  ulong add_tmp, len;

  if (argc < 3)
    return CMD_RET_USAGE;

  if (!strcmp(argv[1], "mem")) {
    if (argc < 4)
      return CMD_RET_USAGE;
    add_tmp = simple_strtoul(argv[2], NULL, 0);
    len = simple_strtoul(argv[3], NULL, 0);
    if (add_tmp != buff) {
      memmove((void *)buff, (void *)add_tmp, len);
    }
    *sys = BOOT_SYS_ADDR;
    return 0;
  } else if (!strcmp(argv[1], "sdio1"))
    *bootmod = BOOT_MEDIUM_SDIO1;
  else if (!strcmp(argv[1], "sdio0"))
    *bootmod = BOOT_MEDIUM_SDIO0;
  else if (!strcmp(argv[1], "spinor"))
    *bootmod = BOOT_MEDIUM_NORFLASH;
  else if (!strcmp(argv[1], "spinand"))
    *bootmod = BOOT_MEDIUM_NANDFLASH;
  else if (!strcmp(argv[1], "auto"))
    *bootmod = g_boot_medium;

  if (!strcmp(argv[2], "rtt"))
    *sys = BOOT_SYS_RTT;
  else if (!strcmp(argv[2], "linux"))
    *sys = BOOT_SYS_LINUX;
  else if (!strcmp(argv[2], "qbc"))
    *sys = BOOT_QUICK_BOOT_CFG;
  else if (!strcmp(argv[2], "fdb"))
    *sys = BOOT_FACE_DB;
  else if (!strcmp(argv[2], "sensor"))
    *sys = BOOT_SENSOR_CFG;
  else if (!strcmp(argv[2], "ai"))
    *sys = BOOT_AI_MODE;
  else if (!strcmp(argv[2], "speckle"))
    *sys = BOOT_SPECKLE;
  else if (!strcmp(argv[2], "rtapp"))
    *sys = BOOT_RTAPP;
  else if (!strcmp(argv[2], "uboot"))
    *sys = BOOT_SYS_UBOOT;
  else if (!strcmp(argv[2], "auto_boot"))
    *sys = BOOT_SYS_AUTO;

  return 0;
}

/**
 * @brief
 *
 * @param cmdtp
 * @param flag
 * @param argc
 * @param argv
 * @return int
 */
static int do_k230_boot(struct cmd_tbl *cmdtp, int flag, int argc,
                        char *const argv[]) {
  int ret = 0;
  en_boot_sys_t sys;
  boot_medium_e bootmod = g_boot_medium;
  ulong cipher_addr = k230_get_encrypted_image_decrypt_addr();

  ret = k230_boot_prepare_args(argc, argv, cipher_addr, &sys, &bootmod);
  if (ret)
    return ret;

  g_boot_medium = bootmod;
  if (sys == BOOT_SYS_ADDR)
    ret = k230_img_boot_sys_bin((firmware_head_s *)cipher_addr);
  else
    ret = k230_img_load_boot_sys(sys);

  return ret;
}

#define K230_BOOT_HELP                                                         \
  " <auto|sdio1|sdio0|spinor|spinand|mem> "                                    \
  "<auto_boot|rtt|linux|qbc|fdb|sensor|ai|speckle|rtapp|uboot|addr> [len]\n"   \
  "qbc---quick boot cfg\n"                                                     \
  "fdb---face database\n"                                                      \
  "sensor---sensor cfg\n"                                                      \
  "ai---ai mode cfg\n"                                                         \
  "speckle---speckle cfg\n"                                                    \
  "rtapp---rtt app\n"                                                          \
  "auto_boot---auto boot\n"                                                    \
  "uboot---boot uboot\n"

U_BOOT_CMD_COMPLETE(k230_boot, 6, 0, do_k230_boot, NULL, K230_BOOT_HELP, NULL);

typedef void (*func_app_entry)(void);
static int k230_boot_baremetal(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	static	ulong	boot_address , boot_size, boot_cpu;

	if (argc < 4)
		return CMD_RET_USAGE;

	boot_cpu = hextoul(argv[1], NULL);
	boot_address = hextoul(argv[2], NULL);
	boot_size = hextoul(argv[3], NULL);

	flush_cache(boot_address, boot_size);
	printf("boot_cpu = %ld boot_address = 0x%lx boot_size=0x%lx\n", boot_cpu, boot_address, boot_size);
	if(boot_cpu)
	{
		writel(boot_address, (void*)0x91102104ULL);//cpu1_hart_rstvec
		udelay(100);
		writel(0x10001000,(void*)0x9110100c);
		udelay(100);
		writel(0x10001,(void*)0x9110100c);
		udelay(100);
		writel(0x10000,(void*)0x9110100c);
	}
	else
	{
		func_app_entry app_entry = (void *)(long)boot_address;
		app_entry();
	}

    return 0;
}

U_BOOT_CMD_COMPLETE(
	boot_baremetal, 4, 1, k230_boot_baremetal,
	"boot_baremetal",
	"\n boot_baremetal cpu addr size\n", NULL
);