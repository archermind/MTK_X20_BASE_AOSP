/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "mt8193-iic: " fmt
#define DEBUG 1

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>

#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <linux/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>

#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <linux/io.h>

#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <linux/uaccess.h>

#include "mt8193_iic.h"
#include "mt8193.h"
#define HDMI_OPEN_PACAKAGE_SUPPORT
#ifdef HDMI_OPEN_PACAKAGE_SUPPORT
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <mt-plat/mt_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#endif
/*----------------------------------------------------------------------------*/
/* mt8193 device information                                                  */
/*----------------------------------------------------------------------------*/
#define MAX_TRANSACTION_LENGTH 8
#define MT8193_DEVICE_NAME            "mtk-multibridge"
#define MT8193_I2C_SLAVE_ADDR       0x3A
#define MT8193_I2C_DEVICE_ADDR_LEN   2
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static int mt8193_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mt8193_i2c_remove(struct i2c_client *client);
static struct i2c_client *mt8193_i2c_client;
static const struct i2c_device_id mt8193_i2c_id[] = {{MT8193_DEVICE_NAME, 0}, {} };
#ifdef HDMI_OPEN_PACAKAGE_SUPPORT
struct regulator *vldo28_pmu =NULL;
struct regulator *vrf12_pmu =NULL;
struct regulator *vibr_pmu =NULL;
struct regulator *vmipi_pmu =NULL;
struct device *mt8193_dev_context =NULL;
static const struct of_device_id mt8193_i2c_mhl_id[] = {
		{.compatible = "mediatek,ext_disp"},
		{},
	};
char *rst_pin_name[2] = {"rst_default", "rst_high"};

struct i2c_driver mt8193_i2c_driver = {
	.probe		= mt8193_i2c_probe,
	.remove		= mt8193_i2c_remove,
	.driver		= { .name = MT8193_DEVICE_NAME,
				.of_match_table = mt8193_i2c_mhl_id,
	},
	.id_table	= mt8193_i2c_id,
};
#else
static struct i2c_board_info i2c_mt8193 __initdata = {I2C_BOARD_INFO(MT8193_DEVICE_NAME, (MT8193_I2C_SLAVE_ADDR>>1))};
/*----------------------------------------------------------------------------*/
struct i2c_driver mt8193_i2c_driver = {
	.probe		= mt8193_i2c_probe,
	.remove		= mt8193_i2c_remove,
	.driver		= { .name = MT8193_DEVICE_NAME, },
	.id_table	= mt8193_i2c_id,
};
#endif

struct mt8193_i2c_data {
	struct i2c_client *client;
	uint16_t addr;
	int use_reset;		/*use RESET flag*/
	int use_irq;		/*use EINT flag*/
	int retry;
};

static struct mt8193_i2c_data *obj_i2c_data;

/*----------------------------------------------------------------------------*/
int mt8193_i2c_read(u16 addr, u32 *data)
{
#if 0
	u8 rxBuf[8] = {0};
	int ret = 0;
	struct i2c_client *client = mt8193_i2c_client;
	u8 lens;

	if (((addr >> 8) & 0xFF) >= 0x80) {
		/* 8 bit : fast mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		lens = 1;
	} else {
		/* 16 bit : noraml mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		rxBuf[1] = addr & 0xFF;
		lens = 2;
	}

	/*client->addr = (client->addr & I2C_MASK_FLAG);
	client->ext_flag = I2C_WR_FLAG;*/
	client->addr = client->addr;

	ret = i2c_master_send(client, (const char *)&rxBuf, (4 << 8) | lens);
	if (ret < 0) {
		pr_err("%s: read error\n", __func__);
		return -EFAULT;
	}
#else
	struct i2c_client *client = mt8193_i2c_client;
	struct i2c_msg msg[2];
	u8 rxBuf[8] = {0};
	u8 lens = 0;

	if (((addr >> 8) & 0xFF) >= 0x80) {
		/* 8 bit : fast mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		lens = 1;
	} else {
		/* 16 bit : noraml mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		rxBuf[1] = addr & 0xFF;
		lens = 2;
	}

	msg[0].flags = 0;
	msg[0].addr = client->addr;
	msg[0].buf = rxBuf;
	msg[0].len = lens;

	msg[1].flags = I2C_M_RD;
	msg[1].addr = client->addr;
	msg[1].buf = rxBuf;
	msg[1].len = 4;


	i2c_transfer(client->adapter, msg, 2);
#endif
	*data = (rxBuf[3] << 24) | (rxBuf[2] << 16) | (rxBuf[1] << 8) | (rxBuf[0]); /*LSB fisrt*/
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(mt8193_i2c_read);
/*----------------------------------------------------------------------------*/

int mt8193_i2c_write(u16 addr, u32 data)
{
	struct i2c_client *client = mt8193_i2c_client;
	u8 buffer[8];
	int ret = 0;
	struct i2c_msg msg = {
		/*.addr	= client->addr & I2C_MASK_FLAG,*/
		.addr	= client->addr,
		.flags	= 0,
		.len	= (((addr >> 8) & 0xFF) >= 0x80)?5:6,
		.buf	= buffer,
	};

	if (((addr >> 8) & 0xFF) >= 0x80) {
		/* 8 bit : fast mode */
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = (data >> 24) & 0xFF;
		buffer[2] = (data >> 16) & 0xFF;
		buffer[3] = (data >> 8) & 0xFF;
		buffer[4] = data & 0xFF;
	} else {
		/* 16 bit : noraml mode */
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = addr & 0xFF;
		buffer[2] = (data >> 24) & 0xFF;
		buffer[3] = (data >> 16) & 0xFF;
		buffer[4] = (data >> 8) & 0xFF;
		buffer[5] = data & 0xFF;
	}

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		pr_err("%s: send command error\n", __func__);
		return -EFAULT;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(mt8193_i2c_write);

/*----------------------------------------------------------------------------*/
/* IIC Probe                                                                  */
/*----------------------------------------------------------------------------*/
static int mt8193_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	struct mt8193_i2c_data *obj;

	pr_err("%s\n", __func__);

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Allocate ts memory fail\n", __func__);
		return ret;
	}
	obj_i2c_data = obj;
	obj->client = client;
	mt8193_i2c_client = obj->client;
	i2c_set_clientdata(client, obj);

	return 0;
}
/*----------------------------------------------------------------------------*/

static int mt8193_i2c_remove(struct i2c_client *client)
{
	pr_err("%s\n", __func__);
	mt8193_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef HDMI_OPEN_PACAKAGE_SUPPORT
static int mt8193_power_init(struct platform_device *pdev,bool switch_power)
{
	int ret =0;
	struct device_node *kd_node =NULL;
	struct pinctrl *rst_pinctrl;
	struct pinctrl_state *pin_state;
	mt8193_dev_context = (struct device *)&(pdev->dev);
	kd_node = mt8193_dev_context->of_node;
	/* get regulator supply node */
	mt8193_dev_context->of_node = of_find_compatible_node(NULL,NULL,"mediatek,mt_pmic_regulator_supply"); 	

	pr_err("mt8193_power_init get mt_pmic_regulator_supply!\n");
	if(switch_power == 1)
	{
		if (vmipi_pmu == NULL) {
		    vmipi_pmu = regulator_get(mt8193_dev_context, "vmipi");	
	    }
	    if (IS_ERR(vmipi_pmu)){
		pr_err("mt8193_power_init vmipi_pmu error %p!!!!!!!!!!!!!!\n", vmipi_pmu);
		ret = -1;
		goto exit;
	} else {
		pr_debug("mt8193_power_init vmipi_pmu init done %p\n", vmipi_pmu );
		regulator_set_voltage(vmipi_pmu, 1800000, 1800000);
		ret = regulator_enable(vmipi_pmu);
		if(ret)
			pr_err("regulator_enable vmipi failed!\n");
		else
			pr_err("regulator_enable vmipi pass!\n");
	}
	msleep(4);

	if (vrf12_pmu == NULL) {
		vrf12_pmu = regulator_get(mt8193_dev_context, "vrf12");	
	}
	if (IS_ERR(vrf12_pmu)) {
		pr_err("mt8193_power_init vrf12_pmu error %p!!!!!!!!!!!!!!\n", vrf12_pmu);
		ret = -1;
		goto vrf12_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vrf12_pmu init done %p\n", vrf12_pmu );
		regulator_set_voltage(vrf12_pmu, 1200000, 1200000);
		ret = regulator_enable(vrf12_pmu);
		if(ret)
			pr_err("regulator_enable vrf12_pmu failed!\n");
		else
			pr_err("regulator_enable vrf12_pmu pass!\n");
	}
	msleep(8);

	if (vibr_pmu == NULL) {
		vibr_pmu = regulator_get(mt8193_dev_context, "vibr");	
	}
	if (IS_ERR(vibr_pmu)){
		pr_err("mt8193_power_init vibr_pmu error %p!!!!!!!!!!!!!!\n", vibr_pmu);
		ret = -1;
		goto vibr_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vibr_pmu init done %p\n", vibr_pmu );
		regulator_set_voltage(vibr_pmu, 3300000, 3300000);
		ret = regulator_enable(vibr_pmu);
		if(ret)
			pr_err("regulator_enable vibr_pmu failed!\n");
		else
			pr_err("regulator_enable vibr_pmu pass!\n");
	}
	msleep(4);

	if (vldo28_pmu == NULL) {
		vldo28_pmu = regulator_get(mt8193_dev_context, "vldo28");	
	}
	if (IS_ERR(vldo28_pmu)) {
		pr_err("mt8193_power_init vldo28_pmu error %p!!!!!!!!!!!!!!\n", vldo28_pmu);
		ret = -1;
		goto vldo28_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vldo28_pmu init done %p\n", vldo28_pmu );
		regulator_set_voltage(vldo28_pmu, 2800000, 2800000);
		ret = regulator_enable(vldo28_pmu);
		if(ret)
			pr_err("regulator_enable vldo28_pmu failed!\n");
		else
			pr_err("regulator_enable vldo28_pmu pass!\n");
	}
	msleep(20);
	
	mt8193_dev_context->of_node = kd_node ;
	rst_pinctrl = devm_pinctrl_get(mt8193_dev_context);
	if(IS_ERR(rst_pinctrl)) {
		ret = PTR_ERR(rst_pinctrl);
		pr_err("Cannot find mt8193 rst pinctrl!\n");
		goto rst_exit;
	}
	pin_state = pinctrl_lookup_state(rst_pinctrl, rst_pin_name[1]);
	if(IS_ERR(pin_state)) {
		ret = PTR_ERR(pin_state);
		pr_err("Cannot find mt8193 rst pin state!\n");
		goto rst_exit;
	} else {
		pinctrl_select_state(rst_pinctrl, pin_state);
	}
#if 0
	dn = of_find_compatible_node(NULL, NULL, "mediatek,mt8193-hdmi");
	bus_reset_pin = of_get_named_gpio(dn, "hdmi_reset_gpios", 0);
	gpio_direction_output(bus_reset_pin, 1);
#endif
	}
	else if (switch_power == 0)
	{
		if (vmipi_pmu == NULL) {
		    vmipi_pmu = regulator_get(mt8193_dev_context, "vmipi");	
	    }
	    if (IS_ERR(vmipi_pmu)){
		pr_err("mt8193_power_init vmipi_pmu error %p!!!!!!!!!!!!!!\n", vmipi_pmu);
		ret = -1;
		goto exit;
	} else {
		pr_debug("mt8193_power_init vmipi_pmu init done %p\n", vmipi_pmu );
		//regulator_set_voltage(vmipi_pmu, 1800000, 1800000);
		ret = regulator_disable(vmipi_pmu);
		if(ret)
			pr_err("regulator_disable vmipi failed!\n");
		else
			pr_err("regulator_disable vmipi pass!\n");
	}
	//msleep(4);

	if (vrf12_pmu == NULL) {
		vrf12_pmu = regulator_get(mt8193_dev_context, "vrf12");	
	}
	if (IS_ERR(vrf12_pmu)) {
		pr_err("mt8193_power_init vrf12_pmu error %p!!!!!!!!!!!!!!\n", vrf12_pmu);
		ret = -1;
		goto vrf12_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vrf12_pmu init done %p\n", vrf12_pmu );
		//regulator_set_voltage(vrf12_pmu, 1200000, 1200000);
		ret = regulator_disable(vrf12_pmu);
		if(ret)
			pr_err("regulator_disable vrf12_pmu failed!\n");
		else
			pr_err("regulator_disable vrf12_pmu pass!\n");
	}
	//msleep(8);

	if (vibr_pmu == NULL) {
		vibr_pmu = regulator_get(mt8193_dev_context, "vibr");	
	}
	if (IS_ERR(vibr_pmu)){
		pr_err("mt8193_power_init vibr_pmu error %p!!!!!!!!!!!!!!\n", vibr_pmu);
		ret = -1;
		goto vibr_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vibr_pmu init done %p\n", vibr_pmu );
		//regulator_set_voltage(vibr_pmu, 3300000, 3300000);
		ret = regulator_disable(vibr_pmu);
		if(ret)
			pr_err("regulator_disable vibr_pmu failed!\n");
		else
			pr_err("regulator_disable vibr_pmu pass!\n");
	}
	//msleep(4);

	if (vldo28_pmu == NULL) {
		vldo28_pmu = regulator_get(mt8193_dev_context, "vldo28");	
	}
	if (IS_ERR(vldo28_pmu)) {
		pr_err("mt8193_power_init vldo28_pmu error %p!!!!!!!!!!!!!!\n", vldo28_pmu);
		ret = -1;
		goto vldo28_pmu_exit;
	} else {
		pr_debug("mt8193_power_init vldo28_pmu init done %p\n", vldo28_pmu );
		//regulator_set_voltage(vldo28_pmu, 2800000, 2800000);
		ret = regulator_disable(vldo28_pmu);
		if(ret)
			pr_err("regulator_disable vldo28_pmu failed!\n");
		else
			pr_err("regulator_disable vldo28_pmu pass!\n");
	}
	//msleep(20);
	
	mt8193_dev_context->of_node = kd_node ;
	rst_pinctrl = devm_pinctrl_get(mt8193_dev_context);
	if(IS_ERR(rst_pinctrl)) {
		ret = PTR_ERR(rst_pinctrl);
		pr_err("Cannot find mt8193 rst pinctrl!\n");
		goto rst_exit;
	}
	pin_state = pinctrl_lookup_state(rst_pinctrl, rst_pin_name[0]);
	if(IS_ERR(pin_state)) {
		ret = PTR_ERR(pin_state);
		pr_err("Cannot find mt8193 rst pin state!\n");
		goto rst_exit;
	} else {
		pinctrl_select_state(rst_pinctrl, pin_state);
	}
#if 0
	dn = of_find_compatible_node(NULL, NULL, "mediatek,mt8193-hdmi");
	bus_reset_pin = of_get_named_gpio(dn, "hdmi_reset_gpios", 0);
	gpio_direction_output(bus_reset_pin, 1);
#endif
	}

	return ret;
rst_exit:
	regulator_disable(vldo28_pmu);
vldo28_pmu_exit:
	regulator_disable(vibr_pmu);
vibr_pmu_exit:
	regulator_disable(vrf12_pmu);
vrf12_pmu_exit:
	regulator_disable(vmipi_pmu);
exit:
	mt8193_dev_context->of_node = kd_node ;
	return ret;
}
#endif
/*----------------------------------------------------------------------------*/
/* device driver probe                                                        */
/*----------------------------------------------------------------------------*/
static int mt8193_probe(struct platform_device *pdev)
{
	pr_err("%s\n", __func__);
#ifdef HDMI_OPEN_PACAKAGE_SUPPORT
	mt8193_power_init(pdev,1);
#else
	if (i2c_add_driver(&mt8193_i2c_driver)) {
		pr_err("%s: unable to add mt8193 i2c driver\n", __func__);
		return -1;
	}
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mt8193_remove(struct platform_device *pdev)
{
	pr_err("%s\n", __func__);
	i2c_del_driver(&mt8193_i2c_driver);
	return 0;
}
static int mt8193_power_suspend(struct platform_device *pdev,pm_message_t state)
{
	//mt8193_power_init(pdev,0);
	return 0;
}
static int mt8193_power_resume(struct platform_device *pdev)
{
	//mt8193_power_init(pdev,1);
	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id multibridge_of_ids[] = {
	{.compatible = "mediatek,multibridge",},
	{}
};
#endif

static struct platform_driver mt8193_mb_driver = {
	.probe		= mt8193_probe,
	.remove		= mt8193_remove,
	.suspend    = mt8193_power_suspend,
	.resume     = mt8193_power_resume,
	.driver		= {
		.name	= "multibridge",
#ifdef CONFIG_OF
		.of_match_table = multibridge_of_ids,
#endif
	}
};

/*----------------------------------------------------------------------------*/
static int __init mt8193_mb_init(void)
{
	int ret = 0;
	pr_err("%s\n", __func__);
#ifdef HDMI_OPEN_PACAKAGE_SUPPORT
	if (i2c_add_driver(&mt8193_i2c_driver)) {
		pr_err("%s: unable to add mt8193 i2c driver\n", __func__);
		return -1;
	}
#endif
	/*i2c_register_board_info(1, &i2c_mt8193, 1);*/
	ret = platform_driver_register(&mt8193_mb_driver);
	if (ret) {
		pr_err("failed to register mt8193_mb_driver, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit mt8193_mb_exit(void)
{
	platform_driver_unregister(&mt8193_mb_driver);
}
/*----------------------------------------------------------------------------*/
#ifndef HDMI_OPEN_PACAKAGE_SUPPORT
static int __init mt8193_i2c_board_init(void)
{
	int ret = 0;

	pr_err("%s\n", __func__);
	ret = i2c_register_board_info(MT8193_I2C_ID, &i2c_mt8193, 1);
	if (ret)
		pr_err("failed to register mt8193 i2c_board_info, ret=%d\n", ret);

	return ret;
}
/*----------------------------------------------------------------------------*/
core_initcall(mt8193_i2c_board_init);
#endif
module_init(mt8193_mb_init);
module_exit(mt8193_mb_exit);
MODULE_AUTHOR("SS, Wu <ss.wu@mediatek.com>");
MODULE_DESCRIPTION("MT8193 Driver");
MODULE_LICENSE("GPL");
