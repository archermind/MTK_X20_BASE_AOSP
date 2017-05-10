#include "cm36558.h"
#include "sensor_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <hal_i2c.h>
#include <FreeRTOSConfig.h>
#include <platform.h>
#include <interrupt.h>
#include <semphr.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hwsen.h"
#include "eint.h"
/*----------------------------------------------------------------------------*/
#define CM36558_DEV_NAME     "cm36558"
/*#define CM36558_LOG_MSG*/
#define APS_TAG                "[ALS/PS] "
#define APS_ERR(fmt, arg...)   PRINTF_D(APS_TAG"%d: "fmt, __LINE__, ##arg)
#define APS_DBG(fmt, arg...)   PRINTF_D(APS_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef  CM36558_LOG_MSG
#define APS_LOG(fmt, arg...)   PRINTF_D(APS_TAG fmt, ##arg)
#define APS_FUN(f)             PRINTF_D("%s\n\r", __FUNCTION__)
#else
#define APS_LOG(fmt, arg...)
#define APS_FUN(f)
#endif
//static struct cm36558_priv *g_cm36558_ptr = NULL;
static struct cm36558_priv cm36558_obj;
//static thread_t *alsps_thread = NULL;
const unsigned int eint_mapping_table[] = {2, 4, 9, 10, 11, 18};

static int check_timeout(portTickType *end_tick)
{
    portTickType cur_time;

    cur_time = xTaskGetTickCount();

    if (cur_time > *end_tick) {
        return 1;
    }
    return 0;
}

static void cal_end_time(unsigned int ms, portTickType *end_tick)
{
    portTickType cur_tick, als_deb_tick;
    cur_tick = xTaskGetTickCount();
    als_deb_tick = (portTickType)(ms / (1000 / portTICK_RATE_MS));

    *end_tick = cur_tick + als_deb_tick;
}

/*----------------------------------------------------------------------------*/
static int cm36558_i2c_write(unsigned char *data, unsigned int len)
{

    int err = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write(&i2c, data, len);
    if (0 != err) {
        APS_ERR("i2c_write fail: %d\n\r", err);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int cm36558_i2c_read(unsigned char *data, unsigned int len)
{

    int err = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;
    i2c.st_rs = I2C_TRANS_REPEATED_START;

    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        APS_ERR("i2c_read fail: %d\n\r", err);
    }
    return err;
}

int cm36558_enable_eint()
{
    //SCP_EnableIRQ(EINT_IRQn);
    return 0;
}

static int cm36558_disable_eint()
{
    //SCP_DisableIRQ(EINT_IRQn);
    return 0;
}

int cm36558_enable_ps(int enable)
{
    struct cm36558_priv *obj = &cm36558_obj;
    int res;
    unsigned char databuf[3];

    if (enable == 1) {
        APS_LOG("enable_ps\n\r");

        cm36558_enable_eint();

        databuf[0] = CM36558_REG_PS_CONF1_2;
        res = cm36558_i2c_read(databuf, 0x2);
        if (res < 0) {
            APS_ERR("read err\n\r");
            goto ENABLE_PS_EXIT_ERR;
        }
        databuf[2] = databuf[1];
        databuf[1] = databuf[0] & 0xFE;
        databuf[0] = CM36558_REG_PS_CONF1_2;
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("write err\n\r");
            goto ENABLE_PS_EXIT_ERR;
        }

        obj->ps_deb_on = 1;
        cal_end_time(obj->ps_debounce, &obj->ps_deb_end);
    } else {
        APS_LOG(" disable_ps\n\r");

        databuf[0] = CM36558_REG_PS_CONF1_2;
        res = cm36558_i2c_read(databuf, 0x2);
        if (res < 0) {
            APS_ERR("i2c_master_send function err\n\r");
            goto ENABLE_PS_EXIT_ERR;
        }
        databuf[2] = databuf[1];
        databuf[1] = databuf[0] | 0x01;
        databuf[0] = CM36558_REG_PS_CONF1_2;
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("write err\n\r");
            goto ENABLE_PS_EXIT_ERR;
        }

        obj->ps_deb_on = 0;

        res = cm36558_disable_eint();
        if (res != 0) {
            APS_ERR("disable eint fail: %d\n\r", res);
            return res;
        }
    }

    return 0;
ENABLE_PS_EXIT_ERR:
    return res;
}
int cm36558_enable_als(int enable)
{
    struct cm36558_priv *obj = &cm36558_obj;
    unsigned char databuf[2] = {0};
    int res;

    if (enable == 1) {
        APS_LOG("CM36558_enable_als\n\r");
        databuf[0] = CM36558_REG_ALS_CONF;
        res = cm36558_i2c_read(databuf, 0x2);
        if (res < 0) {
            APS_ERR("Read CM36558_REG_ALS_CONF err\n\r");
            goto ENABLE_ALS_EXIT_ERR;
        }
        databuf[0] = CM36558_REG_ALS_CONF;
        databuf[1] = databuf[1] & 0xFE;
        res = cm36558_i2c_write(databuf, 2);
        if (res < 0) {
            APS_ERR("Write CM36558_REG_ALS_CONF err\n\r");
            goto ENABLE_ALS_EXIT_ERR;
        }
        obj->als_deb_on = 1;
        cal_end_time(obj->ps_debounce, &obj->ps_deb_end);
    } else {
        APS_LOG("CM36558_disable_als\n\r");
        databuf[0] = CM36558_REG_ALS_CONF;
        res = cm36558_i2c_read(databuf, 2);
        if (res < 0) {
            APS_ERR("Read CM36558_REG_ALS_CONF err\n\r");
            goto ENABLE_ALS_EXIT_ERR;
        }
        databuf[0] = CM36558_REG_ALS_CONF;
        databuf[1] = databuf[1] | 0x01;
        res = cm36558_i2c_write(databuf, 2);
        databuf[0] = CM36558_REG_ALS_CONF;
        res = cm36558_i2c_read(databuf, 2);
        if (res < 0) {
            APS_ERR("Read CM36558_REG_ALS_CONF err\n\r");
            goto ENABLE_ALS_EXIT_ERR;
        }
        if (res < 0) {
            APS_ERR("Write CM36558_REG_ALS_CONF err\n\r");
            goto ENABLE_ALS_EXIT_ERR;
        }
        obj->als_deb_on = 0;
    }

    return 0;
ENABLE_ALS_EXIT_ERR:
    return res;
}
int cm36558_read_ps(unsigned int *data)
{
    int res;
    unsigned char databuf[2];
    struct cm36558_priv *obj = &cm36558_obj;

    databuf[0] = CM36558_REG_PS_DATA;
    res = cm36558_i2c_read(databuf, 0x2);
    if (res < 0) {
        APS_ERR("cm36558_i2c_read function err\n\r");
        goto READ_PS_EXIT_ERR;
    }

    if (databuf[0] < obj->ps_cali)
        *data = 0;
    else
        *data = databuf[0] - obj->ps_cali;
    return 0;
READ_PS_EXIT_ERR:
    return res;
}


int cm36558_read_als(unsigned short *data)
{
    int res;
    struct cm36558_priv *obj = &cm36558_obj;
    unsigned char databuf[2] = {0};

    databuf[0] = CM36558_REG_ALS_DATA;
    res = cm36558_i2c_read(databuf, 0x2);
    if (res < 0) {
        APS_ERR("cm36558_i2c_read function err\n\r");
        goto READ_ALS_EXIT_ERR;
    }

    *data = ((databuf[1] << 8) | databuf[0]);
    if (obj->trace == CMC_TRC_ALS_DATA) {
        APS_DBG("ALS raw data: %d\n\r", *data);
    }
    return 0;
READ_ALS_EXIT_ERR:
    return res;
}
static int cm36558_get_ps_value(struct cm36558_priv *obj, unsigned int ps)
{
    int val, mask = obj->ps_mask;
    int invalid = 0;
    val = 0;

    if (ps > obj->ps_thd_val_high) {
        val = 0;  /*close*/
    } else if (ps < obj->ps_thd_val_low) {
        val = 1;  /*far away*/
    }

    if (obj->ps_suspend) {
        invalid = 1;
    } else if (1 == obj->ps_deb_on) {
        if (check_timeout(&obj->ps_deb_end)) {
            obj->ps_deb_on = 0;
        }

        if (1 == obj->ps_deb_on) {
            invalid = 1;
        }
    }

    if (!invalid) {
        if (obj->trace & CMC_TRC_CVT_PS) {
            if (mask) {
                APS_LOG("PS:  %05d => %05d [M]\n\r", ps, val);
            } else {
                APS_LOG("PS:  %05d => %05d\n\r", ps, val);
            }
        }
        if (0 == (CMC_BIT_PS & obj->enable)) {
            //if ps is disable do not report value
            APS_LOG("PS: not enable and do not report this value\n\r");
            return -1;
        } else {
            return val;
        }

    } else {
        if (obj->trace & CMC_TRC_CVT_PS) {
            APS_LOG("PS:  %05d => %05d (-1)\n\r", ps, val);
        }
        return -1;
    }
}

static int cm36558_get_als_value(struct cm36558_priv *obj, unsigned short als)
{
    int idx;
    int invalid = 0;
    for (idx = 0; idx < obj->als_level_num; idx++) {
        if (als < obj->hw->als_level[idx]) {
            break;
        }
    }

    if (idx >= obj->als_value_num) {
        APS_ERR("exceed range\n\r");
        idx = obj->als_value_num - 1;
    }

    if (1 == obj->als_deb_on) {
        if (check_timeout(&obj->als_deb_end)) {
            obj->als_deb_on = 0;
        }

        if (1 == obj->als_deb_on) {
            invalid = 1;
        }
    }

    if (!invalid) {
        int level_high = obj->hw->als_level[idx];
        int level_low = (idx > 0) ? obj->hw->als_level[idx - 1] : 0;
        int level_diff = level_high - level_low;
        int value_high = obj->hw->als_value[idx];
        int value_low = (idx > 0) ? obj->hw->als_value[idx - 1] : 0;
        int value_diff = value_high - value_low;
        int value = 0;

        if ((level_low >= level_high) || (value_low >= value_high))
            value = value_low;
        else
            value = (level_diff * value_low + (als - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;

        APS_LOG("ALS: %d [%d, %d] => %d [%d, %d] \n\r", als, level_low, level_high, value, value_low, value_high);
        return value;
    } else {
        if (obj->trace & CMC_TRC_CVT_ALS) {
            APS_LOG("ALS: %05d => %05d (-1)\n\r", als, obj->hw->als_value[idx]);
        }
        return -1;
    }

}

static int cm36558_check_intr()
{
    int res;
    unsigned char databuf[2];
    struct cm36558_priv *obj = &cm36558_obj;

    databuf[0] = CM36558_REG_INT_FLAG;
    res = cm36558_i2c_read(databuf, 0x2);
    if (res < 0) {
        APS_ERR("cm36558_i2c_read function err res = %d\n\r", res);
        goto EXIT_ERR;
    }

    APS_LOG("CM36558_REG_INT_FLAG value value_low = %x, value_high = %x\n\r", databuf[0], databuf[1]);

    if (databuf[1] & 0x02) {
        obj->intr_flag = 0;//for close
    } else if (databuf[1] & 0x01) {
        obj->intr_flag = 1;//for away
    } else {
        res = -1;
        APS_ERR("cm36558_check_intr fail databuf[1]&0x01: %d\n\r", res);
        goto EXIT_ERR;
    }

    return 0;
EXIT_ERR:
    APS_ERR("cm36558_check_intr dev: %d\n\r", res);
    return res;
}

void cm36558_eint_handler(int arg)
{
    APS_FUN(f);
    struct cm36558_priv *obj = &cm36558_obj;
    obj->is_interrupt_context = 1;
    sensor_subsys_algorithm_notify(SENSOR_TYPE_PROXIMITY);

}

int cm36558_setup_eint(void)
{
    mt_eint_dis_hw_debounce(11);
    mt_eint_registration(11, LEVEL_SENSITIVE, LOW_LEVEL_TRIGGER, cm36558_eint_handler, EINT_INT_UNMASK,
                         EINT_INT_AUTO_UNMASK_OFF);
    return 0;
}
/************************************************************************/
int set_psensor_threshold()
{
    struct cm36558_priv *obj = &cm36558_obj;
    unsigned char databuf[3];
    int res = 0;

    APS_ERR("set_psensor_threshold function high: %d, low:%d\n\r", obj->ps_thd_val_high, obj->ps_thd_val_low);

    databuf[0] = CM36558_REG_PS_THDL;
    databuf[1] = (unsigned char)(obj->ps_thd_val_low & 0xFF);
    databuf[2] = (unsigned char)(obj->ps_thd_val_low >> 8);
    res = cm36558_i2c_write(databuf, 0x3);
    if (res < 0) {
        APS_ERR("CM36558_REG_PS_THDL\n\r");
        return -1;
    }
    databuf[0] = CM36558_REG_PS_THDH;
    databuf[1] = (unsigned char)(obj->ps_thd_val_high & 0xFF);
    databuf[2] = (unsigned char)(obj->ps_thd_val_high >> 8);
    res = cm36558_i2c_write(databuf, 0x3);
    if (res < 0) {
        APS_ERR("CM36558_REG_PS_THDH err\n\r");
        return -1;
    }
    return 0;

}
int cm36558_check_devID(void)
{
    int res = 0;
    unsigned char databuf[3];
    databuf[0] = CM36558_REG_ID;
    res = cm36558_i2c_read(databuf, 2);
    if (res <  0) {
        return CM36558_ERR_I2C;
    }
    APS_LOG("ID0 = %x, ID1 = %x\n\r", databuf[0], databuf[1]);
    return CM36558_SUCCESS;
}
int cm36558_init_client(void)
{
    int res = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    unsigned char databuf[3];
    res = cm36558_check_devID();
    if (res < 0) {
        APS_ERR("cm36558_check_devID err\n\r");
        goto EXIT_ERR;
    }
    databuf[0] = CM36558_REG_ALS_CONF;
    if (1 == obj->hw->polling_mode_als)
        databuf[1] = 0b00000001;
    else
        databuf[1] = 0b00000011;
    databuf[2] = 0b00000001;
    res = cm36558_i2c_write(databuf, 0x3);
    if (res <  0) {
        APS_ERR("CM36558_REG_ALS_CONF err\n\r");
        goto EXIT_ERR;
    }
    if (true == obj->hw->polling_mode_als) {
        databuf[0] = CM36558_REG_ALS_THDH;
        databuf[1] = (unsigned char)(obj->als_thd_val_high & 0xFF);
        databuf[2] = (unsigned char)(obj->als_thd_val_high >> 8);
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("i2c_master_send function err\n\r");
            goto EXIT_ERR;
        }
        databuf[0] = CM36558_REG_ALS_THDL;
        databuf[1] = (unsigned char)(obj->als_thd_val_low & 0xFF);
        databuf[2] = (unsigned char)(obj->als_thd_val_low >> 8);
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("i2c_master_send function err\n\r");
            goto EXIT_ERR;
        }
    }

    databuf[0] = CM36558_REG_PS_CONF1_2;
    databuf[1] = 0b00010001;
    if (true == obj->hw->polling_mode_ps)
        databuf[2] = 0b00000000;
    else
        databuf[2] = 0b00000011;
    res = cm36558_i2c_write(databuf, 0x3);
    if (res <  0) {
        APS_LOG("CM36558_REG_PS_CONF1_2 err\n\r");
        goto EXIT_ERR;
    }

    databuf[0] = CM36558_REG_PS_CONF3_MS;
    databuf[1] = 0b00010000;
    databuf[2] = 0b00000010;
    res = cm36558_i2c_write(databuf, 0x3);
    if (res <  0) {
        APS_LOG("CM36558_REG_PS_CONF3_MS err\n\r");
        goto EXIT_ERR;
    }

    databuf[0] = CM36558_REG_PS_CANC;
    databuf[1] = 0x00;
    databuf[2] = 0x00;
    res = cm36558_i2c_write(databuf, 0x3);
    if (res < 0) {
        APS_ERR("CM36558_REG_PS_CANC err\n\r");
        goto EXIT_ERR;
    }

    if (false == obj->hw->polling_mode_ps) {
        databuf[0] = CM36558_REG_PS_THDL;
        databuf[1] = (unsigned char)(obj->ps_thd_val_low & 0xFF);
        databuf[2] = (unsigned char)(obj->ps_thd_val_low >> 8);
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("CM36558_REG_PS_THDL\n\r");
            goto EXIT_ERR;
        }
        databuf[0] = CM36558_REG_PS_THDH;
        databuf[1] = (unsigned char)(obj->ps_thd_val_high & 0xFF);
        databuf[2] = (unsigned char)(obj->ps_thd_val_high >> 8);
        res = cm36558_i2c_write(databuf, 0x3);
        if (res < 0) {
            APS_ERR("CM36558_REG_PS_THDH err\n\r");
            goto EXIT_ERR;
        }
        cm36558_setup_eint();
    }
    return CM36558_SUCCESS;

EXIT_ERR:
    APS_ERR("cm36558_init_client fail: %d\n\r", res);
    return res;
}

static void dump_cm36558_obj_hw()
{
    int i = 0;

    APS_LOG("cm36558_obj.hw.i2c_num = %d\n\r", cm36558_obj.hw->i2c_num);
    APS_LOG("cm36558_obj.hw.i2c_addr");
    for (i = 0; i < C_CUST_I2C_ADDR_NUM; i++) {
        APS_LOG(", [%d] = %d ", i, cm36558_obj.hw->i2c_addr[i]);
    }
    APS_LOG("\n\r");
    APS_LOG("cm36558_obj.hw.polling_mode_ps = %d\n\r", cm36558_obj.hw->polling_mode_ps);
    APS_LOG("cm36558_obj.hw.polling_mode_als = %d\n\r", cm36558_obj.hw->polling_mode_als);


    APS_LOG("cm36558_obj.hw.als_level");
    for (i = 0; i < C_CUST_ALS_LEVEL - 1; i++) {
        APS_LOG(", [%d] = %d ", i, cm36558_obj.hw->als_level[i]);
    }
    APS_LOG("\n\r");
    APS_LOG("cm36558_obj.hw.als_value");
    for (i = 0; i < C_CUST_ALS_LEVEL; i++) {
        APS_LOG(", [%d] = %d ", i, cm36558_obj.hw->als_value[i]);
    }
    APS_LOG("\n\r");
    APS_LOG("cm36558_obj.hw.ps_threshold_high = %d\n\r", cm36558_obj.hw->ps_threshold_high);
    APS_LOG("cm36558_obj.hw.ps_threshold_low = %d\n\r", cm36558_obj.hw->ps_threshold_low);
    APS_LOG("cm36558_obj.hw.als_threshold_high = %d\n\r", cm36558_obj.hw->als_threshold_high);
    APS_LOG("cm36558_obj.hw.als_threshold_low = %d\n\r", cm36558_obj.hw->als_threshold_low);
}
static int cust_config()
{
    struct cm36558_priv *obj = &cm36558_obj;
    int err = 0;

    APS_FUN(f);

    dump_cm36558_obj_hw();

    /*-----------------------------value need to be confirmed-----------------------------------------*/
    obj->als_debounce = 200;
    obj->als_deb_on = 0;
    obj->ps_debounce = 200;
    obj->ps_deb_on = 0;
    obj->ps_mask = 0;
    obj->als_suspend = 0;
    obj->als_cmd_val = 0xDF;
    obj->ps_cmd_val = 0xC1;
    obj->ps_thd_val_high = obj->hw->ps_threshold_high;
    obj->ps_thd_val_low = obj->hw->ps_threshold_low;
    obj->als_thd_val_high = obj->hw->als_threshold_high;
    obj->als_thd_val_low = obj->hw->als_threshold_low;

    obj->enable = 0;
    obj->pending_intr = 0;
    obj->is_interrupt_context = 0;
    obj->ps_cali = 0;
    obj->als_level_num = sizeof(obj->hw->als_level) / sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value) / sizeof(obj->hw->als_value[0]);
    obj->enable |= CMC_BIT_ALS;
    obj->enable |= CMC_BIT_PS;

    APS_LOG("%s: OK\n\r", __FUNCTION__);

    return err;
}
static int cm36558_dump_register(void)
{
    int ret = 0;

    return ret;
}
static int als_cust_set(void *data, int len)
{
    int err = 0;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    switch (req->cust.action) {
        case CUST_ACTION_SHOW_ALSLV:
        case CUST_ACTION_SHOW_ALSVAL:
            dump_cm36558_obj_hw();
            break;
        case CUST_ACTION_GET_RAW_DATA:
            APS_LOG("enter CUST_ACTION_GET_RAW_DATA action\n\r");
            break;
        default:
            err = -1;
            break;
    }
    return err;
}

static int ps_cust_set(void *data, int len)
{
    int err = 0;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:
            break;
        case CUST_ACTION_RESET_CALI:
            cm36558_obj.ps_cali = 0;
            APS_LOG("cm36558_obj.ps_cali = %d\n\r", cm36558_obj.ps_cali);
            break;
        case CUST_ACTION_SET_CALI:
            cm36558_obj.ps_cali = req->setCali.int32_data[0];
            APS_LOG("cm36558_obj.ps_cali = %d\n\r", cm36558_obj.ps_cali);
            break;
        case CUST_ACTION_SET_PS_THRESHOLD:
            cm36558_obj.ps_thd_val_low = (req->setPSThreshold.threshold[0] + cm36558_obj.ps_cali);
            cm36558_obj.ps_thd_val_high = (req->setPSThreshold.threshold[1] + cm36558_obj.ps_cali);

            set_psensor_threshold();
            break;
        case CUST_ACTION_SHOW_REG:
            cm36558_dump_register();
            break;
        case CUST_ACTION_SET_TRACE:
            cm36558_obj.trace = req->setTrace.trace;
            break;
        default:
            err = -1;
            break;
    }
    return err;
}
int cm36558_ps_operation(Sensor_Command command, void* buff_in, int size_in, void* buff_out, int size_out)
{
    int err = 0;
    int value;
    struct cm36558_priv *obj = &cm36558_obj;

    APS_FUN(f);


    switch (command) {
        case SETDELAY:
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Set delay parameter error!\n\r");
                err = -1;
            }

            break;
        case ACTIVATE:
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buff_in;
                if (value) {
                    if ((err = cm36558_enable_ps(1))) {
                        APS_ERR("enable ps fail: %d\n\r", err);
                        return -1;
                    }
                    obj->enable |= CMC_BIT_PS;
                } else {
                    if ((err = cm36558_enable_ps(0))) {
                        APS_ERR("disable ps fail: %d\n\r", err);
                        return -1;
                    }
                    obj->enable &= ~CMC_BIT_PS;
                }
            }

            break;
        case SETCUST:
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                if (err != ps_cust_set(buff_in, size_in)) {
                    APS_ERR("Set customization error : %d\n\r", err);
                }
            }

            break;

        default:
            APS_ERR("proxmy sensor operate function no this parameter %d!\n\r", command);
            err = -1;
            break;
    }

    return err;
}

int cm36558_als_operation(Sensor_Command command, void* buff_in, int size_in, void* buff_out, int size_out)
{
    int err = 0;
    int value = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    APS_FUN(f);
    switch (command) {
        case SETDELAY:
            //APS_ERR("cm36558 als delay command! %d \n\r",*(int *)buff_in);
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Set delay parameter error!\n\r");
                err = -1;
            }
            break;
        case ACTIVATE:
            //APS_ERR("cm36558 als enable command! %d\n\r",*(int *)buff_in);
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buff_in;
                if (value) {
                    if ((err = cm36558_enable_als(1))) {
                        APS_ERR("enable als fail: %d\n\r", err);
                        return -1;
                    }
                    obj->enable |= CMC_BIT_ALS;
                } else {
                    if ((err = cm36558_enable_als(0))) {
                        APS_ERR("disable als fail: %d\n\r", err);
                        return -1;
                    }
                    obj->enable &= ~CMC_BIT_ALS;
                }
            }
            break;
        case SETCUST:
            APS_LOG("akm09912_operation command SETCUST\n\r");
            if ((buff_in == NULL) || (size_in < sizeof(int))) {
                APS_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                if (err != als_cust_set(buff_in, size_in)) {
                    APS_ERR("Set customization error : %d\n\r", err);
                    err = -1;
                }
            }
            break;
        default:
            APS_ERR("light sensor operate function no this parameter %d!\n\r", command);
            err = -1;
            break;
    }

    return err;
}
int cm36558_ps_run_algorithm(struct data_t *output)
{
    int ret = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    if (obj->is_interrupt_context == 1) {
        obj->is_interrupt_context = 0;
        ret = cm36558_check_intr();
        if (ret == 0) {
            output->data_exist_count = 1;
            output->data->sensor_type = SENSOR_TYPE_PROXIMITY;
            output->data->time_stamp = read_xgpt_stamp_ns();
            output->data->proximity_t.oneshot = obj->intr_flag;

        }
        mt_eint_unmask(11);
        APS_LOG("psensor interrupt oneshot: %d, steps: %d!\n\r", output->data->proximity_t.oneshot,
                output->data->proximity_t.steps);
    } else {
        if ((ret = cm36558_read_ps(&obj->ps))) {
            ret = -1;
        } else {
            output->data_exist_count = 1;
            output->data->sensor_type = SENSOR_TYPE_PROXIMITY;
            output->data->time_stamp = read_xgpt_stamp_ns();
            output->data->proximity_t.steps = obj->ps;
            output->data->proximity_t.oneshot = cm36558_get_ps_value(obj, obj->ps);
            if (output->data->proximity_t.oneshot < 0)
                ret = -1;
        }

        APS_LOG("psensor oneshot: %d, steps: %d!\n\r", output->data->proximity_t.oneshot, output->data->proximity_t.steps);
    }
    return ret;
}
int cm36558_als_run_algorithm(struct data_t *output)
{
    int ret = 0;
    struct cm36558_priv *obj = &cm36558_obj;
    if ((ret = cm36558_read_als(&obj->als))) {
        ret = -1;;
    } else {
        output->data_exist_count = 1;
        output->data->sensor_type = SENSOR_TYPE_LIGHT;
        output->data->time_stamp = read_xgpt_stamp_ns();
        output->data->light = cm36558_get_als_value(obj, obj->als);
        if (output->data->light < 0)
            ret = -1;
    }
    return ret;
}
//#define CM36558_TESTSUITE_CASE
#ifdef CM36558_TESTSUITE_CASE
static void CM36558testsample(void *pvParameters)
{
    struct cm36558_priv *obj = &cm36558_obj;
    while (1) {
        kal_taskENTER_CRITICAL();
        APS_FUN(f);
        //cm36558_als_run_algorithm(obj->als_sensor_event_t);
        //APS_ERR("LIGHT: %d\n\r", obj->als_sensor_event_t->data->light);
        kal_taskEXIT_CRITICAL();
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}
#endif

int cm36558_sensor_init(void)
{
    int ret = 0;
    APS_FUN(f);
    struct SensorDescriptor_t  cm36558_ps_descriptor_t;
    struct SensorDescriptor_t  cm36558_als_descriptor_t;
    struct cm36558_priv *obj = &cm36558_obj;

    cm36558_ps_descriptor_t.sensor_type = SENSOR_TYPE_PROXIMITY;
    cm36558_ps_descriptor_t.version =  1;
    cm36558_ps_descriptor_t.report_mode = one_shot;
    cm36558_ps_descriptor_t.hw.max_sampling_rate = 5;
    cm36558_ps_descriptor_t.hw.support_HW_FIFO = 0;

    cm36558_ps_descriptor_t.input_list = NULL;

    cm36558_ps_descriptor_t.operate = cm36558_ps_operation;
    cm36558_ps_descriptor_t.run_algorithm = cm36558_ps_run_algorithm;
    cm36558_ps_descriptor_t.set_data = NULL;


    cm36558_als_descriptor_t.sensor_type = SENSOR_TYPE_LIGHT;
    cm36558_als_descriptor_t.version =  1;
    cm36558_als_descriptor_t.report_mode = on_change;
    cm36558_als_descriptor_t.hw.max_sampling_rate = 5;
    cm36558_als_descriptor_t.hw.support_HW_FIFO = 0;

    cm36558_als_descriptor_t.input_list = NULL;

    cm36558_als_descriptor_t.operate = cm36558_als_operation;
    cm36558_als_descriptor_t.run_algorithm = cm36558_als_run_algorithm;
    cm36558_als_descriptor_t.set_data = NULL;
    obj->hw = get_cust_alsps_hw();
    if (NULL == obj->hw) {
        APS_ERR("get_cust_alsps_hw fail\n\r");
        return ret;
    }
    cust_config();
    obj->i2c_addr = obj->hw->i2c_addr[0];
    APS_LOG("i2c_num: %d, i2c_addr: 0x%x\n\r", obj->hw->i2c_num, obj->i2c_addr);

    ret = cm36558_init_client();
    if (ret < 0) {
        APS_ERR("cm36558_init_client fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PROXIMITY, 1);
    if (ret < 0) {
        APS_ERR("RegisterDataBuffer fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_LIGHT, 1);
    if (ret < 0) {
        APS_ERR("RegisterDataBuffer fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&cm36558_ps_descriptor_t);
    if (ret) {
        APS_ERR("RegisterAlgorithm fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&cm36558_als_descriptor_t);
    if (ret) {
        APS_ERR("RegisterAlgorithm fail\n\r");
        return ret;
    }
#ifdef CM36558_TESTSUITE_CASE
    kal_xTaskCreate(CM36558testsample, "aps", 512, (void*)NULL, 3, NULL);
#endif
    return ret;
}
MODULE_DECLARE(cm36558, MOD_PHY_SENSOR, cm36558_sensor_init);
