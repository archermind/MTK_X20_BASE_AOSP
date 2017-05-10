#ifndef	_ERROR_CODE_H_
#define	_ERROR_CODE_H_

#define SUCCESSED(code) (code == STATUS_OK)
#define FAIL(code) (code != STATUS_OK)

#define STATUS_OK                           0x0
#define STATUS_ERR                          0x1
#define STATUS_ABORT                        0x2
#define STATUS_UNSUPPORT_CMD                0x3
#define STATUS_PROTOCOL_ERR                 0x4
#define STATUS_BUFFER_OVERFLOW              0x5
#define STATUS_SECURITY_DENY                0x6
#define STATUS_CHECKSUM_ERR                 0x7
#define STATUS_THREAD                       0x8
#define STATUS_USB_SCAN_ERR                 0x9
#define STATUS_INVALID_SESSION              0xA
#define STATUS_INVALID_STAGE                0xB
#define STATUS_NOT_IMPLEMENTED              0xC

//device err
#define STATUS_UNSUPPORT_OP                 0x1000
#define STATUS_READ_DATA_FAIL               0x1001
#define STATUS_WRITE_DATA_FAIL              0x1002
#define STATUS_DATA_FAIL                    0x1003
#define STATUS_USB_FAIL                     0x1004
#define STATUS_TOO_LARGE                    0x1005
#define STATUS_SPARSE_INCOMPLETE            0x1006
#define STATUS_UNKNOWN_TYPE                 0x1007
#define STATUS_UNSUPPORT_CTRL_CODE          0x1008
#define STATUS_ERASE_FAIL                   0x1009
#define STATUS_PARTITON_NOT_FOUND           0x100A
#define STATUS_READ_PT_FAIL                 0x100B
#define STATUS_EXCEED_MAX_PART_NUM          0x100C
#define STATUS_UNKNOWN_STORAGE_TYPE         0x100D
#define STATUS_DRAM_TEST_FAILED                        0x100E
#define STATUS_DETECT_DRAM_TEST_FAILED        0x100F


//command
#define STATUS_DEVICE_CTRL_EXCEPTION        0x3000
#define STATUS_SHUTDOWN_CMD_FAIL            0x3001
#define STATUS_DOWNLOAD_FAIL                0x3002

/* security */
#define ROM_INFO_NOT_FOUND                  0x4000
#define CUST_NAME_NOT_FOUND                 0x4001
#define ROM_INFO_DEVICE_NOT_SUPPORTED       0x4002

//brom
#define STATUS_BROM_CMD_STARTCMD_FAIL        0x10000
#define STATUS_BROM_BBCHIP_HW_VER_INCORRECT  0x10001
#define STATUS_BROM_CMD_SEND_DA_FAIL         0x10002
#define STATUS_BROM_CMD_JUMP_DA_FAIL         0x10003
#define STATUS_BROM_CMD_FAIL                0x10004
/*#define STATUS_BROM_CMD_SEND_DA_FAIL         0x10005
#define STATUS_BROM_CMD_SEND_DA_FAIL         0x10006
*/


//Lib
#define STATUS_SCATTER_FILE_INVALID         0x11000
#define STATUS_DA_FILE_INVALID              0x11001
#define STATUS_INVALID_HSESSION             0x11002
#define STATUS_DA_SELECTION_ERR             0x11003
#define STATUS_FILE_NOT_FOUND               0x11004
#define STATUS_PRELOADER_INVALID            0x11005
#define STATUS_EMI_HDR_INVALID              0x11006
#define STATUS_OPEN_FILE_ERR                0x11007
#define STATUS_WRITE_FILE_ERR               0x11008
#define STATUS_READ_FILE_ERR                0x11009
#define STATUS_STORAGE_MISMATCH             0x1100A
#define STATUS_INVALID_PARAMETER            0x1100B
#define STATUS_INVALID_GPT                  0x1100C
#define STATUS_INVALID_PMT                  0x1100D
#define STATUS_WRITE_PT_FAILED              0x1100E
#define STATUS_INSUFFICIENT_BUFFER          0x1100F
#define STATUS_LAYOUT_CHANGED               0x11010
#define STATUS_PARSE_XML_ERROR              0x11011
#define STATUS_NODE_NOT_EXIST               0x11012
#define STATUS_UNKNOWN_TEST_TYPE            0x11013

#define STATUS_SEC_DL_FORBIDDEN             0x6000
#define STATUS_SEC_IMG_TOO_LARGE            0x6001
#define STATUS_SEC_PL_VFY_FAIL              0x6002
#define STATUS_SEC_IMG_VFY_FAIL             0x6003
#define STATUS_SEC_HASH_OP_FAIL             0x6004
#define STATUS_SEC_HASH_BINDING_CHK_FAIL    0x6005
#define STATUS_SEC_INVALID_BUF              0x6006
#define STATUS_SEC_BINDING_HASH_NOT_AVAIL   0x6007
#define STATUS_SEC_WRITE_DATA_NOT_ALLOWED   0x6008
#define STATUS_SEC_FORMAT_NOT_ALLOWED       0x6009
#define STATUS_SEC_SV5_PUBK_AUTH_FAIL       0x600a
#define STATUS_SEC_SV5_HASH_VFY_FAIL        0x600b
#define STATUS_SEC_SV5_RSA_OP_FAIL          0x600c
#define STATUS_SEC_SV5_RSA_VFY_FAIL         0x600d
#define STATUS_SEC_SV5_GFH_NOT_FOUND        0x600e
#define STATUS_SEC_NOT_VALID_CERT1          0x600f
#define STATUS_SEC_NOT_VALID_CERT2          0x6010
#define STATUS_SEC_NOT_VALID_IMGHDR         0x6011
#define STATUS_SEC_SIG_SZ_NOT_VALID         0x6012
#define STATUS_SEC_PSS_OP_FAIL              0x6013
#define STATUS_SEC_CERT_AUTH_FAIL           0x6014
#define STATUS_SEC_PUBK_AUTH_MISMATCH_N_SIZE  0x6015
#define STATUS_SEC_PUBK_AUTH_MISMATCH_E_SIZE  0x6016
#define STATUS_SEC_PUBK_AUTH_MISMATCH_N       0x6017
#define STATUS_SEC_PUBK_AUTH_MISMATCH_E       0x6018
#define STATUS_SEC_PUBK_AUTH_MISMATCH_HASH    0x6019
#define STATUS_SEC_CERT_OBJ_NOT_FOUND         0x601a
#define STATUS_SEC_CERT_OID_NOT_FOUND         0x601b
#define STATUS_SEC_CERT_OUT_OF_RANGE          0x601c
#define STATUS_SEC_OID_NOT_MATCH              0x601d
#define STATUS_SEC_LEN_NOT_MATCH              0x601e
#define STATUS_SEC_ASN1_UNKNOWN_OP            0x601f
#define STATUS_SEC_OID_IDX_OUT_OF_RANGE       0x6020
#define STATUS_SEC_OID_TOO_LARGE              0x6021
#define STATUS_SEC_PUBK_SZ_MISMATCH           0x6022
#define STATUS_SEC_SWID_MISMATCH              0x6023
#define STATUS_SEC_HASH_SZ_MISMATCH           0x6024
#define STATUS_SEC_IMGHDR_TYPE_MISMATCH       0x6025
#define STATUS_SEC_IMG_TYPE_MISMATCH          0x6026
#define STATUS_SEC_IMGHDR_HASH_VFY_FAIL       0x6027
#define STATUS_SEC_IMG_HASH_VFY_FAIL          0x6028
#define STATUS_SEC_VIOLATE_ANTI_ROLLBACK      0x6029
#define STATUS_SEC_SECCFG_NOT_FOUND           0x602a
#define STATUS_SEC_SECCFG_MAGIC_INCORRECT     0x602b
#define STATUS_SEC_SECCFG_NOT_VALID           0x602c
#define STATUS_SEC_CIPHER_MODE_INVALID        0x602d
#define STATUS_SEC_CIPHER_KEY_INVALID         0x602e
#define STATUS_SEC_CIPHER_DATA_UNALIGNED      0x602f


#endif
