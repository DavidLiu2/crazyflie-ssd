# Makefile
# Responsibility: Build modular app + generated DORY network for GAP SDK targets.

io ?= uart
PMSIS_OS ?= freertos
BOARD_NAME ?= ai_deck
USE_PMSIS_BSP = 1

CORE ?= 1
APP_DEBUG ?= 1
APP_ENABLE_CPX_APP_PACKET_TX ?= 0
APP_GENERATED_NETWORK_VERBOSE ?= 0
APP_ENABLE_LTO ?= 0

APP = main
APP_SRCS := main.c
APP_SRCS += $(wildcard src/*.c)
APP_SRCS += $(wildcard lib/cpx/src/*.c)
APP_SRCS += $(wildcard generated/*.c)
APP_SRCS += $(filter-out generated/src/main.c, $(wildcard generated/src/*.c))

# -O2 with -fno-indirect-inlining is just as fast as -O3 and reduces code size considerably
# by not inlining of small functions in the management code
APP_CFLAGS += -DNUM_CORES=$(CORE) -I. -Iinc -Igenerated -Igenerated/inc -Ilib/cpx/inc -O2 -fno-indirect-inlining -w
APP_LDFLAGS += -lm -Wl,--print-memory-usage
APP_CFLAGS += -D__PMSIS__ -DHIMAX

ifeq ($(APP_ENABLE_LTO),1)
APP_CFLAGS += -flto
APP_LDFLAGS += -flto
endif

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
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1
APP_CFLAGS += -DAPP_DEBUG=$(APP_DEBUG)
APP_CFLAGS += -DAPP_ENABLE_CPX_APP_PACKET_TX=$(APP_ENABLE_CPX_APP_PACKET_TX)
APP_CFLAGS += -DAPP_GENERATED_NETWORK_VERBOSE=$(APP_GENERATED_NETWORK_VERBOSE)
APP_CFLAGS += -DAPP_ENABLE_LTO=$(APP_ENABLE_LTO)

include vars.mk

include $(RULES_DIR)/pmsis_rules.mk
