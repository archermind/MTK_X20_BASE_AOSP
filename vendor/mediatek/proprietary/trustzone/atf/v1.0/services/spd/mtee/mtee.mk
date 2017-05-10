
MTEE_DIR		:=	services/spd/mtee
SPD_INCLUDES		:=	-Iinclude/bl32/mtee \
				-I${MTEE_DIR}

SPD_SOURCES		:=	mtee_common.c		\
				mtee_helpers.S	\
				mtee_main.c		\
				mtee_pm.c

vpath %.c ${MTEE_DIR}
vpath %.S ${MTEE_DIR}

