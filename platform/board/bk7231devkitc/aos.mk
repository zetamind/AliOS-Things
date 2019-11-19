NAME := board_bk7231

JTAG := jlink

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION    := 1.0.1
$(NAME)_SUMMARY    := configuration for board bk7231

MODULE               := BK7231
HOST_ARCH            := ARM968E-S
HOST_MCU_FAMILY      := mcu_bk7231
SUPPORT_MBINS        := no

$(NAME)_COMPONENTS += $(HOST_MCU_FAMILY) kernel_init

$(NAME)_SOURCES := board.c

GLOBAL_INCLUDES += .
GLOBAL_DEFINES  += STDIO_UART=1
GLOBAL_DEFINES  += CLI_CONFIG_SUPPORT_BOARD_CMD=1

CONFIG_SYSINFO_PRODUCT_MODEL := ALI_AOS_BK7231
CONFIG_SYSINFO_DEVICE_NAME   := BK7231

GLOBAL_CFLAGS += -DSYSINFO_PRODUCT_MODEL=\"$(CONFIG_SYSINFO_PRODUCT_MODEL)\"
GLOBAL_CFLAGS += -DSYSINFO_DEVICE_NAME=\"$(CONFIG_SYSINFO_DEVICE_NAME)\"
#GLOBAL_CFLAGS += -DSYSINFO_APP_VERSION=\"$(CONFIG_SYSINFO_APP_VERSION)\"

GLOBAL_LDS_INCLUDES += $($(NAME)_LOCATION)/bk7231devkitc.ld.S