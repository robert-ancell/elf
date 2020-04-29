/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

int
main (int argc, char **argv)
{
    if (argc != 3) {
        printf ("Usage: test-runner <path-to-elf> <file>\n");
        return EXIT_FAILURE;
    }
    const char *elf_path = argv[1];
    const char *source_path = argv[2];
    autofree_str expected_stdout_path = str_printf ("%s.stdout", source_path);

    // Make pipe to capture stdout
    int stdout_pipe[2];
    if (pipe (stdout_pipe) < 0) {
        printf ("Failed to make pipe\n");
        return EXIT_FAILURE;
    }

    // Run Elf with the given file
    pid_t pid = fork ();
    if (pid == 0) {
        close (stdout_pipe[0]);
        dup2 (stdout_pipe[1], STDOUT_FILENO);
        execl (elf_path, elf_path, "run", source_path);
        exit (EXIT_FAILURE);
    }
    close (stdout_pipe[1]);

    // Read result from Elf
    autofree_bytes stdout_data = readall (stdout_pipe[0]);
    if (stdout_data == NULL) {
        printf ("Failed to read Elf output\n");
        return EXIT_FAILURE;
    }

    // Wait for Elf to complete
    int status;
    if (waitpid (pid, &status, 0) < 0) {
        printf ("Failed to wait for Elf to exit\n");
        return EXIT_FAILURE;
    }

    // Get expected result
    autofree_bytes expected_stdout_data = file_readall (expected_stdout_path);

    if (!bytes_equal (stdout_data, expected_stdout_data))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}