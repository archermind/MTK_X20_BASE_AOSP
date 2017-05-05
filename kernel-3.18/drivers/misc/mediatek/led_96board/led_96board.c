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


#include "led_96board.h"
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>



#define  led_0     196|0x80000000
#define  led_1     197|0x80000000
#define  led_2     198|0x80000000
#define  led_3     199|0x80000000
#define  led_4     200|0x80000000
#define  led_5     201|0x80000000




struct led_96board_context *led_96board_context_obj = NULL;

static void gpio_set(unsigned long pin,unsigned long mode,unsigned long dir)
{
	mt_set_gpio_mode(pin,mode);
    mt_set_gpio_dir(pin,dir);
}

static void init_leds(void)
{
	gpio_set(led_0,GPIO_MODE_00,GPIO_DIR_OUT);
	gpio_set(led_1,GPIO_MODE_00,GPIO_DIR_OUT);
	gpio_set(led_2,GPIO_MODE_00,GPIO_DIR_OUT);
	gpio_set(led_3,GPIO_MODE_00,GPIO_DIR_OUT);
	gpio_set(led_4,GPIO_MODE_00,GPIO_DIR_OUT);
	gpio_set(led_5,GPIO_MODE_00,GPIO_DIR_OUT);

	mt_set_gpio_out(led_0,GPIO_OUT_ZERO);
	mt_set_gpio_out(led_1,GPIO_OUT_ZERO);
	mt_set_gpio_out(led_2,GPIO_OUT_ZERO);
	mt_set_gpio_out(led_3,GPIO_OUT_ZERO);
	mt_set_gpio_out(led_4,GPIO_OUT_ZERO);
	mt_set_gpio_out(led_5,GPIO_OUT_ZERO);

	led_96board_context_obj->led0 = 0;
	led_96board_context_obj->led1 = 0;
	led_96board_context_obj->led2 = 0;
	led_96board_context_obj->led3 = 0;
	led_96board_context_obj->led4 = 0;
	led_96board_context_obj->led5 = 0;
}

static ssize_t led_96board_store_led_0(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{   
    
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
    if (!strncmp(buf, "on", 2)) 
	{
      	//led0 on
		led_96board_context_obj->led0 = 1;
		mt_set_gpio_out(led_0,GPIO_OUT_ONE);
	
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led0 off
	   led_96board_context_obj->led0 = 0;
	   mt_set_gpio_out(led_0,GPIO_OUT_ZERO);

    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);

    return count;
}

static ssize_t led_96board_show_led_0(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led0); 
}

static ssize_t led_96board_store_led_1(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	 if (!strncmp(buf, "on", 2)) 
	{
      	//led1 on
		led_96board_context_obj->led1 = 1;
		mt_set_gpio_out(led_1,GPIO_OUT_ONE);

    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led1 off
	   led_96board_context_obj->led1 = 0;
	   mt_set_gpio_out(led_1,GPIO_OUT_ZERO);
	  
    }
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
	
    return count;
}

static ssize_t led_96board_show_led_1(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led1); 
}

static ssize_t led_96board_store_led_2(struct device* dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	 if (!strncmp(buf, "on", 2)) 
	{
      	//led2 on
		led_96board_context_obj->led2 = 1;
		mt_set_gpio_out(led_2,GPIO_OUT_ONE);
	
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led2 off
	   led_96board_context_obj->led2 = 0;
	   mt_set_gpio_out(led_2,GPIO_OUT_ZERO);
	  
    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
    return count;
}

static ssize_t led_96board_show_led_2(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led2); 
}


static ssize_t led_96board_store_led_3(struct device* dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	  if (!strncmp(buf, "on", 2)) 
	{
      	//led3 on
		led_96board_context_obj->led3 = 1;
		mt_set_gpio_out(led_3,GPIO_OUT_ONE);
		
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led3 off
	   led_96board_context_obj->led3 = 0;
	   mt_set_gpio_out(led_3,GPIO_OUT_ZERO);
	   
    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
    return count;
}

static ssize_t led_96board_show_led_3(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led3); 
}

static ssize_t led_96board_store_led_4(struct device* dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	  if (!strncmp(buf, "on", 2)) 
	{
      	//led3 on
		led_96board_context_obj->led4 = 1;
		mt_set_gpio_out(led_4,GPIO_OUT_ONE);
		
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led3 off
	   led_96board_context_obj->led4 = 0;
	   mt_set_gpio_out(led_4,GPIO_OUT_ZERO);
	  
    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
    return count;
}

static ssize_t led_96board_show_led_4(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led4); 
}

static ssize_t led_96board_store_led_5(struct device* dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	  if (!strncmp(buf, "on", 2)) 
	{
      	//led3 on
		led_96board_context_obj->led5 = 1;
		mt_set_gpio_out(led_5,GPIO_OUT_ONE);
		
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led3 off
	   led_96board_context_obj->led5 = 0;
	   mt_set_gpio_out(led_5,GPIO_OUT_ZERO);
	   
    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
    return count;
}

static ssize_t led_96board_show_led_5(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->led5); 
}

static ssize_t led_96board_store_led_all(struct device* dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	mutex_lock(&led_96board_context_obj->led_96board_op_mutex);
	
	  if (!strncmp(buf, "on", 2)) 
	{
      	//led all on
		led_96board_context_obj->leds = 1;
		mt_set_gpio_out(led_0,GPIO_OUT_ONE);
		mt_set_gpio_out(led_1,GPIO_OUT_ONE);
		mt_set_gpio_out(led_2,GPIO_OUT_ONE);
		mt_set_gpio_out(led_3,GPIO_OUT_ONE);
		mt_set_gpio_out(led_4,GPIO_OUT_ONE);
		mt_set_gpio_out(led_5,GPIO_OUT_ONE);
    } 
	else if(!strncmp(buf, "off", 3)) 
	{
        //led  all off
	   led_96board_context_obj->leds = 0;
	   mt_set_gpio_out(led_0,GPIO_OUT_ZERO);
	   mt_set_gpio_out(led_1,GPIO_OUT_ZERO);
	   mt_set_gpio_out(led_2,GPIO_OUT_ZERO);
	   mt_set_gpio_out(led_3,GPIO_OUT_ZERO);
	   mt_set_gpio_out(led_4,GPIO_OUT_ZERO);
	   mt_set_gpio_out(led_5,GPIO_OUT_ZERO);
	   
    }
	
	mutex_unlock(&led_96board_context_obj->led_96board_op_mutex);
    return count;
}

static ssize_t led_96board_show_led_all(struct device* dev, 
                                 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", led_96board_context_obj->leds); 
}

static int led_96board_misc_init(struct led_96board_context *cxt)
{

    int err=0;
    cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = "96board_leds";
	if((err = misc_register(&cxt->mdev)))
	{
       printk(KERN_ERR"96board_leds misc device register failure!\n"); 
	}
	return err;
}


DEVICE_ATTR(96_led0,     		S_IWUSR | S_IRUGO, led_96board_show_led_0,  led_96board_store_led_0);
DEVICE_ATTR(96_led1,      		S_IWUSR | S_IRUGO, led_96board_show_led_1,  led_96board_store_led_1);
DEVICE_ATTR(96_led2,      		S_IWUSR | S_IRUGO, led_96board_show_led_2,  led_96board_store_led_2);
DEVICE_ATTR(96_led3,      		S_IWUSR | S_IRUGO, led_96board_show_led_3,  led_96board_store_led_3);
DEVICE_ATTR(96_led4,      		S_IWUSR | S_IRUGO, led_96board_show_led_4,  led_96board_store_led_4);
DEVICE_ATTR(96_led5,      		S_IWUSR | S_IRUGO, led_96board_show_led_5,  led_96board_store_led_5);
DEVICE_ATTR(96_leds,      		S_IWUSR | S_IRUGO, led_96board_show_led_all,  led_96board_store_led_all);

static struct attribute *led_96board_attributes[] = {
	&dev_attr_96_led0.attr,
	&dev_attr_96_led1.attr,
	&dev_attr_96_led2.attr,
	&dev_attr_96_led3.attr,
	&dev_attr_96_led4.attr,
	&dev_attr_96_led5.attr,
	&dev_attr_96_leds.attr,
	NULL
};

static struct attribute_group led_96board_attribute_group = {
	.attrs = led_96board_attributes
};


static int led_96board_probe(struct platform_device *pdev) 
{
	int err = 0;
	struct led_96board_context *obj = NULL;
	
	obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
    	
	if(!obj)
	{
		printk(KERN_ERR"96board_leds device kzalloc failure!\n");
		goto exit_alloc_data_failed;
	}	
	mutex_init(&obj->led_96board_op_mutex);
	
	
	led_96board_context_obj = obj;
	
	err = led_96board_misc_init(led_96board_context_obj);
	if(err)
	{
	   printk(KERN_ERR"96board_leds device led_96board_misc_init failure!\n");
	   return -2;
	}
	err = sysfs_create_group(&led_96board_context_obj->mdev.this_device->kobj,
		    &led_96board_attribute_group);
	if (err < 0)
	{
	   printk(KERN_ERR"96board_leds device sysfs_create_group failure!\n");
	   return -3;
	}
	kobject_uevent(&led_96board_context_obj->mdev.this_device->kobj, KOBJ_ADD);
	
    init_leds();
	
	return 0;

	exit_alloc_data_failed:
	
	return err;
}


static int led_96board_remove(struct platform_device *pdev)
{
    int err=0;
    
	sysfs_remove_group(&led_96board_context_obj->mdev.this_device->kobj,
				&led_96board_attribute_group);
	
	if((err = misc_deregister(&led_96board_context_obj->mdev)))
	{
		 printk(KERN_ERR"96board_leds device misc_deregister failure!\n");
	}
	kfree(led_96board_context_obj);

	return 0;
}

static int led_96board_suspend(struct platform_device *dev, pm_message_t state) 
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static int led_96board_resume(struct platform_device *dev)
{
	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id m_led_96board_pl_of_match[] = {
	{ .compatible = "mediatek,m_led_96board_pl", },
	{},
};
#endif

static struct platform_driver led_96board_driver =
{
	.probe      = led_96board_probe,
	.remove     = led_96board_remove,    
	.suspend    = led_96board_suspend,
	.resume     = led_96board_resume,
	.driver     = 
	{
		.name = "led_96board",
		#ifdef CONFIG_OF
		.of_match_table = m_led_96board_pl_of_match,
		#endif
	}
};

static int __init led_96board_init(void) 
{
	if(platform_driver_register(&led_96board_driver))
	{
		 printk(KERN_ERR"96board_leds device platform_driver_register failure!\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit led_96board_exit(void)
{
	platform_driver_unregister(&led_96board_driver);  
}

module_init(led_96board_init);
module_exit(led_96board_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("96board led device driver");
MODULE_AUTHOR("Archermind,chaobing.dang@archermind.com");

