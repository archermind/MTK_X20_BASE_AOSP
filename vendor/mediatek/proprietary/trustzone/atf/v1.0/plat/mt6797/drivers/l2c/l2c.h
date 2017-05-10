#ifndef _MT_L2C_H_
#define _MT_L2C_H_

#define CONFIGED_256K   0x1
#define CONFIGED_512K   0x3
#define L2C_SIZE_CFG_OFF 8
#define L2C_SHARE_ENABLE    12

enum options{
    BORROW_L2,
    RETURN_L2,
    BORROW_NONE
};

typedef struct _l2c_share_info{
        uint32_t share_cluster_num;
        uint32_t cluster_borrow;
        uint32_t cluster_return;
}l2c_share_info;

void config_L2_size();
//uint64_t switch_L2_size(uint64_t option, uint64_t share_cluster_num, uint64_t cluster_borrow_return);

#endif
