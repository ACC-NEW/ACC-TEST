/*
 * arch/sh/boards/st/mb442/setup.c
 *
 * Copyright (C) 2005 STMicroelectronics Limited
 * Author: Stuart Menefy (stuart.menefy@st.com)
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * STMicroelectronics STb7100 Reference board support.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
//#include <linux/lirc.h>
#include <media/lirc.h>
#include <linux/phy.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/physmap.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7100.h>
#include <asm/irl.h>
#include <linux/bpa2.h>

#define MB442_PIO_STE100P_RESET stm_gpio(3, 6)
#define MB442_PIO_FLASH_VPP stm_gpio(2, 7)

const char *LMI_VID_partalias[] = { "BPA2_Region1", "coredisplay-video", "gfx-memory", "v4l2-video-buffers", "v4l2-coded-video-buffers", NULL };
const char *LMI_SYS_partalias[] = { "BPA2_Region0", "bigphysarea", NULL };

static struct bpa2_partition_desc bpa2_parts_table[] = {
      {
	      .name  = "LMI_VID",
	      .start = 0x10800000, 
	      .size  = 0x03800000, //57344 KB by 0x03800000, 106496 KB by 0x06800000
	      .flags = 0,
	      .aka   = LMI_VID_partalias
      },
      {
	      .name  = "LMI_SYS",
	      .start = 0,
	      .size  = 0x01700000, //Wir brauchen mehr Speicher fuer Dual Record
	      .flags = 0,
	      .aka   = LMI_SYS_partalias
      }
};

void __init mb442_setup(char** cmdline_p)
{
	printk("STMicroelectronics STb7100 Reference board initialisation\n");

	stx7100_early_device_init();

	stx7100_configure_asc(2, &(struct stx7100_asc_config) {
			.hw_flow_control = 0,
			.is_console = 1, });
	stx7100_configure_asc(3, &(struct stx7100_asc_config) {
			.hw_flow_control = 0,
			.is_console = 0, });

  	bpa2_init(bpa2_parts_table, ARRAY_SIZE(bpa2_parts_table));
}

static struct resource mb442_smc91x_resources[] = {
	[0] = {
		.start	= 0x02000300,
		.end	= 0x02000300 + 0xff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRL0_IRQ,
		.end	= IRL0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mb442_smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(mb442_smc91x_resources),
	.resource	= mb442_smc91x_resources,
};

static void mb442_set_vpp(struct map_info *info, int enable)
{
	gpio_set_value(MB442_PIO_FLASH_VPP, enable);
}

static struct mtd_partition mtd_parts_table[] =
{
	{
		.name = "Boot Firmware",
		.size =   0x00040000,	//u-boot 0x00000000-0xa003ffff  256k
		.offset = 0x00000000,
		mask_flags: 0
	},
	{
		.name = "Boot Config",
		.size =   0x00020000,	//boot config 0xa0040000-0xa005ffff  128k
		.offset = 0x00040000,
 	},
	{
		.name = "Kernel",
		.size =   0x00280000,	//kernel 0x40000-0x2dffff 2.5MB
		.offset = 0x00060000,
	},
	{
		.name = "ROOTFS (Backup)",
		.size =   0x002E0000,
		.offset = 0x00240000,
	},
 	{
		.name = "Full without bootloader",
		.size =   0x00580000,
		.offset = 0x00040000,
	},
};

static struct platform_device mb442_physmap_flash = {
	.name		= "physmap-flash",
	.id		= -1,
	.num_resources	= 1,
	.resource	= (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0, 8*1024*1024),
	},
	.dev.platform_data = &(struct physmap_flash_data) {
		.width		= 2,
		.set_vpp	= mb442_set_vpp,
    	.nr_parts	= ARRAY_SIZE(mtd_parts_table),
    	.parts		= mtd_parts_table
	},
};


static int mb442_phy_reset(void* bus)
{
	gpio_set_value(MB442_PIO_STE100P_RESET, 1);
	udelay(1);
	gpio_set_value(MB442_PIO_STE100P_RESET, 0);
	udelay(1);
	gpio_set_value(MB442_PIO_STE100P_RESET, 1);

	return 1;
}

#define STMMAC_PHY_ADDR 2
static int stmmac_phy_irqs[PHY_MAX_ADDR] = {
	[STMMAC_PHY_ADDR] = IRL3_IRQ,
};
static struct stmmac_mdio_bus_data stmmac_mdio_bus = {
//	.bus_id = 0,
	.phy_reset = mb442_phy_reset,
	.phy_mask = 1,
	.irqs = stmmac_phy_irqs,
};

static struct platform_device *mb442_devices[] __initdata = {
	&mb442_smc91x_device,
	&mb442_physmap_flash,
};

static int __init mb442_device_init(void)
{
	stx7100_configure_sata();

/*	stx7100_configure_pwm(&(struct stx7100_pwm_config) {
			.out0_enabled = 0,
			.out1_enabled = 1, });*/

	stx7100_configure_pwm(&(struct stx7100_pwm_config) {
			.pwm_channel_config[0].enabled = 0,
                        .pwm_channel_config[1].enabled = 1, });

	stx7100_configure_ssc_i2c(0);
	stx7100_configure_ssc_i2c(1);
	stx7100_configure_ssc_i2c(2);

	stx7100_configure_usb();

	stx7100_configure_lirc(&(struct stx7100_lirc_config) {
			.rx_mode = stx7100_lirc_rx_mode_ir,
			.tx_enabled = 0,
			.tx_od_enabled = 0, });

	gpio_request(MB442_PIO_FLASH_VPP, "Flash VPP");
	gpio_direction_output(MB442_PIO_FLASH_VPP, 0);

	gpio_request(MB442_PIO_STE100P_RESET, "STE100P reset");
	gpio_direction_output(MB442_PIO_STE100P_RESET, 1);

	stx7100_configure_ethernet(&(struct stx7100_ethernet_config) {
			.mode = stx7100_ethernet_mode_mii,
			.ext_clk = 0,
			.phy_bus = 0,
			.phy_addr = STMMAC_PHY_ADDR,
			.mdio_bus_data = &stmmac_mdio_bus,
		});

	return platform_add_devices(mb442_devices,
			ARRAY_SIZE(mb442_devices));
}
device_initcall(mb442_device_init);
