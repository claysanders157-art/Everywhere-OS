# Makefile for Everywhere OS Kernel -- grub added

CC      = gcc
LD      = ld
NASM    = nasm

CFLAGS  = -c -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
          -m32 -Wall -Wextra   \
          -I./base/kernel/inc  \
          -I./public/sdk/inc   \
		  -I./base/fs/ext2/inc \
		  -I./base/fs/inc	   \

LDFLAGS = -m elf_i386 -T kernel.ld

ASFLAGS = -f elf32

BUILD = build
ISO   = iso

KERNEL_DIR = base/kernel

ENTRY_SRC = entry.asm
ENTRY_OBJ = $(BUILD)/entry.o

KERNEL_SRCS = \
    $(KERNEL_DIR)/kernel.c \
    $(KERNEL_DIR)/io.c     \
    $(KERNEL_DIR)/video.c  \
    $(KERNEL_DIR)/string.c \
    $(KERNEL_DIR)/fs.c     \
    $(KERNEL_DIR)/shell.c  \
    $(KERNEL_DIR)/snake.c  \
    $(KERNEL_DIR)/box.c

FS_DIR = base/fs
EXT2_DIR = base/fs/ext2

FS_SRCS = \
    $(FS_DIR)/ata.c          \
    $(EXT2_DIR)/ext2.c       \
    $(EXT2_DIR)/ext2_block.c \
    $(EXT2_DIR)/ext2_inode.c \
    $(EXT2_DIR)/ext2_dir.c

FS_OBJS = $(patsubst $(FS_DIR)/%.c,    $(BUILD)/fs_%.o, $(FS_DIR)/ata.c) \
          $(patsubst $(EXT2_DIR)/%.c,   $(BUILD)/ext2_%.o, $(EXT2_DIR)/ext2.c) \
          $(patsubst $(EXT2_DIR)/%.c,   $(BUILD)/ext2_%.o, $(EXT2_DIR)/ext2_block.c) \
          $(patsubst $(EXT2_DIR)/%.c,   $(BUILD)/ext2_%.o, $(EXT2_DIR)/ext2_inode.c) \
          $(patsubst $(EXT2_DIR)/%.c,   $(BUILD)/ext2_%.o, $(EXT2_DIR)/ext2_dir.c)

KERNEL_OBJS = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD)/%.o, $(KERNEL_SRCS))

KERNEL_ELF = $(BUILD)/kernel.elf
OS_ISO     = $(BUILD)/os.iso
DISK_IMG   = $(BUILD)/disk.img

.PHONY: all clean run

$(shell mkdir -p $(BUILD))
$(shell mkdir -p $(ISO)/boot/grub)

all: $(OS_ISO)

$(ENTRY_OBJ): $(ENTRY_SRC)
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fs_%.o: $(FS_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ext2_%.o: $(EXT2_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(KERNEL_ELF): $(ENTRY_OBJ) $(KERNEL_OBJS) $(FS_OBJS)
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

$(DISK_IMG):
	dd if=/dev/zero of=$@ bs=512 count=131072
	mkfs.ext2 -b 1024 $@

$(OS_ISO): $(ISO)/boot/kernel.elf $(ISO)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO)

run: $(OS_ISO) $(DISK_IMG)
	qemu-system-i386 -cdrom $(OS_ISO) -hda $(DISK_IMG)

clean:
	rm -rf $(BUILD)/entry.o $(KERNEL_OBJS) $(FS_OBJS) $(KERNEL_ELF) $(OS_ISO)
	rm -rf $(ISO)