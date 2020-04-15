/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "elf-parser.h"
#include "elf-runner.h"
#include "utils.h"

static int
mmap_file (const char *filename, char **data, size_t *data_length)
{
    int fd = open (filename, O_RDONLY);
    if (fd < 0) {
        printf ("Failed to open file \"%s\": %s\n", filename, strerror (errno));
        return -1;
    }

    struct stat file_info;
    if (fstat (fd, &file_info) < 0) {
        close (fd);
        printf ("Failed to get information: %s\n", strerror (errno));
        return -1;
    }
    size_t data_length_ = file_info.st_size;

    char *data_ = NULL;
    if (data_length_ > 0) {
        data_ = mmap (NULL, data_length_, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data_ == MAP_FAILED) {
            close (fd);
            printf ("Failed to map file: %s\n", strerror (errno));
            return -1;
        }
    }

    *data = data_;
    *data_length = data_length_;
    return fd;
}

static void
munmap_file (int fd, char *data, size_t data_length)
{
    if (data_length > 0)
        munmap (data, data_length);
    close (fd);
}

static int
run_tutorial (void)
{
    autofree_str source_name = str_new ("tutorial.elf");
    int index = 0;
    int fd = 0;
    while (true) {
        fd = open (source_name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd > 0)
            break;

        if (errno != EEXIST) {
            printf ("Failed to make %s: %s", source_name, strerror (errno));
            return 1;
        }

        str_free (&source_name);
        index++;
        source_name = str_printf ("tutorial-%d.elf", index);
    }

    printf ("Welcome to Elf. Elf is a language designed to help you learn how computers work.\n"
            "\n"
            "Let's get started! I've make your first Elf program in the file '%s'. If will show you the basic concepts of the language.\n"
            "\n"
            "Open this with your favourite editor to continue.\n", source_name);

    const char *hello_world =
        "# Write something to the outside world\n"
        "print ('Hello world!')\n"
        "\n"
        "# Variables can store values. Integers can be 8, 16, 32 or 64 bit, signed or unsigned \n"
        "uint8 the_meaning_of_life = 6 * 7\n"
        "\n"
        "# Strings are stored in UTF-8 encoding\n"
        "utf8 name = 'Zelda'\n"
        "\n"
        "# Functions allow you to re-use code\n"
        "uint32 add (uint32 a, uint32 b) {\n"
        "  return a + b\n"
        "}\n"
        "uint32 three = add (1, 2)\n"
        "\n"
        "# Conditionals allow you to run code if something is true\n"
        "if three != 3 {\n"
        "  print ('uh oh...')\n"
        "}\n"
        "\n"
        "# Loops allow you to repeat code\n"
        "int8 countdown = 10\n"
        "while countdown > 0 {\n"
        "    countdown = countdown - 1\n"
        "}\n";
    write (fd, hello_world, strlen (hello_world));
    close (fd);

    return 0;
}

static int
run_elf_source (const char *filename)
{
    char *data;
    size_t data_length;
    int fd = mmap_file (filename, &data, &data_length);
    if (fd < 0)
        return 1;

    OperationFunctionDefinition *main_function = elf_parse (data, data_length);
    if (main_function == NULL) {
        munmap_file (fd, data, data_length);
        return 1;
    }

    elf_run (data, main_function);

    munmap_file (fd, data, data_length);

    return 0;
}

static void
write_binary (int fd, uint8_t *text, size_t text_length, uint8_t *rodata, size_t rodata_length)
{
    const char *section_names[] = { "", ".shrtrtab", ".text", ".rodata", NULL };
    size_t shrtrtab_length = 0;
    for (int i = 0; section_names[i] != NULL; i++)
        shrtrtab_length += strlen (section_names[i]) + 1;

    char padding[16] = { 0 };
    size_t text_padding_length = text_length % 16;
    if (text_padding_length > 0)
        text_padding_length = 16 - text_padding_length;
    size_t rodata_padding_length = rodata_length % 16;
    if (rodata_padding_length > 0)
        rodata_padding_length = 16 - rodata_padding_length;
    size_t shrtrtab_padding_length = shrtrtab_length % 16;
    if (shrtrtab_padding_length > 0)
        shrtrtab_padding_length = 16 - shrtrtab_padding_length;

    Elf64_Ehdr elf_header = { 0 };
    elf_header.e_ident[EI_MAG0] = ELFMAG0;
    elf_header.e_ident[EI_MAG1] = ELFMAG1;
    elf_header.e_ident[EI_MAG2] = ELFMAG2;
    elf_header.e_ident[EI_MAG3] = ELFMAG3;
    elf_header.e_ident[EI_CLASS] = ELFCLASS64;
    elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EV_CURRENT;
    elf_header.e_type = ET_EXEC;
    elf_header.e_machine = EM_X86_64;
    elf_header.e_version = EV_CURRENT;
    elf_header.e_entry = 0x8000000 + sizeof (Elf64_Ehdr) + sizeof (Elf64_Phdr);
    elf_header.e_phoff = sizeof (elf_header);
    elf_header.e_shoff = sizeof (Elf64_Ehdr) + sizeof (Elf64_Phdr) + text_length + text_padding_length + rodata_length + rodata_padding_length + shrtrtab_length + shrtrtab_padding_length;
    elf_header.e_ehsize = sizeof (Elf64_Ehdr);
    elf_header.e_phentsize = sizeof (Elf64_Phdr);
    elf_header.e_phnum = 1;
    elf_header.e_shentsize = sizeof (Elf64_Shdr);
    elf_header.e_shnum = 4; // Matches length of section_names
    elf_header.e_shstrndx = 3; // ".shrtrtab" from section header table

    ssize_t n_written = 0;
    n_written += write (fd, &elf_header, sizeof (elf_header));

    Elf64_Phdr program_header = { 0 };
    program_header.p_type = PT_LOAD;
    program_header.p_flags = PF_R | PF_X;
    program_header.p_offset = 0;
    program_header.p_vaddr = 0x8000000;
    program_header.p_paddr = 0x8000000;
    program_header.p_filesz = sizeof (Elf64_Ehdr) + sizeof (Elf64_Phdr) + text_length + text_padding_length + rodata_length + rodata_padding_length;
    program_header.p_memsz = program_header.p_filesz;

    n_written += write (fd, &program_header, sizeof (program_header));

    ssize_t text_offset = n_written;
    n_written += write (fd, text, text_length);
    n_written += write (fd, padding, text_padding_length);

    ssize_t rodata_offset = n_written;
    n_written += write (fd, rodata, rodata_length);
    n_written += write (fd, padding, rodata_padding_length);

    ssize_t shrtrtab_offset = n_written;
    ssize_t null_name_offset = n_written - shrtrtab_offset;
    n_written += write (fd, section_names[0], strlen (section_names[0]) + 1);
    ssize_t shrtrtab_name_offset = n_written - shrtrtab_offset;
    n_written += write (fd, section_names[1], strlen (section_names[1]) + 1);
    ssize_t text_name_offset = n_written - shrtrtab_offset;
    n_written += write (fd, section_names[2], strlen (section_names[2]) + 1);
    ssize_t rodata_name_offset = n_written - shrtrtab_offset;
    n_written += write (fd, section_names[3], strlen (section_names[3]) + 1);
    n_written += write (fd, padding, shrtrtab_padding_length);

    Elf64_Shdr null_section_header = { 0 };
    null_section_header.sh_name = null_name_offset;
    null_section_header.sh_type = SHT_NULL;
    n_written += write (fd, &null_section_header, sizeof (null_section_header));

    Elf64_Shdr text_section_header = { 0 };
    text_section_header.sh_name = text_name_offset;
    text_section_header.sh_type = SHT_PROGBITS;
    text_section_header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    text_section_header.sh_addr = 0x8000000 + text_offset;
    text_section_header.sh_offset = text_offset;
    text_section_header.sh_size = text_length;
    n_written += write (fd, &text_section_header, sizeof (text_section_header));

    Elf64_Shdr rodata_section_header = { 0 };
    rodata_section_header.sh_name = rodata_name_offset;
    rodata_section_header.sh_type = SHT_PROGBITS;
    rodata_section_header.sh_flags = SHF_ALLOC;
    rodata_section_header.sh_addr = 0x80000000 + rodata_offset;
    rodata_section_header.sh_offset = rodata_offset;
    rodata_section_header.sh_size = rodata_length;
    n_written += write (fd, &rodata_section_header, sizeof (rodata_section_header));

    Elf64_Shdr shrtrtab_section_header = { 0 };
    shrtrtab_section_header.sh_name = shrtrtab_name_offset;
    shrtrtab_section_header.sh_type = SHT_STRTAB;
    shrtrtab_section_header.sh_offset = shrtrtab_offset;
    shrtrtab_section_header.sh_size = shrtrtab_length;
    n_written += write (fd, &shrtrtab_section_header, sizeof (shrtrtab_section_header));
}

static int
compile_elf_source (const char *filename)
{
    if (!str_has_suffix (filename, ".elf")) {
        printf ("Elf program doesn't have standard extension, can't determine name of binary to write\n");
        return 1;
    }
    autofree_str binary_name = str_slice (filename, 0, -4);

    char *data;
    size_t data_length;
    int fd = mmap_file (filename, &data, &data_length);
    if (fd < 0)
        return 1;

    OperationFunctionDefinition *main_function = elf_parse (data, data_length);
    if (main_function == NULL) {
        munmap_file (fd, data, data_length);
        return 1;
    }

    int binary_fd = open (binary_name, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (binary_fd < 0) {
        printf ("Failed to open '%s' to write program to\n", binary_name);
        return 1;
    }

    write_binary (binary_fd, NULL, 0, NULL, 0);

    close (binary_fd);

    printf ("%s compiled to '%s', run with:\n", filename, binary_name);
    printf ("$ ./%s\n", binary_name);

    operation_free ((Operation *) main_function);

    munmap_file (fd, data, data_length);

    return 0;
}

int
main (int argc, char **argv)
{
    const char *command = "help";
    if (argc > 1)
        command = argv[1];

    if (strcmp (command, "tutorial") == 0) {
        return run_tutorial ();
    }
    else if (strcmp (command, "run") == 0) {
        if (argc < 3) {
            printf ("Need file to run, run elf help for more information\n");
            return 1;
        }
        const char *filename = argv[2];

        return run_elf_source (filename);
    }
    else if (strcmp (command, "compile") == 0) {
        if (argc < 3) {
            printf ("Need file to compile, run elf help for more information\n");
            return 1;
        }
        const char *filename = argv[2];

        return compile_elf_source (filename);
    }
    else if (strcmp (command, "version") == 0) {
        printf ("0\n");
        return 0;
    }
    else if (strcmp (command, "help") == 0) {
        printf ("Elf is a programming languge designed for teching how memory works.\n"
                "\n"
                "Usage:\n"
                "  elf tutorial        - Get an introduction to Elf\n"
                "  elf run <file>      - Run an elf program\n"
                "  elf compile <file>  - Compile an elf program\n"
                "  elf version         - Show the version of the Elf tool\n"
                "  elf help            - Show help information\n");
        return 0;
    }
    else {
        printf ("Unknown command \"%s\", run elf to see supported commands.\n", command);
        return 1;
    }

    return 0;
}
