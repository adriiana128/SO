#include "common.h"

ssize_t readln_v1(int fd, char* line, size_t size) {
  char *buffer = line;
  ssize_t bytes_read;

  while((buffer - line) < size && (bytes_read = read(fd, buffer, 1)) > 0) {
    if (*buffer++ == '\n') {
      return (buffer - line);
    }
  }

  if (bytes_read <= 0) {
    return -1;
  }

  if (buffer - line == size) {
    char c;

    while(read(fd, &c, 1) > 0 && c != '\n') {
      ;
    }
  }

  return (buffer - line);
}

int equals(char* str1, char* str2) {
  return strcmp(str1, str2) == 0;
}

int command_with_integer_args(char* command) {
  return equals(command, "tempo-inactividade") || equals(command, "tempo-execucao") || equals(command, "terminar") || equals(command, "output");
}

int command_with_args(char* command) {
  return equals(command, "executar") || command_with_integer_args(command);
}

