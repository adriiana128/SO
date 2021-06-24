#include "common.h"
#include "argus.h"

int option_with_integer_args(char* option) {
  return equals(option, "-i") || equals(option, "-m") || equals(option, "-t") || equals(option, "-o");
}

int option_with_args(char* option) {
  return equals(option, "-e") || option_with_integer_args(option);
}

char* get_command(char* option) {
  char *command = "";

       if (equals(option, "-i")) { command = "tempo-inactividade"; }
  else if (equals(option, "-m")) { command = "tempo-execucao";     }
  else if (equals(option, "-e")) { command = "executar";           }
  else if (equals(option, "-l")) { command = "listar";             }
  else if (equals(option, "-t")) { command = "terminar";           }
  else if (equals(option, "-r")) { command = "historico";          }
  else if (equals(option, "-h")) { command = "ajuda";              }
  else if (equals(option, "-o")) { command = "output";             }
  else {
    perror("[ERROR] Invalid option! See usage with ajuda (-h)\n");
    exit(1);
  }

  return command;
}

int valid_command(char* command) {
  char *pointer     = strtok(command, " ");
  char *command_aux = "";

  if (equals(command, "tempo-inactividade") || equals(command, "tempo-execucao") ||
      equals(command, "executar")           || equals(command, "listar")         ||
      equals(command, "terminar")           || equals(command, "historico")      ||
      equals(command, "ajuda")              || equals(command, "output") ) {
    command_aux = strdup(pointer);
  }
  else {
    perror("[ERROR] Invalid command! See usage with ajuda (-h)\n");
    return 0;
  }

  if (command_with_args(command_aux)) {
    pointer = strtok(NULL, "");

    if (pointer == NULL) {
      perror("ERROR] Missing argument! See usage with ajuda (-h)\n");
      return 0;
    }

    if (command_with_integer_args(command_aux) && atoi(pointer) <= 0) {
      perror("[ERROR] Argument must be an integer greater than zero! See usage with ajuda (-h)\n");
      return 0;
    }

    if (equals(command, "executar") && (pointer[0] != '"' || pointer[strlen(pointer) - 1] != '"')) {
      perror("[ERROR] Argument must be quoted! See usage with ajuda (-h)\n");
      return 0;
    }
  }

  return 1;
}

void print_prompt() {
  write(STD_OUT, "argus$ ", 7);
}

void print_response(int prompt) {
  if (!fork()) {
    char buffer[1024];
    int bytes_read = 0;
    int server_to_client_fd = open("server_to_client_fifo", O_RDONLY);

    while ((bytes_read = readln_v1(server_to_client_fd, buffer, 1024)) > 0) {
      buffer[bytes_read] = '\0';

      write(STD_OUT, buffer, bytes_read);
    }

    if (prompt) {
      print_prompt();
    }

    _exit(0);
  }
}

int main(int argc, char* argv[]) {
  char buffer_read[1024];
  char buffer_write[1024];
  int bytes_read = 0;
  int bytes_written = 0;

  int client_to_server_fd = open("client_to_server_fifo", O_WRONLY);

  if (argc > 1) {
    char *command = get_command(argv[1]);

    if (option_with_args(argv[1])) {
      // Erro caso a opção tenha argumento e este não esteja presente
      if (argc < 3) {
        perror("[ERROR] Missing argument! See usage with ajuda (-h)\n");
        exit(1);
      }

      // Atoi retorna 0 se não for um número // Portanto nenhum comando pode receber 0 // Segundos e ID's não podem ser 0
      if (option_with_integer_args(argv[1]) && atoi(argv[2]) <= 0) {
        perror("[ERROR] Argument must be an integer greater than zero! See usage with ajuda (-h)\n");
        exit(1);
      }

      bytes_written = sprintf(buffer_write, "%s %s\n", command, argv[2]);
    }
    else {
      bytes_written = sprintf(buffer_write, "%s\n", command);
    }

    write(client_to_server_fd, buffer_write, bytes_written);
    print_response(0);
  }
  else { // MODO CONSOLA
    print_prompt();

    while ((bytes_read = readln_v1(STD_IN, buffer_read, 1024)) > 0) {
      buffer_read[bytes_read - 1] = '\0';

      if (equals(buffer_read, "exit") || equals(buffer_read, "quit")) {
        _exit(0);
      }

      if (valid_command(strdup(buffer_read))) {
        bytes_written = sprintf(buffer_write, "%s\n", buffer_read);
        write(client_to_server_fd, buffer_write, bytes_read);
        print_response(1);
      }
      else {
        print_prompt();
      }
    }
  }

  return 0;
}
