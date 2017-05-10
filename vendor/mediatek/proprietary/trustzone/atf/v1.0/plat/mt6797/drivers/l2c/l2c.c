#include <platform_def.h>
#include <arch.h>
#include <arch_helpers.h>
#include <mmio.h>
#include <sip_error.h>
#include <spinlock.h>
#include <debug.h>
#include "plat_private.h"
#include "l2c.h"


spinlock_t l2_share_lock;

void config_L2_size()
{
        unsigned int cache_cfg0;

        /* D1: mp0 L2$ 512KB; D2: mp0 L2$ 256KB */
        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) & (0xF << L2C_SIZE_CFG_OFF);
        cache_cfg0 = (cache_cfg0 << 1) | (0x1 << L2C_SIZE_CFG_OFF);
        cache_cfg0 = (mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~(0xF << L2C_SIZE_CFG_OFF)) | cache_cfg0;
	mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
	mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
#if 0
        /* mp1 L2$ 512KB */
        cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) | (CONFIGED_512K << L2C_SIZE_CFG_OFF);
        mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
        cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
	mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
#endif
}
#if 0
uint64_t switch_L2_size(uint64_t option, uint64_t share_cluster_num, uint64_t cluster_borrow_return)
{
    unsigned int cache_cfg0, cache_cfg1;
    int ret = SIP_SVC_E_SUCCESS;
    l2c_share_info share_info;

    share_info.cluster_borrow = (cluster_borrow_return >> 16) & 0xFFFF;
    share_info.cluster_return = cluster_borrow_return & 0xFFFF;
    share_info.share_cluster_num = share_cluster_num;

    spin_lock(&l2_share_lock);

    //printf("switch L2$ size 1. option = %d\n", option);

    dis_i_d_dcsw_op_all(DCCISW);

    //printf("switch L2$ size 2\n");

    cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG);
    cache_cfg0 &= ~(0xf << L2C_SIZE_CFG_OFF);
    cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG);
    cache_cfg1 &= ~(0xf << L2C_SIZE_CFG_OFF);

    switch(option) {
    case BORROW_L2:
	if(share_info.share_cluster_num == 1)	
	{
	        cache_cfg0 |= (share_info.cluster_borrow << L2C_SIZE_CFG_OFF);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) | (0x1 << L2C_SHARE_ENABLE);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        //printf("switch L2$ size 3\n");
	}
	else if(share_info.share_cluster_num == 2)
	{
		cache_cfg0 |= (share_info.cluster_borrow << L2C_SIZE_CFG_OFF);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) | (0x1 << L2C_SHARE_ENABLE);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        //printf("switch L2$ size 3\n");

	        cache_cfg1 |= (share_info.cluster_borrow << L2C_SIZE_CFG_OFF);
	        mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
	        cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) | (0x1 << L2C_SHARE_ENABLE);
	        mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
	}
        //printf("switch L2$ size 4\n");
        break;
    case RETURN_L2:
	if(share_info.share_cluster_num == 1)	
	{
	        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) | (share_info.cluster_return << L2C_SIZE_CFG_OFF);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        //printf("switch L2$ size 5\n");
	}
	else if(share_info.share_cluster_num == 2)
	{
		cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) | (share_info.cluster_return << L2C_SIZE_CFG_OFF);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);
	        cache_cfg0 = mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
	        mmio_write_32(MP0_CA7L_CACHE_CONFIG, cache_cfg0);

		/* mp1 L2$ 512KB */
            	cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) | (share_info.cluster_return << L2C_SIZE_CFG_OFF);
            	mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
            	cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
            	mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
	}

#if 0
        if((get_devinfo_with_index(0)&0xff) == 0xc0)
        {
            printf("switch L2$ size 5.1\n");
            /* mp1 L2$ 256KB */
            cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) | (CONFIGED_256K << L2C_SIZE_CFG_OFF);
            mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
            cache_cfg1 = mmio_read_32(MP1_CA7L_CACHE_CONFIG) & ~(0x1 << L2C_SHARE_ENABLE);
            mmio_write_32(MP1_CA7L_CACHE_CONFIG, cache_cfg1);
        }
        else
        {
#endif          
            //printf("switch L2$ size 5.2\n");            
        //}
        //printf("switch L2$ size 6\n");
        break;    
    default:        
        ret = SIP_SVC_E_NOT_SUPPORTED;
        break;
    }


    enable_cache();

    printf("switch L2$ size 7\n");

    spin_unlock(&l2_share_lock);

    return ret;
}
#endif

