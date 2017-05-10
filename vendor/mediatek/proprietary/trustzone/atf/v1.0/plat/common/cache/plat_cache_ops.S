#define __ALIGN         .align 4

#define ENTRY(name) \
  .globl name; \
  __ALIGN; \
  name:

#define END(name) \
  .size name, .-name

#define ENDPROC(name) \
  .type name, @function; \
  END(name)

/**
 * input: None
 * output: x0 indicates the value of ccsidr L1 icache
 */
ENTRY(mt_L1_icache_ccsidr)
	mov x0, #1
	msr csselr_el1, x0
	isb
	mrs x0, ccsidr_el1
	ret
ENDPROC(mt_L1_icache_ccsidr)

/**
 * Return the tag address and other line attributes
 * input: x0 for set, way location encode
 * output: x0 is the value containing tag address & related attributes
 */
ENTRY(mt_L1_icache_tag_ca53)
	msr S3_3_c15_c2_1, x0
	isb
	mrs x0, S3_3_c15_c0_0
	ret
ENDPROC(mt_L1_icache_tag_ca53)

/**
 * Return 40bit raw data in the specific location of L1 icache
 * input: x0 for set, way location encode
 * output: x0 is 40bit raw data
 */
ENTRY(mt_L1_icache_raw_data_ca53)
	msr S3_3_c15_c4_1, x0
	isb
	mrs x0, S3_3_c15_c0_0
	mrs x1, S3_3_c15_c0_1
	and x0, x0, #0xfffff
	and x1, x1, #0xfffff
	lsl x1, x1, #20
	orr x0, x1, x0
	ret
ENDPROC(mt_L1_icache_raw_data_ca53)


/**
 * write ramindex ca72
 * input: x0 for set, way location encode
 * output:
 */
ENTRY(mt_write_ramindex_ca72)
	sys #0, c15, c4, #0, x0
	dsb sy
	isb
	ret
ENDPROC(mt_write_ramindex_ca72)

ENTRY(mt_read_il1_data0_ca72)
	mrs x0, s3_0_c15_c0_0
	isb
	dsb sy
	ret
ENDPROC(mt_read_il1_data0_ca72)

ENTRY(mt_read_il1_data1_ca72)
	mrs x0, s3_0_c15_c0_1
	isb
	dsb sy
	ret
ENDPROC(mt_read_il1_data1_ca72)


