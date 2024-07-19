PROJECT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
OPENCM3_DIR ?= $(PROJECT_DIR)vendor/libopencm3
NAME	   ?= spi-flash-demo
LDSCRIPT   ?= $(PROJECT_DIR)/memory.ld

ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
endif

# gcc toolchain
PREFIX   ?= arm-none-eabi-
CC       := $(PREFIX)gcc
LD       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy
OBJDUMP  := $(PREFIX)objdump
AR       := $(PREFIX)ar
AS       := $(PREFIX)as

OPTFLAGS ?= -O3
DBGFLAGS ?= -g -DNDEBUG
CPUFLAGS ?= -mcpu=cortex-m3 -mthumb
FPUFLAGS ?= -msoft-float

CFLAGS   += $(OPTFLAGS) \
            $(DBGFLAGS) \
            -std=gnu11 \
            -W \
            -Wall \
            -Wextra \
            -Wimplicit-function-declaration \
            -Wredundant-decls \
            -Wstrict-prototypes \
            -Wundef \
            -Wshadow \
            -Wpointer-arith \
            -Wformat \
            -Wreturn-type \
            -Wsign-compare \
            -Wmultichar \
            -Wformat-nonliteral \
            -Winit-self \
            -Wuninitialized \
            -Wformat-security \
            -Wno-missing-braces \
            -Werror \
            -fno-common \
            -fno-exceptions \
            -fvisibility=internal \
            -ffunction-sections \
            -fdata-sections \
            $(CPUFLAGS) \
            $(FPUFLAGS) \
            -DSTM32F2 \
            -I$(OPENCM3_DIR)/include \
            -I$(PROJECT_DIR)

LDFLAGS  += -L$(PROJECT_DIR) \
            $(DBGFLAGS) \
            $(CPUFLAGS) \
            $(FPUFLAGS) \
			--static \
            -Wl,--start-group \
            -lc \
            -lgcc \
            -lnosys \
            -Wl,--end-group \
            -nostartfiles \
			-Wl,--gc-sections \
			-Wl,--print-memory-usage
all: $(NAME).bin


opencm3:
	$(Q)$(MAKE) -C $(OPENCM3_DIR) lib/stm32/f2

LDFLAGS += -T $(LDSCRIPT)
LDFLAGS += -L$(OPENCM3_DIR)/lib -lopencm3_stm32f2


$(NAME).bin: $(NAME).elf
	@printf "  OBJCOPY $@\n"
	$(Q)$(OBJCOPY) -O binary $< $@

OBJS ?= main.o \
		spi_flash.o \
		startup.o

$(NAME).elf: opencm3 $(OBJS) 
	@printf "  LD      $@\n"
	$(Q)$(LD) $(OBJS) $(LDFLAGS) -o $@

%.o: %.S
	@printf "  AS      $@\n"
	$(Q)$(CC) $(CPUFLAGS) -o $@ -c $<
%.o: %.c
	@printf "  CC      $@\n"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.d: %.c
	@printf "  DEP     $@\n"
	$(Q)$(CC) $(CFLAGS) -MM -MP -MG -o $@ $<

flash: $(NAME).bin
	openocd -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f2x.cfg -c "init; reset halt; flash write_image erase spi-flash-demo.bin 0x8000000; exit"


clean::
	rm -f $(OBJS)
	rm -f *.a
	rm -f *.bin
	rm -f *.d
	rm -f *.elf
	make -C $(OPENCM3_DIR) clean

-include $(OBJS:.o=.d)

.PHONY: all