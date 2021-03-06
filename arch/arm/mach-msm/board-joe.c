
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/usb/mass_storage_function.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>
#include <linux/zte_memlog.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_serial_hs.h>
#include <mach/memory.h>
#include <mach/msm_battery.h>
#if defined( CONFIG_TOUCHSCREEN_MSM_LEGACY)
#include <mach/msm_touch.h>
#elif defined( CONFIG_TOUCHSCREEN_MSM)
#include <mach/msm_ts.h>
#endif

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h> 
#include <linux/android_pmem.h>
#include <mach/camera.h>

#include "devices.h"
#include "socinfo.h"
#include "clock.h"
#include "msm-keypad-devices.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include "pm.h"
#ifdef CONFIG_ARCH_MSM7X27
#include <linux/msm_kgsl.h>
#endif

#include <msm_usb_config.h> 

#include <linux/lis302dl.h>
#ifdef CONFIG_ARCH_MSM7X25
#define MSM_PMEM_MDP_SIZE	0xb21000
#define MSM_PMEM_ADSP_SIZE	0x97b000
#define MSM_FB_SIZE		0x200000
#define PMEM_KERNEL_EBI1_SIZE	0x80000
#endif

#ifdef CONFIG_ARCH_MSM7X27
#define MSM_PMEM_MDP_SIZE	0x1C91000  
#define MSM_PMEM_ADSP_SIZE	0xAE4000
#define MSM_FB_SIZE		0x200000
#define MSM_GPU_PHYS_SIZE	SZ_2M
#define PMEM_KERNEL_EBI1_SIZE	0x200000
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define MSM_RAM_CONSOLE_PHYS  0x02900000
#define MSM_RAM_CONSOLE_SIZE  SZ_1M
#endif

static smem_global *global;

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
static int g_zte_ftm_flag_fixup;
#endif
static struct resource smc91x_resources[] = {
	[0] = {
		.start	= 0x9C004300,
		.end	= 0x9C0043ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(132),
		.end	= MSM_GPIO_TO_INT(132),
		.flags	= IORESOURCE_IRQ,
	},
};

#ifdef CONFIG_USB_FUNCTION

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9015,
		.functions	    = 0x12 
	},
	{
		.product_id         = 0xF000,
		.functions	    = 0x02, 
	},
	{
		.product_id         = 0xF005,
		.functions	    = 0x03,
	},

	{
		.product_id         = 0x8080,
		.functions	    = 0x34,
	},
	{
		.product_id         = 0x8082,
		.functions	    = 0x0314,
	},
	{
		.product_id         = 0x8085,
		.functions	    = 0x25314,
	},
	{
		.product_id         = 0x9016,
		.functions	    = 0x764,
	},
	{
		.product_id         = 0x9017,
		.functions	    = 0x2764,
	},
	{
		.product_id         = 0x9018,
		.functions	    = 0x27614,
	},
	{
		.product_id         = 0xF009,
		.functions	    = 0x08,
	}
};
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0x9018,
	.functions	= 0x27614,
	.version	= 0x0100,
	.serial_number  = "1234567890ABCDEF",
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
	.nluns = 1,
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif
static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};


static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static int msm_hsusb_rpc_phy_reset(void __iomem *addr)
{
	return msm_hsusb_phy_reset();
}

static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,
	.phy_reset	= msm_hsusb_rpc_phy_reset,
	.pmic_notif_init         = msm_pm_app_rpc_init,
	.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
	.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
	.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
	.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
};

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(HEADSET_AND_SPEAKER, 26),
	SND(CURRENT, 28),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP)| \
	(1<<MSM_ADSP_CODEC_MP3))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP)| \
	(1<<MSM_ADSP_CODEC_MP3))
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), 
	DEC_INFO("AUDPLAY1TASK", 14, 1, 5),  
	DEC_INFO("AUDPLAY2TASK", 15, 2, 5),  
	DEC_INFO("AUDPLAY3TASK", 16, 3, 4),  
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,

	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = "7k_handset",
	},
};


#define LCDC_CONFIG_PROC          21
#define LCDC_UN_CONFIG_PROC       22
#define LCDC_API_PROG             0x30000066
#define LCDC_API_VERS             0x00010001

#ifdef CONFIG_ZTE_PLATFORM
#define GPIO_LCD_RESET_OUT 	    91
#define GPIO_LCD_SPI_CS_OUT    	122
#define GPIO_LCD_SPI_SDO_OUT 	123
#define GPIO_LCD_SPI_SCLK_OUT 	124 
#define GPIO_LCD_SPI_SDI_IN 	132
#else
#define GPIO_OUT_132    132
#define GPIO_OUT_131    131
#define GPIO_OUT_103    103
#define GPIO_OUT_102    102
#define GPIO_OUT_88     88
#endif

static struct msm_rpc_endpoint *lcdc_ep;

static int msm_fb_lcdc_config(int on)
{
	int rc = 0;
	struct rpc_request_hdr hdr;

	if (on)
		pr_info("lcdc config\n");
	else
		pr_info("lcdc un-config\n");

	lcdc_ep = msm_rpc_connect_compatible(LCDC_API_PROG, LCDC_API_VERS, 0);
	if (IS_ERR(lcdc_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",
			__func__, PTR_ERR(lcdc_ep));
		return -EINVAL;
	}

	rc = msm_rpc_call(lcdc_ep,
				(on) ? LCDC_CONFIG_PROC : LCDC_UN_CONFIG_PROC,
				&hdr, sizeof(hdr),
				5 * HZ);
	if (rc)
		printk(KERN_ERR
			"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	msm_rpc_close(lcdc_ep);
	return rc;
}

#ifdef CONFIG_ZTE_PLATFORM
static int gpio_array_num[] = {
				GPIO_LCD_SPI_SCLK_OUT, 
				GPIO_LCD_SPI_CS_OUT, 
				GPIO_LCD_SPI_SDI_IN, 
				GPIO_LCD_SPI_SDO_OUT, 
				GPIO_LCD_RESET_OUT,
				};

static void lcdc_himax_gpio_init(void)
{
	if (gpio_request(GPIO_LCD_SPI_SCLK_OUT, "spi_clk"))
		pr_err("failed to request gpio spi_clk\n");
	if (gpio_request(GPIO_LCD_SPI_CS_OUT, "spi_cs"))
		pr_err("failed to request gpio spi_cs\n");
	if (gpio_request(GPIO_LCD_SPI_SDI_IN, "spi_sdi"))
		pr_err("failed to request gpio spi_sdi\n");
	if (gpio_request(GPIO_LCD_SPI_SDO_OUT, "spi_sdoi"))
		pr_err("failed to request gpio spi_sdoi\n");
	if (gpio_request(GPIO_LCD_RESET_OUT, "gpio_dac"))
		pr_err("failed to request gpio_dac\n");
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_CFG(GPIO_LCD_SPI_SCLK_OUT, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_LCD_SPI_CS_OUT, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_LCD_SPI_SDI_IN, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),
	GPIO_CFG(GPIO_LCD_SPI_SDO_OUT, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_LCD_RESET_OUT,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), };
#else
static int gpio_array_num[] = {
				GPIO_OUT_132, 
				GPIO_OUT_131, 
				GPIO_OUT_103, 
				GPIO_OUT_102, 
				GPIO_OUT_88
				};

static void lcdc_gordon_gpio_init(void)
{
	if (gpio_request(GPIO_OUT_132, "spi_clk"))
		pr_err("failed to request gpio spi_clk\n");
	if (gpio_request(GPIO_OUT_131, "spi_cs"))
		pr_err("failed to request gpio spi_cs\n");
	if (gpio_request(GPIO_OUT_103, "spi_sdi"))
		pr_err("failed to request gpio spi_sdi\n");
	if (gpio_request(GPIO_OUT_102, "spi_sdoi"))
		pr_err("failed to request gpio spi_sdoi\n");
	if (gpio_request(GPIO_OUT_88, "gpio_dac"))
		pr_err("failed to request gpio_dac\n");
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_CFG(GPIO_OUT_132, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_131, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_103, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_102, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_88,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};
#endif

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

#ifdef CONFIG_ZTE_PLATFORM
static void lcdc_himax_config_gpios(int enable)
#else
static void lcdc_gordon_config_gpios(int enable)
#endif
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static char *msm_fb_lcdc_vreg[] = {
	"gp5"
};

#define MSM_FB_LCDC_VREG_OP(name, op) \
do { \
	vreg = vreg_get(0, name); \
	if (vreg_##op(vreg)) \
		printk(KERN_ERR "%s: %s vreg operation failed \n", \
			(vreg_##op == vreg_enable) ? "vreg_enable" \
				: "vreg_disable", name); \
} while (0)

static void msm_fb_lcdc_power_save(int on)
{
	struct vreg *vreg;
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_fb_lcdc_vreg); i++) {
		if (on)
			MSM_FB_LCDC_VREG_OP(msm_fb_lcdc_vreg[i], enable);
		else{
			MSM_FB_LCDC_VREG_OP(msm_fb_lcdc_vreg[i], disable);
		#ifndef CONFIG_ZTE_PLATFORM
			gpio_tlmm_config(GPIO_CFG(GPIO_OUT_88, 0,
			GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_set_value(88, 0);
			mdelay(15);
			gpio_set_value(88, 1);
			mdelay(15);
		#endif
			}
		}
}
static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = msm_fb_lcdc_config,
	.lcdc_power_save   = msm_fb_lcdc_power_save,
};

static struct msm_panel_common_pdata lcdc_himax_panel_data = {
	.panel_config_gpio = lcdc_himax_config_gpios,
	.gpio_num          = gpio_array_num,
};

static struct platform_device lcdc_himax_panel_device = {
	.name   = "lcdc_himax_wqvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_himax_panel_data,
	}
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;

	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
		if (!strcmp(name, "lcdc_gordon_vga"))
			ret = 0;
		else
			ret = -ENODEV;
	}

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_WAKE,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_PCM_DOUT,
	BT_PCM_DIN,
	BT_PCM_SYNC,
	BT_PCM_CLK,
	BT_HOST_WAKE,
};

static unsigned bt_config_power_on[] = {
	GPIO_CFG(90, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	
	GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* PCM_DIN */
	
	//GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	//GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(70, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	
	GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* HOST_WAKE */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(90, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* WAKE */ 
	GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* HOST_WAKE */
};

static int bluetooth_power(int on)
{
	struct vreg *vreg_bt;
	int pin, rc;

	printk(KERN_DEBUG "%s\n", __func__);

	/* do not have vreg bt defined, gp6 is the same */
	/* vreg_get parameter 1 (struct device *) is ignored */
	vreg_bt = vreg_get(NULL, "gp6");

	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_bt));
		return PTR_ERR(vreg_bt);
	}

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}

		/* units of mV, steps of 50 mV */
		rc = vreg_set_level(vreg_bt, 1800); 
		if (rc) {
			printk(KERN_ERR "%s: vreg set level failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_enable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg enable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		if((gpio_direction_output(20, 1))<0)
		{
						printk(KERN_ERR "gpio_direction_output: %d failed!\n", 20);
		}
	} else {
		rc = vreg_disable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg disable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
    if((gpio_direction_output(20, 0))<0)
    {
        		printk(KERN_ERR "gpio_direction_output: %d failed!\n", 20);
		}
	}
	return 0;
}

static void __init bt_power_init(void)
{
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif

#ifdef CONFIG_ARCH_MSM7X27
static struct resource kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "kgsl_yamato_irq", 
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
	},
};

static struct kgsl_platform_data kgsl_pdata;

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(kgsl_resources),
	.resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};
#endif

static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds-status",
	.id = -1,
};

static struct gpio_led android_led_list[] = {
	{
		.name = "button-backlight",
		.gpio = 89,
	},
};

static struct gpio_led_platform_data android_leds_data = {
	.num_leds	= ARRAY_SIZE(android_led_list),
	.leds		= android_led_list,
};

static struct platform_device android_leds = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &android_leds_data,
	},
};

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 90,   
		.end	= 90,     
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(83),
		.end	= MSM_GPIO_TO_INT(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};



#ifdef CONFIG_ZTE_R750

static struct resource modemctl_ifxgsm_resources[] = {
	{
		.name	= "GSM_KEY_ON",		
		.start	= 72,
		.end	= 72,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "GSM_RESET",		//GPIO_72
		.start	= 73,
		.end	= 73,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "G2C_IRQ_IO",		//GPIO_42
		.start	= 42,
		.end	= 42,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "G2C_IRQ",		//GPIO_42,to wake gsm modem
		.start	= MSM_GPIO_TO_INT(42),
		.end	= MSM_GPIO_TO_INT(42),
		.flags	= IORESOURCE_IRQ,
	},
      {
		.name	= "C2G_READY",		//GPIO_75, to interrupt gsm for ready, and show the state of ap sleep.
		.start	= 75,
		.end	= 75,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "C2G_IRQ",		//GPIO_74, to interrupt gsm for wake up
		.start	= 74,
		.end	= 74,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "G2C_READY_IO",		//GPIO_78, here just usded by gsm to show the state of modem sleep.
		.start	= 78,
		.end	= 78,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "G2C_READY",		//GPIO_78, gsm interrupt ap for ready, and show the state of modem sleep.
		.start	= MSM_GPIO_TO_INT(78),
		.end	= MSM_GPIO_TO_INT(78),
		.flags	= IORESOURCE_IRQ,
	},
	
	{
		.name	= "GSM_WORKING",		//GPIO_16, gsm working state
		.start	= 16,
		.end	= 16,
		.flags	= IORESOURCE_IO,
	},
};


static struct platform_device modemctl_device = {
	.name = "msm_modemctl",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(modemctl_ifxgsm_resources),
	.resource	= modemctl_ifxgsm_resources,
};


#define MSM_GPIO_UARTDM2_RX	    21
#define MSM_GPIO_UARTDM2_TX	    108
#define MSM_GPIO_UARTDM2_RTS	    19
#define MSM_GPIO_UARTDM2_CTS	    20


static unsigned modemctl_config_rx = GPIO_CFG(MSM_GPIO_UARTDM2_RX, 5, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA);	/* Rx */
static unsigned modemctl_config_tx = GPIO_CFG(MSM_GPIO_UARTDM2_TX, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);	/* Tx */


static int modemctl_init_uartdm2_gpio(void)
{
	int rc;
      printk(KERN_ERR "@@@@@@ enter %s: config the GPIO UART DM2 ++\n",__func__);


	rc = gpio_tlmm_config(modemctl_config_rx,GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, MSM_GPIO_UARTDM2_RX, rc);
		return -EIO;
	}

	rc = gpio_request(MSM_GPIO_UARTDM2_RX, "modemctl");
	if(rc)
	{
		printk(KERN_ERR "gpio_request: %d failed!\n", MSM_GPIO_UARTDM2_RX);
	}
    
    	rc = gpio_tlmm_config(modemctl_config_tx,GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, MSM_GPIO_UARTDM2_TX, rc);
		return -EIO;
	}
	rc = gpio_request(MSM_GPIO_UARTDM2_TX, "modemctl");
	if(rc)
	{
		printk(KERN_ERR "gpio_request: %d failed!\n", MSM_GPIO_UARTDM2_TX);
	}


    
      printk(KERN_ERR "@@@@@@ exit %s: config the GPIO UART DM2 --\n",__func__);
    
	return 0;
}
#endif



static struct i2c_board_info i2c_devices[] = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_MT9P012_KM
	{
		I2C_BOARD_INFO("mt9p012_km", 0x6C >> 2),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif
#ifdef CONFIG_VB6801
	{
		I2C_BOARD_INFO("vb6801", 0x20),
	},
#endif

#ifdef CONFIG_MT9P111
 
    {
        I2C_BOARD_INFO("mt9p111", 0x7A >> 1),
    },
#endif

#ifdef CONFIG_MT9T11X

    {
        I2C_BOARD_INFO("mt9t11x", 0x7A >> 1),
    },
#endif

#ifdef CONFIG_MT9V113

    {
        I2C_BOARD_INFO("mt9v113", 0x78 >> 1),
    },
#endif

#ifdef CONFIG_OV5642

    {
        I2C_BOARD_INFO("ov5642", 0x78 >> 1),
    },
#endif
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI
{
		.type         = "synaptics-rmi-ts",
		.addr         = 0x22,
		.irq          = MSM_GPIO_TO_INT(29),
	},
	#endif
	#ifdef CONFIG_TOUCHSCREEN_CYPRESS_I2C_RMI
	{
		.type         = "cypress_touch",
		.addr         = 0x0a,
		.irq          = MSM_GPIO_TO_INT(29),
	},
	#endif
};

static struct i2c_gpio_platform_data aux_i2c_gpio_data = {
	.sda_pin		= 93,
	.scl_pin		= 92,
	.sda_is_open_drain	= 1,
	.scl_is_open_drain	= 1,
	.udelay			= 2,
};

static struct platform_device aux_i2c_gpio_device = {
	.name		= "i2c-gpio",
	.id		= 1,
	.dev		= {
		.platform_data	= &aux_i2c_gpio_data
	},
};

static struct i2c_gpio_platform_data aux2_i2c_gpio_data = {
	.sda_pin		= 109,
	.scl_pin		= 107,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 1,
	.udelay			= 2,
};

static struct platform_device aux2_i2c_gpio_device = {
	.name		= "i2c-gpio",
	.id		= 2,
	.dev		= {
		.platform_data	= &aux2_i2c_gpio_data
	},
};

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
  
#if 0     
    GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
    GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
    GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
    GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
    GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
    GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
    GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
    GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
    GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
    GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
    GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
    GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
#else
    GPIO_CFG(4,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <0> */
    GPIO_CFG(5,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <1> */
    GPIO_CFG(6,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <2> */
    GPIO_CFG(7,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <3> */
    GPIO_CFG(8,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <4> */
    GPIO_CFG(9,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <5> */
    GPIO_CFG(10, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <6> */
    GPIO_CFG(11, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_DATA <7> */
    GPIO_CFG(12, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_PCLK */
    GPIO_CFG(13, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_HSYNC */
    GPIO_CFG(14, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* CIF_VSYNC */
    GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* CIF_MCLK */
#endif
};

static uint32_t camera_on_gpio_table[] = {
 
    GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
    GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
    GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
    GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
    GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
    GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
    GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
    GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
    GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
    GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
    GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
    GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static struct vreg *vreg_gp2;
static struct vreg *vreg_gp3;

static void msm_camera_vreg_config(int vreg_en)
{
	int rc;

	if (vreg_gp2 == NULL) {
		vreg_gp2 = vreg_get(NULL, "gp2");
		if (IS_ERR(vreg_gp2)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp2", PTR_ERR(vreg_gp2));
			return;
		}

		rc = vreg_set_level(vreg_gp2, 1800);
		if (rc) {
			printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

		rc = vreg_set_level(vreg_gp3, 2850);
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
	}
}

static void config_camera_on_gpios(void)
{
	int vreg_en = 1;

	if (machine_is_msm7x25_ffa() ||
	    machine_is_msm7x27_ffa())
		msm_camera_vreg_config(vreg_en);

	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	int vreg_en = 0;

	if (machine_is_msm7x25_ffa() ||
	    machine_is_msm7x27_ffa())
		msm_camera_vreg_config(vreg_en);

	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}


int32_t msm_camera_power_backend(enum msm_camera_pwr_mode_t pwr_mode)
{
    struct vreg *vreg_cam_dvdd  = NULL;
    struct vreg *vreg_cam_avdd  = NULL;
    struct vreg *vreg_cam_iovdd = NULL;
    int32_t rc_cam_dvdd, rc_cam_avdd, rc_cam_iovdd;

    CDBG("%s: entry\n", __func__);


    vreg_cam_dvdd  = vreg_get(0, "gp2");
    vreg_cam_iovdd = vreg_get(0, "msmp");
    vreg_cam_avdd  = vreg_get(0, "gp3");
    if ((!vreg_cam_dvdd) || (!vreg_cam_iovdd) || (!vreg_cam_avdd))
    {
        CCRT("%s: vreg_get failed!\n", __func__);
        return -EIO;
    }

    switch (pwr_mode)
    {
        case MSM_CAMERA_PWRUP_MODE:
        {
            rc_cam_dvdd = vreg_set_level(vreg_cam_dvdd, 1800);
            if (rc_cam_dvdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_dvdd = vreg_enable(vreg_cam_dvdd);
            if (rc_cam_dvdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            rc_cam_iovdd = vreg_set_level(vreg_cam_iovdd, 2800);
            if (rc_cam_iovdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_iovdd = vreg_enable(vreg_cam_iovdd);
            if (rc_cam_iovdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(2);

            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, 2600);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(500);

            break;
        }
        case MSM_CAMERA_STANDBY_MODE:
        {
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

            break;
        }
        case MSM_CAMERA_NORMAL_MODE:
        {
            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, 2600);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            break;
        }
        case MSM_CAMERA_PWRDWN_MODE:
        {
      
            rc_cam_dvdd  = vreg_disable(vreg_cam_dvdd);
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if ((rc_cam_dvdd) || (rc_cam_avdd))
            {
                CCRT("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

            break;
        }
        default:
        {
            CCRT("%s: parameter not supported!\n", __func__);
            return -EIO;
        }
    }

    return 0;
}


int32_t msm_camera_power_frontend(enum msm_camera_pwr_mode_t pwr_mode)
{
    struct vreg *vreg_cam_dvdd  = NULL;
    struct vreg *vreg_cam_avdd  = NULL;
    struct vreg *vreg_cam_iovdd = NULL;
    int32_t rc_cam_dvdd, rc_cam_avdd, rc_cam_iovdd;

    CDBG("%s: entry\n", __func__);

 
    vreg_cam_dvdd  = vreg_get(0, "gp2");
    vreg_cam_iovdd = vreg_get(0, "gp4");
    vreg_cam_avdd  = vreg_get(0, "gp3");
    if ((!vreg_cam_dvdd) || (!vreg_cam_iovdd) || (!vreg_cam_avdd))
    {
        CCRT("%s: vreg_get failed!\n", __func__);
        return -EIO;
    }

    switch (pwr_mode)
    {
        case MSM_CAMERA_PWRUP_MODE:
        {
            rc_cam_dvdd = vreg_set_level(vreg_cam_dvdd, 1800);
            if (rc_cam_dvdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_dvdd = vreg_enable(vreg_cam_dvdd);
            if (rc_cam_dvdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(1);

            rc_cam_iovdd = vreg_set_level(vreg_cam_iovdd, 2800);
            if (rc_cam_iovdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_iovdd = vreg_enable(vreg_cam_iovdd);
            if (rc_cam_iovdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(2);

            rc_cam_avdd = vreg_set_level(vreg_cam_avdd, 2600);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_set_level failed!\n", __func__);
                return -EIO;
            }

            rc_cam_avdd = vreg_enable(vreg_cam_avdd);
            if (rc_cam_avdd)
            {
                CCRT("%s: vreg_enable failed!\n", __func__);
                return -EIO;
            }

            mdelay(500);

            break;
        }
        case MSM_CAMERA_STANDBY_MODE:
        {
            CCRT("%s: MSM_CAMERA_STANDBY_MODE not supported!\n", __func__);
            return -EIO;
        }
        case MSM_CAMERA_NORMAL_MODE:
        {
            CCRT("%s: MSM_CAMERA_NORMAL_MODE not supported!\n", __func__);
            return -EIO;
        }
        case MSM_CAMERA_PWRDWN_MODE:
        {
            rc_cam_dvdd  = vreg_disable(vreg_cam_dvdd);
            rc_cam_iovdd = vreg_disable(vreg_cam_iovdd);
            rc_cam_avdd  = vreg_disable(vreg_cam_avdd);
            if ((rc_cam_dvdd) || (rc_cam_iovdd) || (rc_cam_avdd))
            {
                CCRT("%s: vreg_disable failed!\n", __func__);
                return -EIO;
            }

            break;
        }
        default:
        {
            CCRT("%s: parameter not supported!\n", __func__);
            return -EIO;
        }
    }

    return 0;
}


int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                  uint32_t gpio_switch,
                                  uint32_t switch_val)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(gpio_switch, data->sensor_name);
    if (0 == rc)
    {
        rc = gpio_direction_output(gpio_switch, switch_val);

        mdelay(1);
    }

    gpio_free(gpio_switch);

    return rc;
}

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

#ifdef CONFIG_MT9D112
static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 88,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif

#ifdef CONFIG_MT9P012_KM
static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_km_data = {
	.sensor_name    = "mt9p012_km",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 88,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_mt9p012_km = {
	.name      = "msm_camera_mt9p012_km",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_km_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

#ifdef CONFIG_VB6801
static struct msm_camera_sensor_info msm_camera_sensor_vb6801_data = {
	.sensor_name    = "vb6801",
	.sensor_reset   = 89,
	.sensor_pwd     = 88,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_vb6801 = {
	.name      = "msm_camera_vb6801",
	.dev       = {
		.platform_data = &msm_camera_sensor_vb6801_data,
	},
};
#endif

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
	._fsrc.pmic_src.low_current  = 30,
	._fsrc.pmic_src.high_current = 100,
};

#ifdef CONFIG_MT9P111

static struct msm_camera_sensor_flash_data flash_mt9p111 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
}; 

static struct msm_camera_sensor_info msm_camera_sensor_mt9p111_data = {
	.sensor_name    = "mt9p111",
	.sensor_reset   = 2,
	.sensor_pwd     = 1,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9p111
};

static struct platform_device msm_camera_sensor_mt9p111 = {
	.name      = "msm_camera_mt9p111",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p111_data,
	},
};
#endif

#ifdef CONFIG_MT9T11X

static struct msm_camera_sensor_flash_data flash_mt9t11x = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};
 
static struct msm_camera_sensor_info msm_camera_sensor_mt9t11x_data = {
	.sensor_name    = "mt9t11x",
	.sensor_reset   = 2,
	.sensor_pwd     = 1,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9t11x
};

static struct platform_device msm_camera_sensor_mt9t11x = {
	.name      = "msm_camera_mt9t11x",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t11x_data,
	},
};
#endif

#ifdef CONFIG_MT9V113
 
static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};
 
static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name    = "mt9v113",
	.sensor_reset   = 0,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9v113
};

static struct platform_device msm_camera_sensor_mt9v113 = {
	.name      = "msm_camera_mt9v113",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9v113_data,
	},
};
#endif

#ifdef CONFIG_OV5642

static struct msm_camera_sensor_flash_data flash_ov5642 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};
 
static struct msm_camera_sensor_info msm_camera_sensor_ov5642_data = {
	.sensor_name    = "ov5642",
	.sensor_reset   = 2,
	.sensor_pwd     = 1,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_ov5642
};

static struct platform_device msm_camera_sensor_ov5642 = {
	.name      = "msm_camera_ov5642",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov5642_data,
	},
};
#endif

#endif

#if defined( CONFIG_TOUCHSCREEN_MSM_LEGACY)
struct msm_ts_platform_data msm_tssc_pdata ={
	.x_max = 239,
	.y_max = 319,
	.pressure_max =255,
};
#elif defined( CONFIG_TOUCHSCREEN_MSM)
struct msm_ts_platform_data msm_tssc_pdata = {
	.min_x = 0,
	.max_x = 239,
	.min_y = 0,
	.max_y = 319,
	.min_press =0,
	.max_press =255,
	.inv_y = 955,
};
#endif
#if 0
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 2800,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity	= &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage   = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage  = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
		/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};
#else
static u32 msm_calculate_batt_capacity(u32 current_voltage);

typedef struct 
{
    u32 voltage;
    u32 capacity;
} BattFuelCapacity;

static const BattFuelCapacity fuelCapacity[] = {
   {3388, 0},                      /*   0 % 3388 */
   {3500, 10},                     /*  10%,actual only 1.4% */
   {3660, 20},                     /*  20%, actual only 10.6% */
   {3709, 30},                     /*  30% actual only 19% */
   {3758, 40},                     /*  40% actual only 38% */
   {3798, 50},                     /*  50% actual only 51% */
   {3839, 60},                     /*  60% actual only 60% */
   {3901, 70},                     /*  70% actual only 70% */
   {3963, 80},                     /*  80% actual only 80% */
   {4056, 90},                     /*  90% actual only 91% */
   {4150, 100}                     /* 100% 4147*/
};

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3388,
	.voltage_max_design	= 4150,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity	= &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
    u8 step = sizeof(fuelCapacity)/sizeof(BattFuelCapacity);
    u8 table_count;

    if (current_voltage <= fuelCapacity[0].voltage)
    {
        return 0;
    }
    else if (current_voltage >= fuelCapacity[step-1].voltage)
    {
        return 100;
    }
    else
    {    
        for (table_count = 1; table_count< step; table_count++)
        {
            if (current_voltage <= fuelCapacity[table_count].voltage)
            {
                return (fuelCapacity[table_count-1].capacity 
                    + ((current_voltage - fuelCapacity[table_count-1].voltage)*10
                    /(fuelCapacity[table_count].voltage - 
                    fuelCapacity[table_count-1].voltage)));
            }
        }
    }

    printk("%s: error\n", __func__);
    return 0;
}

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

#endif

#if 1
/* ATHENV */
static struct platform_device msm_wlan_ar6000_pm_device = {
	.name		= "wlan_ar6000_pm_dev",
	.id		= 1,
	.num_resources	= 0,
	.resource	= NULL,
};
/* ATHENV */
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.start	= MSM_RAM_CONSOLE_PHYS,
		.end	= MSM_RAM_CONSOLE_PHYS + MSM_RAM_CONSOLE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resource),
	.resource       = ram_console_resource,
};
#endif

static struct platform_device *devices[] __initdata = {
#if 1
	&msm_wlan_ar6000_pm_device,
#endif

#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
	&msm_device_hsusb_host,
	&msm_device_hsusb_peripheral,
#ifdef CONFIG_USB_FUNCTION
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID
	&android_usb_device,
#endif
	&msm_device_i2c,
	&aux_i2c_gpio_device,  
	&aux2_i2c_gpio_device,  
	&smc91x_device,
#if defined( CONFIG_TOUCHSCREEN_MSM_LEGACY) || defined( CONFIG_TOUCHSCREEN_MSM)
	&msm_device_tssc,
#endif
	&android_pmem_kernel_ebi1_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&msm_fb_device,
#ifdef CONFIG_ZTE_PLATFORM
	&lcdc_himax_panel_device,
#else
	&lcdc_gordon_panel_device,
#endif
	&msm_device_uart_dm1,
#ifdef CONFIG_ZTE_R750
	&msm_device_uart_dm2,
      &modemctl_device,
#endif
#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
	&msm_device_pmic_leds,
	&android_leds, 
	&msm_device_snd,
	&msm_device_adspdec,
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_MT9P012_KM
	&msm_camera_sensor_mt9p012_km,
#endif
#ifdef CONFIG_VB6801
	&msm_camera_sensor_vb6801,
#endif
	&msm_bluesleep_device,
#ifdef CONFIG_ARCH_MSM7X27
	&msm_device_kgsl,
#endif
#ifdef CONFIG_MT9P111

    &msm_camera_sensor_mt9p111,
#endif

#ifdef CONFIG_MT9T11X
  
    &msm_camera_sensor_mt9t11x,
#endif

#ifdef CONFIG_MT9V113
  
    &msm_camera_sensor_mt9v113,
#endif

#ifdef CONFIG_OV5642
  
    &msm_camera_sensor_ov5642,
#endif
	

	&hs_device,
	&msm_batt_device,
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", 0);
	msm_fb_register_device("lcdc", &lcdc_pdata);
}

static struct i2c_board_info aux_i2c_devices[] = {
	{
		.type         = "synaptics-rmi-ts",
		.addr         = 0x22,
		.irq          = MSM_GPIO_TO_INT(29),
	},
	{
		.type         = "cypress_touch",
		.addr         = 0x0a,
		.irq          = MSM_GPIO_TO_INT(29),
	},
	{
		I2C_BOARD_INFO("si4708", 0x10),
	},
};


static struct lis302dl_platform_data gsensor = {
	.gpio_intr1 =  84,
	.gpio_intr2 =  85,
	.scale      =  2 ,
	.int_active_low = 1,
};

static struct i2c_board_info aux2_i2c_devices[] = {
	{
		I2C_BOARD_INFO("akm8973", 0x1c),
	},
	{
		.type = "lis302dl",
		.addr = 0x1d,
		.platform_data = &gsensor,
	},
#ifdef CONFIG_SENSORS_TSL2771	
	{
		.type = "taos",
		.addr = 0x39,
	},
#endif	
};
extern struct sys_timer msm_timer;

static void __init msm7x2x_init_irq(void)
{
	msm_init_irq();
}

static struct msm_acpu_clock_platform_data msm7x2x_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.max_axi_khz = 160000,
};

void msm_serial_debug_init(unsigned int base, int irq,
			   struct device *clk_device, int signal_irq);

#ifdef CONFIG_MMC
static void sdcc_gpio_init(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (gpio_request(51, "sdc1_data_3"))
		pr_err("failed to request gpio sdc1_data_3\n");
	if (gpio_request(52, "sdc1_data_2"))
		pr_err("failed to request gpio sdc1_data_2\n");
	if (gpio_request(53, "sdc1_data_1"))
		pr_err("failed to request gpio sdc1_data_1\n");
	if (gpio_request(54, "sdc1_data_0"))
		pr_err("failed to request gpio sdc1_data_0\n");
	if (gpio_request(55, "sdc1_cmd"))
		pr_err("failed to request gpio sdc1_cmd\n");
	if (gpio_request(56, "sdc1_clk"))
		pr_err("failed to request gpio sdc1_clk\n");
#endif
	if (machine_is_msm7x25_ffa())
		return;

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (gpio_request(62, "sdc2_clk"))
		pr_err("failed to request gpio sdc2_clk\n");
	if (gpio_request(63, "sdc2_cmd"))
		pr_err("failed to request gpio sdc2_cmd\n");
	if (gpio_request(64, "sdc2_data_3"))
		pr_err("failed to request gpio sdc2_data_3\n");
	if (gpio_request(65, "sdc2_data_2"))
		pr_err("failed to request gpio sdc2_data_2\n");
	if (gpio_request(66, "sdc2_data_1"))
		pr_err("failed to request gpio sdc2_data_1\n");
	if (gpio_request(67, "sdc2_data_0"))
		pr_err("failed to request gpio sdc2_data_0\n");
#endif
	if (machine_is_msm7x27_ffa())
		return;

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	if (gpio_request(88, "sdc3_clk"))
		pr_err("failed to request gpio sdc3_clk\n");
	if (gpio_request(89, "sdc3_cmd"))
		pr_err("failed to request gpio sdc3_cmd\n");
	if (gpio_request(90, "sdc3_data_3"))
		pr_err("failed to request gpio sdc3_data_3\n");
	if (gpio_request(91, "sdc3_data_2"))
		pr_err("failed to request gpio sdc3_data_2\n");
	if (gpio_request(92, "sdc3_data_1"))
		pr_err("failed to request gpio sdc3_data_1\n");
	if (gpio_request(93, "sdc3_data_0"))
		pr_err("failed to request gpio sdc3_data_0\n");
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	if (gpio_request(19, "sdc4_data_3"))
		pr_err("failed to request gpio sdc4_data_3\n");
	if (gpio_request(20, "sdc4_data_2"))
		pr_err("failed to request gpio sdc4_data_2\n");
	if (gpio_request(21, "sdc4_data_1"))
		pr_err("failed to request gpio sdc4_data_1\n");
	if (gpio_request(107, "sdc4_cmd"))
		pr_err("failed to request gpio sdc4_cmd\n");
	if (gpio_request(108, "sdc4_data_0"))
		pr_err("failed to request gpio sdc4_data_0\n");
	if (gpio_request(109, "sdc4_clk"))
		pr_err("failed to request gpio sdc4_clk\n");
#endif
}

static unsigned sdcc_cfg_data[][6] = {
	{
	GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	},
	{
	GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	{
	GPIO_CFG(88, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(89, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(90, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(91, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(92, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(93, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	{
	GPIO_CFG(19, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(20, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(21, 4, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(107, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(108, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(109, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	}
};

static unsigned long vreg_sts, gpio_sts;
static unsigned mpp_mmc = 2;
static struct vreg *vreg_mmc;

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int i, rc;

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable)
		set_bit(dev_id, &gpio_sts);
	else
		clear_bit(dev_id, &gpio_sts);

	for (i = 0; i < ARRAY_SIZE(sdcc_cfg_data[dev_id - 1]); i++) {
		rc = gpio_tlmm_config(sdcc_cfg_data[dev_id - 1][i],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, sdcc_cfg_data[dev_id - 1][i], rc);
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			if (machine_is_msm7x25_ffa() ||
					machine_is_msm7x27_ffa()) {
				rc = mpp_config_digital_out(mpp_mmc,
				     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     MPP_DLOGIC_OUT_CTRL_LOW));
			} else
				rc = vreg_disable(vreg_mmc);
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
			rc = mpp_config_digital_out(mpp_mmc,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));
		} else {
			rc = vreg_set_level(vreg_mmc, 2850);
			if (!rc)
				rc = vreg_enable(vreg_mmc);
		}
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

static struct mmc_platform_data msm7x2x_sdcc_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
};

static void (*wifi_status_notify_cb)(int card_present, void *dev_id);
void *wifi_devid;
static int msm_sdcc_register_status_notify(void (*callback)(int card_present, void *dev_id), void *dev_id)
{	
	wifi_status_notify_cb = callback;
	wifi_devid = dev_id;
	printk("%s: callback %p devid %p\n", __func__, callback, dev_id);
	return 0;
}

void wifi_detect_change(int on)
{
	if (wifi_status_notify_cb) {
		printk("%s: callback %p devid %p is called!!\n", __func__, wifi_status_notify_cb, wifi_devid);
		wifi_status_notify_cb(on, wifi_devid);
	}
}
EXPORT_SYMBOL(wifi_detect_change);

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x2x_sdc2_data = {
    .ocr_mask   = MMC_VDD_28_29,
    .translate_vdd  = msm_sdcc_setup_power,
    .mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#if 0
    .sdiowakeup_irq = MSM_GPIO_TO_INT(66),
#endif
    .register_status_notify = msm_sdcc_register_status_notify,
    .dummy52_required = 1,
};
#endif

static void __init msm7x2x_init_mmc(void)
{
	if (!machine_is_msm7x25_ffa() && !machine_is_msm7x27_ffa()) {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}

	sdcc_gpio_init();
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_add_sdcc(1, &msm7x2x_sdcc_data);
#endif

#ifndef CONFIG_ZTE_PLATFORM
	if (machine_is_msm7x25_surf() || machine_is_msm7x27_surf() ||
		machine_is_msm7x27_ffa()) {
#else
	{
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
		msm_add_sdcc(2, &msm7x2x_sdc2_data);  
#endif
	}

#ifndef CONFIG_ZTE_PLATFORM
	if (machine_is_msm7x25_surf() || machine_is_msm7x27_surf()) {
#else
	{
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
		msm_add_sdcc(3, &msm7x2x_sdcc_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
		msm_add_sdcc(4, &msm7x2x_sdcc_data);
#endif
	}
}
#else
#define msm7x2x_init_mmc() do {} while (0)
#endif


static struct msm_pm_platform_data msm7x25_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
};

static struct msm_pm_platform_data msm7x27_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 20000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 20000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 0,
};

static void
msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;
	if (iface) {
		gpio_scl = 95;
		gpio_sda = 96;
	} else {
		gpio_scl = 60;
		gpio_sda = 61;
	}
	if (config_type) {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {

	.clk_freq = 100000,
	.rmutex  = 0,
	.pri_clk = 60,
	.pri_dat = 61,

 
#if 0
	.aux_clk = 95,
	.aux_dat = 96,
#endif

	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (gpio_request(60, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(61, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");

 
#if 0
	if (gpio_request(95, "i2c_sec_clk"))
		pr_err("failed to request gpio i2c_sec_clk\n");
	if (gpio_request(96, "i2c_sec_dat"))
		pr_err("failed to request gpio i2c_sec_dat\n");
#endif

	if (cpu_is_msm7x27())
		msm_i2c_pdata.pm_lat =
		msm7x27_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;
	else
		msm_i2c_pdata.pm_lat =
		msm7x25_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}
#define MSM_GPIO_USB3V3	    21 
static unsigned usb_config_power_on =	GPIO_CFG(MSM_GPIO_USB3V3, 0, 
                                                                                GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA);
static int init_usb3v3(void)
{
	int rc;
	rc = gpio_tlmm_config(usb_config_power_on,GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, MSM_GPIO_USB3V3, rc);
		return -EIO;
	}
	rc = gpio_request(MSM_GPIO_USB3V3, "usb");
	if(!rc)
	{
		gpio_direction_output(MSM_GPIO_USB3V3, 1);
		gpio_set_value(MSM_GPIO_USB3V3, 1);
		printk(KERN_ERR "gpio_request: %d ok!\n", MSM_GPIO_USB3V3);
	}
	else
	{
		printk(KERN_ERR "gpio_request: %d failed!\n", MSM_GPIO_USB3V3);
	}
	gpio_free(MSM_GPIO_USB3V3);
	return 0;
}	

 extern void  __init msm_init_pmic_vibrator(void); 

static ssize_t debug_global_read(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
  ssize_t size;

  size = sizeof(smem_global);

	if (pos >= size)
		return 0;

	count = min(len, (size_t)(size - pos));
	if (copy_to_user(buf, (char *)global + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static struct file_operations debug_global_file_ops = {
	.owner = THIS_MODULE,
	.read = debug_global_read,
};

static void __init msm7x2x_init(void)
{
  struct proc_dir_entry *entry;
  
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	zte_ftm_set_value(g_zte_ftm_flag_fixup);
#endif

	if (socinfo_init() < 0)
		BUG();

#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
			&msm_device_uart3.dev, 1);
#endif
	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
		smc91x_resources[0].start = 0x98000300;
		smc91x_resources[0].end = 0x980003ff;
		smc91x_resources[1].start = MSM_GPIO_TO_INT(85);
		smc91x_resources[1].end = MSM_GPIO_TO_INT(85);
		if (gpio_tlmm_config(GPIO_CFG(85, 0,
					      GPIO_INPUT,
					      GPIO_PULL_DOWN,
					      GPIO_2MA),
				     GPIO_ENABLE)) {
			printk(KERN_ERR
			       "%s: Err: Config GPIO-85 INT\n",
				__func__);
		}
	}

	if (cpu_is_msm7x27())
		msm7x2x_clock_data.max_axi_khz = 200000;

	msm_acpu_clock_init(&msm7x2x_clock_data);
	init_usb3v3();//USB-HML-001 enable ldo.

#ifdef CONFIG_ARCH_MSM7X27

	kgsl_pdata.max_axi_freq = 160000;
	

	kgsl_pdata.max_grp2d_freq = 0;
	kgsl_pdata.min_grp2d_freq = 0;
	kgsl_pdata.set_grp2d_async = NULL;
	kgsl_pdata.max_grp3d_freq = 0;
	kgsl_pdata.min_grp3d_freq = 0;
	kgsl_pdata.set_grp3d_async = NULL;
#endif

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	if(g_zte_ftm_flag_fixup) {
		msm_hsusb_pdata.zte_pid = 0x0112;
		msm_hsusb_pdata.vendor_id = 0x19D2;
		msm_hsusb_pdata.serial_number = 0;
		msm_hsusb_pdata.product_name = "ZTE HSUSB Device";
		msm_hsusb_pdata.manufacturer_name = "ZTE Incorporated";
	} else {
		msm_hsusb_pdata.zte_pid = 0xD00D;
	}
#endif

	msm_hsusb_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
#if defined( CONFIG_TOUCHSCREEN_MSM_LEGACY) || defined( CONFIG_TOUCHSCREEN_MSM)
	msm_device_tssc.dev.platform_data = &msm_tssc_pdata;
#endif
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;

	msm_gadget_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
	msm_device_hsusb_host.dev.platform_data = &msm_hsusb_pdata;

	msm_init_pmic_vibrator(); 
	
	platform_add_devices(devices, ARRAY_SIZE(devices));
#ifdef CONFIG_MSM_CAMERA
	config_camera_off_gpios(); 
#endif
	msm_device_i2c_init();
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	i2c_register_board_info(1, aux_i2c_devices, ARRAY_SIZE(aux_i2c_devices));
	i2c_register_board_info(2, aux2_i2c_devices, ARRAY_SIZE(aux2_i2c_devices));
#ifdef CONFIG_SURF_FFA_GPIO_KEYPAD
	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa())
		platform_device_register(&keypad_device_7k_ffa);
	else
		platform_device_register(&keypad_device_surf);
#endif
#ifdef CONFIG_ZTE_PLATFORM
	lcdc_himax_gpio_init();
#else
	lcdc_gordon_gpio_init();
#endif
	msm_fb_add_devices();
	rmt_storage_add_ramfs();
	msm7x2x_init_mmc();
	bt_power_init();
#ifdef CONFIG_ZTE_R750
modemctl_init_uartdm2_gpio();
#endif


	if (cpu_is_msm7x27())
		msm_pm_set_platform_data(msm7x27_pm_data);
	else
		msm_pm_set_platform_data(msm7x25_pm_data);

  global = ioremap(SMEM_LOG_GLOBAL_BASE, sizeof(smem_global));
  if (!global) {
    printk(KERN_ERR "ioremap failed with SCL_SMEM_LOG_RAM_BASE\n");
    return;
  }
  entry = create_proc_entry("smem_global", S_IFREG | S_IRUGO, NULL);
  if (!entry) {
		printk(KERN_ERR "smem_global: failed to create proc entry\n");
		return;
	}
  entry->proc_fops = &debug_global_file_ops;
	entry->size = sizeof(smem_global);
}

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static void __init pmem_mdp_size_setup(char **p)
{
	pmem_mdp_size = memparse(*p, p);
}
__early_param("pmem_mdp_size=", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

#ifdef CONFIG_ARCH_MSM7X27
static unsigned gpu_phys_size = MSM_GPU_PHYS_SIZE;
static void __init gpu_phys_size_setup(char **p)
{
	gpu_phys_size = memparse(*p, p);
}
__early_param("gpu_phys_size=", gpu_phys_size_setup);
#endif

static void __init msm_msm7x2x_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;
#if 0
	unsigned int *pos = MSM_RAM_LOG_BASE + 4*ZTE_MEM_F3LOG;
	unsigned int len = *pos;
#endif

	size = pmem_mdp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for mdp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}
#ifdef CONFIG_ARCH_MSM7X27
	size = gpu_phys_size ? : MSM_GPU_PHYS_SIZE;
	addr = alloc_bootmem(size);
	kgsl_resources[1].start = __pa(addr);
	kgsl_resources[1].end = kgsl_resources[1].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for KGSL\n",
		size, addr, __pa(addr));
#endif

#if 0
	size = A11TRACE_LENGTH;
	reserve_bootmem(0x08900000, A11TRACE_LENGTH, BOOTMEM_DEFAULT);		
	addr = phys_to_virt(0x08900000);																	
	pr_info("allocating %lu bytes at %p (%lx physical) for kenrel ram log\n",size, addr, __pa(addr));


	if (len > 12)
		len = 12;
	else
		len = len/2*2;
	pr_info("length = %d\n", len);
	size = len;
	if (size)
		reserve_bootmem(0x08D00000, size*0x100000, BOOTMEM_DEFAULT);		

	addr = phys_to_virt(0x08D00000);																	
	pr_info("allocating %lu M at %p (%lx physical) for F3\n",size, addr, __pa(addr));
#endif
}

static void __init msm7x2x_map_io(void)
{
	msm_map_common_io();

#ifndef CONFIG_ZTE_PLATFORM
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa())
		msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);

	if (machine_is_msm7x25_surf() || machine_is_msm7x25_ffa())
		msm_clock_init(msm_clocks_7x25, msm_num_clocks_7x25);
#else
		msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
#endif
	msm_msm7x2x_allocate_memory_regions();

#ifdef CONFIG_CACHE_L2X0
#ifndef CONFIG_ZTE_PLATFORM
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa()) {
#endif
	
		l2x0_init(MSM_L2CC_BASE, 0x00068012, 0xfe000000);
#ifndef CONFIG_ZTE_PLATFORM
	}
#endif
#endif
}

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
#define ATAG_ZTEFTM 0x5d53cd73
static int  parse_tag_zteftm(const struct tag *tags)
{
	int flag = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_ZTEFTM) {
			printk(KERN_DEBUG "find the zte ftm tag\n");
			find = 1;
			break;
		}
	}

	if (find)
		flag = t->u.revision.rev;
	printk(KERN_INFO "[ZYF@FTM]parse_tag_zteftm: zte FTM %s !\n", flag?"enable":"disable");
	return flag;
}
#endif

#ifdef CONFIG_ZTE_PLATFORM
static void __init zte_fixup(struct machine_desc *desc, struct tag *tags,
				  char **cmdline, struct meminfo *mi)
{
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	g_zte_ftm_flag_fixup = parse_tag_zteftm((const struct tag *)tags);
#endif

}
#endif

MACHINE_START(MSM7X27_SURF, "QCT MSM7x27 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X27_FFA, "QCT MSM7x27 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X25_SURF, "QCT MSM7x25 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X25_FFA, "QCT MSM7x25 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(JOE, "joe ZTE handset")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
#ifdef CONFIG_ZTE_PLATFORM
	.fixup          = zte_fixup, 
#endif
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

