

TEEID_DIR		:=	services/spd/teeid
SPD_INCLUDES		:=	-I${TEEID_DIR}

SPD_SOURCES		:=	teei_fastcall.c	\
				teei_main.c		\
				teei_helpers.S		\
				teei_common.c	\
				teei_pm.c		

vpath %.c ${TEEID_DIR}
vpath %.S ${TEEID_DIR}

