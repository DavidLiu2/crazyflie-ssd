# Makefile
# Responsibility: Build modular app + generated DORY network for GAP SDK targets.

CORE ?= 1

APP = main
APP_SRCS := main.c
APP_SRCS += $(wildcard src/*.c)
APP_SRCS += $(wildcard generated/*.c)
APP_SRCS += $(wildcard generated/src/*.c)

# -O2 with -fno-indirect-inlining is just as fast as -O3 and reduces code size considerably
# by not inlining of small functions in the management code
APP_CFLAGS += -DNUM_CORES=$(CORE) -I. -Iinc -Igenerated -Igenerated/inc -O2 -fno-indirect-inlining -flto -w
APP_LDFLAGS += -lm -Wl,--print-memory-usage -flto

GAP9_DEFAULT_FLASH_TYPE = DEFAULT_FLASH
GAP9_DEFAULT_RAM_TYPE = DEFAULT_RAM

GAP8_DEFAULT_FLASH_TYPE = HYPERFLASH
GAP8_DEFAULT_RAM_TYPE = HYPERRAM

PULP_DEFAULT_FLASH_TYPE = HYPERFLASH
PULP_DEFAULT_RAM_TYPE = HYPERRAM

FLASH_TYPE ?= $($(TARGET_CHIP_FAMILY)_DEFAULT_FLASH_TYPE)
RAM_TYPE ?= $($(TARGET_CHIP_FAMILY)_DEFAULT_RAM_TYPE)

APP_CFLAGS += -DGAP_SDK=1
APP_CFLAGS += -DTARGET_CHIP_FAMILY_$(TARGET_CHIP_FAMILY)

ifeq '$(FLASH_TYPE)' 'MRAM'
READFS_FLASH = target/chip/soc/mram
endif

APP_CFLAGS += -DFLASH_TYPE=$(FLASH_TYPE) -DUSE_$(FLASH_TYPE) -DUSE_$(RAM_TYPE)
APP_CFLAGS += -DALWAYS_BLOCK_DMA_TRANSFERS

include vars.mk

include $(RULES_DIR)/pmsis_rules.mk