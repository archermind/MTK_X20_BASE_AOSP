TRUSTY_DIR     :=  services/spd/trusty
SPD_INCLUDES		:= -I${TRUSTY_DIR}

SPD_SOURCES		:=	services/spd/trusty/trusty.c		\
				services/spd/trusty/trusty_helpers.S	\

NEED_BL32		:=	no
