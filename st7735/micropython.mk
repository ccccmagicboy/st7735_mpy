ST7735_MOD_DIR := $(USERMOD_DIR)
SRC_USERMOD += $(addprefix $(ST7735_MOD_DIR)/, \
	st7735.c \
)
CFLAGS_USERMOD += -I$(ST7735_MOD_DIR) -DMODULE_ST7735_ENABLED=1
CFLAGS_USERMOD += -DEXPOSE_EXTRA_METHODS=1
