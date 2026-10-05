# macros for overlay definition that automatically grab everything that we need from the folders provided
CODE_BUILD_DIRS += $(BUILD)/
THUMB_HELP := $(BUILD)/thumb_help.o
LINKED_OUTPUTS = build/linked.o

INDIVIDUAL := individual
OVERLAYS := $(filter-out $(INDIVIDUAL) $(shell cd $(C_SUBDIR); ls *.*),$(shell cd $(C_SUBDIR); ls))

INDIVIDUAL_OVERLAYS = $(basename $(notdir $(wildcard $(C_SUBDIR)/$(INDIVIDUAL)/*.c)))

# everything is expanded because it was not working for me otherwise
# this is aggressively defined but works.  in order to add a new overlay, you just have to add to the top now.
define OVERLAY_DEFINE

$1_LINK = $(BUILD)/$1_linked.o
$1_OUTPUT = $(BUILD)/output_$1.bin
OVERLAY_OUTPUTS += $(BUILD)/output_$1.bin
LINKED_OUTPUTS += $(BUILD)/$1_linked.o
CODE_BUILD_DIRS += $(BUILD)/$1/

ALL_C_SRCS += $(wildcard $(C_SUBDIR)/$1/*.c)
ALL_ASM_SRCS += $(wildcard $(ASM_SUBDIR)/$1/*.s)


$1_OBJS = $(patsubst $(C_SUBDIR)/%.c,$(BUILD)/%.o,$(wildcard $(C_SUBDIR)/$1/*.c)) $(patsubst $(ASM_SUBDIR)/%.s,$(BUILD)/%.o,$(wildcard $(ASM_SUBDIR)/$1/*.s))

$(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.s &: $(BUILD)/rom_gen.ld scripts/split_rom_symbols.py $$($1_OBJS) $(THUMB_HELP)
	$(PYTHON) scripts/split_rom_symbols.py $(BUILD)/rom_gen.ld $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.s $$($1_OBJS) $(THUMB_HELP)

$(BUILD)/$1_linked.o:$(patsubst $(C_SUBDIR)/%.c,$(BUILD)/%.o,$(wildcard $(C_SUBDIR)/$1/*.c)) $(patsubst $(ASM_SUBDIR)/%.s,$(BUILD)/%.o,$(wildcard $(ASM_SUBDIR)/$1/*.s)) $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.o
	$(LD) --use-blx $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.o -T $(C_SUBDIR)/$1/linker.ld -o $(BUILD)/$1_linked.o $(patsubst $(C_SUBDIR)/%.c,$(BUILD)/%.o,$(wildcard $(C_SUBDIR)/$1/*.c)) $(patsubst $(ASM_SUBDIR)/%.s,$(BUILD)/%.o,$(wildcard $(ASM_SUBDIR)/$1/*.s)) $(THUMB_HELP)

$(BUILD)/output_$1.bin:$(BUILD)/$1_linked.o
	$(OBJCOPY) -O binary $(BUILD)/$1_linked.o $(BUILD)/output_$1.bin

endef

ifneq (1,$(NOSCAN))
$(foreach overlay, $(OVERLAYS), $(eval $(call OVERLAY_DEFINE,$(overlay))))
endif


CODE_BUILD_DIRS += $(BUILD)/$(INDIVIDUAL)/

$(BUILD)/rom_gen_battle.ld:$(battle_LINK) $(battle_OUTPUT) $(BUILD)/rom_gen.ld
	cp $(BUILD)/rom_gen.ld $(BUILD)/rom_gen_battle.ld
	$(PYTHON) scripts/generate_ld.py $(BUILD)/rom_gen_battle.ld $(battle_LINK)


define INDIVIDUAL_OVERLAY_DEFINE

LDFLAGS_$1 = --use-blx $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.o -T $(C_SUBDIR)/$(INDIVIDUAL)/$1.ld

$1_LINK = $(BUILD)/$1_linked.o
$1_OUTPUT = $(BUILD)/output_$1.bin
OVERLAY_OUTPUTS += $(BUILD)/output_$1.bin
LINKED_OUTPUTS += $(BUILD)/$1_linked.o

ALL_C_SRCS += $(C_SUBDIR)/$(INDIVIDUAL)/$1.c

$(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.s &: $(BUILD)/rom_gen_battle.ld scripts/split_rom_symbols.py $(BUILD)/$(INDIVIDUAL)/$1.o $(THUMB_HELP)
	$(PYTHON) scripts/split_rom_symbols.py $(BUILD)/rom_gen_battle.ld $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.s $(BUILD)/$(INDIVIDUAL)/$1.o $(THUMB_HELP)

$(BUILD)/$1_linked.o:$(patsubst $(C_SUBDIR)/%.c,$(BUILD)/%.o,$(C_SUBDIR)/$(INDIVIDUAL)/$1.c) $(THUMB_HELP) $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.o
	$(LD) --use-blx $(BUILD)/$1_syms_arm.ld $(BUILD)/$1_syms_thumb.o -T $(C_SUBDIR)/$(INDIVIDUAL)/linker/$1.ld -o $(BUILD)/$1_linked.o $(patsubst $(C_SUBDIR)/%.c,$(BUILD)/%.o,$(C_SUBDIR)/$(INDIVIDUAL)/$1.c) $(THUMB_HELP)

$(BUILD)/output_$1.bin:$(BUILD)/$1_linked.o
	$(OBJCOPY) -O binary $(BUILD)/$1_linked.o $(BUILD)/output_$1.bin

endef

ifneq (1,$(NOSCAN))
$(foreach overlay, $(INDIVIDUAL_OVERLAYS), $(eval $(call INDIVIDUAL_OVERLAY_DEFINE,$(overlay))))
endif
