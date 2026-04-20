# Makefile for Everywhere OS Kernel -- multi-module version

CC      = gcc
LD      = ld
NASM    = nasm

CFLAGS  = -c -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
          -m32 -Wall -Wextra \
          -I./base/ntos/inc \
          -I./shell/explorer \
          -I./base/fs/evryfs

LDFLAGS = -m elf_i386 -T kernel.ld
ASFLAGS = -f elf32

BUILD = build
ISO   = iso
DISK_IMG = $(BUILD)/disk.img

ENTRY_SRC = entry.asm
ENTRY_OBJ = $(BUILD)/entry.o

# Kernel core (base\ntos\ke)
NTOS_SRC = base/ntos/ke/io.c \
           base/ntos/ke/video.c \
           base/ntos/ke/font.c \
           base/ntos/ke/mouse.c \
           base/ntos/ke/keyboard.c \
           base/ntos/ke/window.c

# File System (base\fs\evryfs)
FS_SRC = base/fs/evryfs/ata.c \
         base/fs/evryfs/evryfs.c
HAL_SRC = base/hals/halx86/halinit.c
HAL_ASM_SRC = base/hals/halx86/irq12.asm
HAL_ASM_OBJ = $(BUILD)/base/hals/halx86/irq12.o

# Shell / Explorer (userspace)
SHELL_SRC = shell/explorer/desktop.c \
            shell/explorer/taskbar.c \
            shell/explorer/shell.c \
            shell/explorer/notes.c \
            shell/explorer/snake.c \
            shell/explorer/input.c \
            shell/explorer/files.c

# Main entry
MAIN_SRC = kernel.c

ALL_C_SRC = $(NTOS_SRC) $(HAL_SRC) $(FS_SRC) $(SHELL_SRC) $(MAIN_SRC)
ALL_C_OBJ = $(patsubst %.c,$(BUILD)/%.o,$(ALL_C_SRC))

KERNEL_ELF = $(BUILD)/kernel.elf
OS_ISO     = $(BUILD)/os.iso

.PHONY: all clean run

$(shell mkdir -p $(BUILD))
$(shell mkdir -p $(BUILD)/base/ntos/ke)
$(shell mkdir -p $(BUILD)/base/hals/halx86)
$(shell mkdir -p $(BUILD)/base/fs/evryfs)
$(shell mkdir -p $(BUILD)/shell/explorer)
$(shell mkdir -p $(ISO)/boot/grub)

all: $(OS_ISO)

$(ENTRY_OBJ): $(ENTRY_SRC)
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(HAL_ASM_OBJ): $(HAL_ASM_SRC)
	$(NASM) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(ENTRY_OBJ) $(ALL_C_OBJ) $(HAL_ASM_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

$(ISO)/boot/kernel.elf: $(KERNEL_ELF)
	cp $< $@

$(ISO)/boot/grub/grub.cfg:
	echo 'set timeout=0' > $@
	echo 'set default=0' >> $@
	echo '' >> $@
	echo 'menuentry "Everywhere OS" {' >> $@
	echo '    multiboot /boot/kernel.elf' >> $@
	echo '    boot' >> $@
	echo '}' >> $@

$(OS_ISO): $(ISO)/boot/kernel.elf $(ISO)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO)

$(DISK_IMG):
	qemu-img create -f raw $@ 1M

run: $(OS_ISO) $(DISK_IMG)
	qemu-system-i386 -cdrom $(OS_ISO) -hda $(DISK_IMG) -full-screen

clean:
	rm -rf $(BUILD) $(ISO)