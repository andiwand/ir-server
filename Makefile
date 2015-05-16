# name for the target project
TARGET		:= app

# relative to the project directories
SRC_BASE	:= src
BUILD_BASE	:= obj
FW_BASE		:= bin

# base directory for the compiler
XTENSA_TOOLS_ROOT	:= /opt/espressif/crosstool-NG/builds/xtensa-lx106-elf/bin

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	:= /opt/espressif/sdk

# which modules (subdirectories) of the project to include in compiling
MODULES			:= debug driver network user
EXTRA_INCDIR	:= include src/user

# libraries used in this project, mainly provided by the SDK
LIBS		:= c gcc hal phy pp net80211 lwip wpa upgrade main

# compiler flags using during compilation of source files
CFLAGS		:= -Os -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH

# linker flags used to generate the main object file
LDFLAGS		:= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the above linkier step
LD_SCRIPT_STD	:= eagle.app.v6.ld
LD_SCRIPT_OTA1	:= eagle.app.v6.old.512.app1.ld
LD_SCRIPT_OTA2	:= eagle.app.v6.old.512.app2.ld

# various paths from the SDK used in this project
SDK_LIBDIR	:= lib
SDK_LDDIR	:= ld
SDK_INCDIR	:= include
SDK_BINDIR	:= bin

# path to the esptool used to generate the final binaries
# it assumed to have it somwhere in the main SDK directoy tree
FW_TOOL		:= esptool
FLASH_TOOL	:= esptool.py
JOIN_TOOL	:= gen_flashbin.py
ROM_TOOL	:= gen_anybin.py

# relativ path of the tool directory in the sdk
FW_TOOLDIR	:= tools

# we create six different files for uploading into the flash
# these are the names and options to generate them
FW_FILE_STD1		:= std_0x00000
FW_FILE_STD1_ARGS	 = -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

FW_FILE_STD2		:= std_0x40000
FW_FILE_STD2_ARGS	 = -es .irom0.text $@ -ec

FW_FILE_OTA1	:= ota_user1_full
FW_FILE_OTA2	:= ota_user2_full

FW_FILE_STD	:= std_full
FW_FILE_OTA	:= ota_full

#Intermediate files for user1.bin and user2.bin
FW_FILE_OTA1_TMP1		:= ota_user1_0x01000
FW_FILE_OTA1_TMP1_ARGS	 = -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

FW_FILE_OTA1_TMP2		:= ota_user1_0x11000
FW_FILE_OTA1_TMP2_ARGS	 = -es .irom0.text $@ -ec

FW_FILE_OTA2_TMP1		:= ota_user2_0x41000
FW_FILE_OTA2_TMP1_ARGS	 = -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

FW_FILE_OTA2_TMP2		:= ota_user2_0x51000
FW_FILE_OTA2_TMP2_ARGS	 = -es .irom0.text $@ -ec

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc

####
#### no user configurable options below here
####
FW_TOOLDIR	:= $(addprefix $(SDK_BASE)/,$(FW_TOOLDIR))
FW_TOOL		:= $(addprefix $(FW_TOOLDIR)/,$(FW_TOOL))
FLASH_TOOL	:= $(addprefix $(FW_TOOLDIR)/,$(FLASH_TOOL))
JOIN_TOOL	:= $(addprefix $(FW_TOOLDIR)/,$(JOIN_TOOL))
ROM_TOOL	:= $(addprefix $(FW_TOOLDIR)/,$(ROM_TOOL))

SRC_DIR		:= $(addprefix $(SRC_BASE)/,$(MODULES))
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
SDK_BINDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_BINDIR))

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ			:= $(patsubst $(SRC_BASE)/%.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET).a)

TARGET_STD_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET)_std.out)
TARGET_OTA1_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET)_ota_user1.out)
TARGET_OTA2_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET)_ota_user2.out)

LD_SCRIPT_STD	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT_STD))
LD_SCRIPT_OTA1	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT_OTA1))
LD_SCRIPT_OTA2	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT_OTA2))

INCDIR			:= $(addprefix -I,$(SRC_BASE))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))

FW_FILE_STD1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_STD1).bin)
FW_FILE_STD2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_STD2).bin)
FW_FILE_OTA1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA1).bin)
FW_FILE_OTA2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA2).bin)
FW_FILE_STD		:= $(addprefix $(FW_BASE)/,$(FW_FILE_STD).bin)
FW_FILE_OTA		:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA).bin)

FW_FILE_OTA1_TMP1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA1_TMP1).bin)
FW_FILE_OTA1_TMP2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA1_TMP2).bin)
FW_FILE_OTA2_TMP1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA2_TMP1).bin)
FW_FILE_OTA2_TMP2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_OTA2_TMP2).bin)


vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	@echo "CC $$<"
	@$(CC) $(INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@
endef



.PHONY: all checkdirs clean

all: checkdirs $(TARGET_STD_OUT) $(TARGET_OTA1_OUT) $(TARGET_OTA2_OUT) $(FW_FILE_STD1) $(FW_FILE_STD2) $(FW_FILE_OTA1) $(FW_FILE_OTA2) $(FW_FILE_STD) $(FW_FILE_OTA)


$(FW_FILE_STD): $(FW_FILE_STD1) $(FW_FILE_STD2)
	@echo "FW $@"
	@$(ROM_TOOL) $@ 0x80000 0x00000 $(FW_FILE_STD1) 0x40000 $(FW_FILE_STD2) 0x7C000 $(SDK_BINDIR)/esp_init_data_default.bin

$(FW_FILE_OTA): $(FW_FILE_OTA1) $(FW_FILE_OTA2)
	@echo "FW $@"
	@$(ROM_TOOL) $@ 0x80000 0x00000 $(SDK_BINDIR)/boot_v1.2.bin 0x01000 $(FW_FILE_OTA1) 0x41000 $(FW_FILE_OTA2)

$(FW_FILE_STD1): $(TARGET_STD_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_STD_OUT) $(FW_FILE_STD1_ARGS)

$(FW_FILE_STD2): $(TARGET_STD_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_STD_OUT) $(FW_FILE_STD2_ARGS)

$(FW_FILE_OTA1): $(FW_FILE_OTA1_TMP1) $(FW_FILE_OTA1_TMP2)
	@echo "FW $@"
	@$(JOIN_TOOL) $(FW_FILE_OTA1_TMP1) $(FW_FILE_OTA1_TMP2)
	@mv eagle.app.flash.bin $@

$(FW_FILE_OTA2): $(FW_FILE_OTA2_TMP1) $(FW_FILE_OTA2_TMP2)
	@echo "FW $@"
	@$(JOIN_TOOL) $(FW_FILE_OTA2_TMP1) $(FW_FILE_OTA2_TMP2)
	@mv eagle.app.flash.bin $@


$(FW_FILE_OTA1_TMP1): $(TARGET_OTA1_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_OTA1_OUT) $(FW_FILE_OTA1_TMP1_ARGS)

$(FW_FILE_OTA1_TMP2): $(TARGET_OTA1_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_OTA1_OUT) $(FW_FILE_OTA1_TMP2_ARGS)

$(FW_FILE_OTA2_TMP1): $(TARGET_OTA2_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_OTA2_OUT) $(FW_FILE_OTA2_TMP1_ARGS)

$(FW_FILE_OTA2_TMP2): $(TARGET_OTA2_OUT)
	@echo "FW $@"
	@$(FW_TOOL) -eo $(TARGET_OTA2_OUT) $(FW_FILE_OTA2_TMP2_ARGS)


$(TARGET_STD_OUT): $(APP_AR)
	@echo "LD $@"
	@$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT_STD) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(TARGET_OTA1_OUT): $(APP_AR)
	@echo "LD $@"
	@$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT_OTA1) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(TARGET_OTA2_OUT): $(APP_AR)
	@echo "LD $@"
	@$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT_OTA2) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	@echo "AR $@"
	@$(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	@mkdir -p $@

$(FW_BASE):
	@mkdir -p $@

flash-std: all
	@$(FLASH_TOOL) --port /dev/ttyUSB0 write_flash 0x00000 $(FW_FILE_STD1) 0x40000 $(FW_FILE_STD2)

flash-ota: all
	@$(FLASH_TOOL) --port /dev/ttyUSB0 write_flash 0x00000 $(SDK_BINDIR)/boot_v1.2.bin 0x01000 $(FW_FILE_OTA1) 0x7E000 $(SDK_BINDIR)/blank.bin 

update: all
	@pv < $(FW_FILE_OTA) | netcat -q1 ${IP} 4444

clean:
	@rm -f $(APP_AR)
	@rm -f $(TARGET_OUT)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_BASE)
	@rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
