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

    char *data_ = mmap (NULL, data_length_, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data_ == MAP_FAILED) {
        close (fd);
        printf ("Failed to map file: %s\n", strerror (errno));
        return -1;
    }

    *data = data_;
    *data_length = data_length_;
    return fd;
}

static void
munmap_file (int fd, char *data, size_t data_length)
{
    munmap (data, data_length);
    close (fd);
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

    elf_run (main_function);

    munmap_file (fd, data, data_length);

    return 0;
}

static int
compile_elf_source (const char *filename)
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

    for (int i = 0; main_function->body[i] != NULL; i++) {
        Operation *op = main_function->body[i];
        char *op_text = operation_to_string (op);
        printf ("%s\n", op_text);
    }

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

    if (strcmp (command, "run") == 0) {
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
