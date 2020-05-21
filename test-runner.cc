/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static bool fd_readall(int fd, std::vector<uint8_t> &buffer) {
  while (true) {
    uint8_t read_buffer[1024];
    auto n_read = read(fd, read_buffer, 1024);
    if (n_read < 0) {
      printf("Failed to read\n");
      return false;
    }
    if (n_read == 0)
      return true;
    buffer.insert(buffer.end(), read_buffer, read_buffer + n_read);
  }
}

static bool file_readall(std::string &pathname, std::vector<uint8_t> &buffer) {
  auto fd = open(pathname.c_str(), O_RDONLY);
  if (fd < 0) {
    printf("Failed to open %s\n", pathname.c_str());
    return false;
  }

  return fd_readall(fd, buffer);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: test-runner <path-to-elf> <file>\n");
    return EXIT_FAILURE;
  }
  const char *elf_path = argv[1];
  const char *source_path = argv[2];
  auto expected_stdout_path = std::string(source_path) + ".stdout";

  // Make pipe to capture stdout
  int stdout_pipe[2];
  if (pipe(stdout_pipe) < 0) {
    printf("Failed to make pipe\n");
    return EXIT_FAILURE;
  }

  // Run Elf with the given file
  pid_t pid = fork();
  if (pid == 0) {
    close(stdout_pipe[0]);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    execl(elf_path, elf_path, "run", source_path, nullptr);
    exit(EXIT_FAILURE);
  }
  close(stdout_pipe[1]);

  // Read result from Elf
  std::vector<uint8_t> stdout_data;
  if (!fd_readall(stdout_pipe[0], stdout_data)) {
    printf("Failed to read Elf output\n");
    return EXIT_FAILURE;
  }

  // Wait for Elf to complete
  int status;
  if (waitpid(pid, &status, 0) < 0) {
    printf("Failed to wait for Elf to exit\n");
    return EXIT_FAILURE;
  }
  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status != 0) {
      printf("Elf exited with status %d\n", exit_status);
      return EXIT_FAILURE;
    }
  } else if (WIFSIGNALED(status)) {
    int term_signal = WTERMSIG(status);
    printf("Elf terminated with signal %d\n", term_signal);
    return EXIT_FAILURE;
  } else
    return EXIT_FAILURE;

  // Get expected result
  std::vector<uint8_t> expected_stdout_data;
  file_readall(expected_stdout_path, expected_stdout_data);

  if (stdout_data != expected_stdout_data) {
    printf("stdout does not match expected\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
