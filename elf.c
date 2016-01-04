#include "defines.h"
#include "elf.h"
#include "lib.h"

struct elf_header {
    struct {
        unsigned char magic[4];
        unsigned char class;
        unsigned char format;
        unsigned char version;
        unsigned char abi;
        unsigned char abi_version;
        unsigned char reserve[7];
    } id;
    short type;
    short arch;
    int version;
    int entry_point;
    int program_header_offset;
    int section_header_offset;
    int flags;
    short header_size;
    short program_header_size;
    short program_header_num;
    short section_header_size;
    short section_header_num;
    short section_name_index;
};

struct elf_program_header {
    int type;
    int offset;
    int virtual_addr;
    int physical_addr;
    int file_size;
    int memory_size;
    int flags;
    int align;
};


static int elf_check(struct elf_header *header)
{
    if (memcmp(header->id.magic, "\x7f" "ELF", 4)) {
        return -1;
    }

    if (header->id.class   != 1) return -1; // ELF32
    if (header->id.format  != 1) return -1; // Little endian
    if (header->id.version != 1) return -1; // version 1
    if (header->type       != 2) return -1; // Executable file
    if (header->version    != 1) return -1; // version 1

    // ARM
    if (header->arch != 0x28) return -1;

    return 0;
}

static int elf_load_program(struct elf_header *header)
{
    for (int i = 0; i < header->program_header_num; i++) {
        struct elf_program_header *phdr;
        phdr = (struct elf_program_header *)((char *)header + header->program_header_offset + header->program_header_size * i);

        if (phdr->type != 1) {
            continue;
        }

        putxval(phdr->offset,        6); puts(" ");
        putxval(phdr->virtual_addr,  8); puts(" ");
        putxval(phdr->physical_addr, 8); puts(" ");
        putxval(phdr->file_size,     5); puts(" ");
        putxval(phdr->memory_size,   5); puts(" ");
        putxval(phdr->flags,         2); puts(" ");
        putxval(phdr->align,         2); puts("\n");
    }

    return 0;
}

int elf_load(char *buf)
{
    struct elf_header *header = (struct elf_header *)buf;

    if (elf_check(header) < 0) {
        return -1;
    }

    if (elf_load_program(header) < 0) {
        return -1;
    }

    return 0;
}
