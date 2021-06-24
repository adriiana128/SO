#include "common.h"
#include "argusd.h"

int current_task_id     = 0;
int max_execution_time  = DEFAULT_MAX_EXECUTION_TIME;
int max_inactivity_time = DEFAULT_MAX_INACTIVITY_TIME;

int my_system(char* comando) {
  char* aux   = strdup(comando);
  char **args = NULL;
  int indice  = 0;

  int fdin  = -1, stdinCopy  = -1;
  int fdout = -1, stdoutCopy = -1;
  int fderr = -1, stderrCopy = -1;

  char *ptr = strtok(aux," ");

  while(ptr != NULL) {
    if (!strcmp("<", ptr) && fdin == -1) { // command < file - command input from file
      stdinCopy = dup(0);
      ptr       = strtok(NULL, " ");
      fdin      = open(ptr, O_RDONLY, S_IREAD);

      dup2(fdin, 0);
      close(fdin);

      ptr = strtok(NULL, " ");
    }
    else if (!strcmp(">", ptr) && fdout == -1) { // command > file1 - command output to file
      stdoutCopy = dup(1);
      ptr        = strtok(NULL, " ");
      fdout      = open(ptr,O_CREAT|O_TRUNC|O_WRONLY,S_IRWXU);

      dup2(fdout,1);
      close(fdout);

      ptr = strtok(NULL, " ");
    }
    else if (!strcmp(">>", ptr) && fdout == -1) { // command >> append command output to file
      stdoutCopy = dup(1);
      ptr        = strtok(NULL, " ");
      fdout      = open(ptr, O_CREAT| O_APPEND | O_WRONLY, S_IRWXU);

      dup2(fdout,1);
      close(fdout);

      ptr = strtok(NULL, " ");
    }
    else if (!strcmp("2>", ptr) && fderr == -1) { // command 2> file - stderr output to file
      stderrCopy = dup(2);
      ptr        = strtok(NULL, " ");
      fderr      = open(ptr, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);

      dup2(fdout,2);
      close(fdout);

      ptr = strtok(NULL, " ");
    }
    else if (!strcmp("2>>", ptr) && fderr == -1) { // command 2>> file - append stderr output to file
      stderrCopy = dup(2);
      ptr        = strtok(NULL, " ");
      fderr      = open(ptr, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);

      dup2(fdout,2);
      close(fdout);

      ptr = strtok(NULL, " ");
    }
    else { // senão inserir argumento
      args = realloc(args,sizeof(char*)*(indice+1));
      if (!args) exit(-1);

      args[indice] = ptr;
      indice++;

      ptr = strtok(NULL, " ");
    }
  }

  args = realloc(args, sizeof(char*) * (indice + 1));
  args[indice] = 0;
  indice++;

  pid_t filho = fork();
  if (!filho) {
    execvp(args[0], args);
    exit(127);
  }

  if (stdinCopy != -1) {
    dup2(stdinCopy,0);
   close(stdinCopy);
  }
  if (stdoutCopy != -1) {
    dup2(stdoutCopy,1);
    close(stdoutCopy);
  }
  if (stderrCopy != -1) {
    dup2(stderrCopy,2);
   close(stderrCopy);
  }

  int status;
  waitpid(filho,&status,0);

  return (WEXITSTATUS(status));
}

int my_popen(char* comando, char* modo) {
  pid_t filho;
  int pd[2];
  int res = -1;

  pipe(pd);

  if((filho = fork()) == 0) {
    if(!strcmp(modo, "r")) {
      close(pd[READ]);
      dup2(pd[WRITE],1);
      close(pd[WRITE]);
    }
    else if(!strcmp(modo, "w")) {
      close(pd[WRITE]);
      dup2(pd[READ],0);
      close(pd[READ]);
    }

    int res = my_system(comando);

    exit(res);
  }
  else {
    if(!strcmp(modo, "r")) {
      close(pd[WRITE]);
      return pd[READ];
    }
    else if(!strcmp(modo, "w")) {
      close(pd[READ]);
      return pd[WRITE];
    }
  }

  return res;
}

void my_bash(char *linhaComandos) {
  if(!fork()) {
    int status;
    pid_t pid;

    if ((pid = fork()) == 0) {
      // Handler para o comando terminar
      signal(SIGTERM, sig_handler_for_commands);
      signal(SIGUSR1, sig_handler_for_commands);
      signal(SIGUSR2, sig_handler_for_commands);

      // Redireccionar output das tarefa para o log
      int task_log_fd = open(get_task_log_filepath(current_task_id), O_CREAT | O_TRUNC | O_WRONLY, 0666);

      dup2(task_log_fd, STD_OUT);
      dup2(task_log_fd, STD_ERR);
      close(task_log_fd);

      // Criar array com os comandos
      char **comandos = NULL;
      int indice      = 0;
      char *ptr       = strtok(linhaComandos,"|");

      while(ptr != NULL) {
        comandos = realloc(comandos, sizeof(char*) * (indice + 1));

        comandos[indice] = ptr;
        indice++;

        ptr = strtok(NULL, "|");
      }

      // Executar comandos de forma encadeada
      pid_t filho;
      int status;
      int pipe_file_descriptors[2];
      int fdin = 0;

      int i;
      for (i = 0; i < indice; i++) {
        pipe(pipe_file_descriptors);

        if ((filho = fork()) == 0) {
          dup2(fdin, 0);

          if (i < indice - 1) {
            dup2(pipe_file_descriptors[WRITE], 1);
          }

          close(pipe_file_descriptors[READ]);

          int res = my_system(comandos[i]);

          exit(res);
        }
        else {
          waitpid(filho,&status,0);
          close(pipe_file_descriptors[WRITE]);
          fdin = pipe_file_descriptors[READ];

          if (WEXITSTATUS(status) == 127) {
            char buf[1024];

            sprintf(buf, "%s - Command not found or incorrect arguments.\n", comandos[i]);
            write(1, buf, strlen(buf));

            break;
          }
        }
      }

      _exit(CONCLUDED_EXIT_STATUS);
    }
    else {
      char buffer[1024];
      int bytes_written = 0;
      int task_info_fd  = open(get_task_info_filepath(current_task_id), O_CREAT | O_WRONLY, 0666);

      bytes_written = sprintf(buffer, "%d\n", pid);
      write(task_info_fd, buffer, bytes_written);

      bytes_written = sprintf(buffer, "%s\n", linhaComandos);
      write(task_info_fd, buffer, bytes_written);

      bytes_written = sprintf(buffer, "%ld\n", time(NULL));
      write(task_info_fd, buffer, bytes_written);

      close(task_info_fd);

      waitpid(pid, &status, 0);
      handle_task_finish(current_task_id, WEXITSTATUS(status));

      _exit(0);
    }
  }
}

void cleanup_and_start_server() {
  int i = 0;
  current_task_id     = get_current_task_id();
  max_execution_time  = get_max_execution_time();
  max_inactivity_time = get_max_inactivity_time();

  for (i = 1; i <= current_task_id; i++) {
    remove_task_info_file(i);
    remove_task_log_file(i);
  }

  start_execution_time_monitor();

  printf("Argus server started!\n");
  printf("Current task id: %d\n", current_task_id);
}

void start_execution_time_monitor() {
  if (!fork()) {
    while (1) {
      signal(SIGALRM, sig_handler_for_timeouts);

      alarm(1);
      pause();
    }
  }
}

int get_max_inactivity_time() {
  char buffer[32];
  int bytes_read = 0;
  int max_inactivity_time_fd = open("max_inactivity_time", O_RDONLY, 0666);

  if (max_inactivity_time_fd == -1) {
    return DEFAULT_MAX_INACTIVITY_TIME;
  }

  bytes_read = readln_v1(max_inactivity_time_fd, buffer, 32);
  buffer[bytes_read - 1] = '\0';
  int max_inactivity_time = atoi(buffer);

  close(max_inactivity_time_fd);

  return max_inactivity_time;
}

void update_max_inactivity_time() {
  int max_inactivity_time_fd = open("max_inactivity_time", O_CREAT | O_TRUNC | O_WRONLY, 0666);

  char buffer[64];
  int bytes_written = 0;

  bytes_written = sprintf(buffer, "%d\n", max_inactivity_time);
  write(max_inactivity_time_fd, buffer, bytes_written);

  close(max_inactivity_time_fd);
}

int get_max_execution_time() {
  char buffer[32];
  int bytes_read = 0;
  int max_execution_time_fd = open("max_execution_time", O_RDONLY, 0666);

  if (max_execution_time_fd == -1) {
    return DEFAULT_MAX_EXECUTION_TIME;
  }

  bytes_read = readln_v1(max_execution_time_fd, buffer, 32);
  buffer[bytes_read - 1] = '\0';
  int max_execution_time = atoi(buffer);

  close(max_execution_time_fd);

  return max_execution_time;
}

void update_max_execution_time() {
  int max_execution_time_fd = open("max_execution_time", O_CREAT | O_TRUNC | O_WRONLY, 0666);

  char buffer[64];
  int bytes_written = 0;

  bytes_written = sprintf(buffer, "%d\n", max_execution_time);
  write(max_execution_time_fd, buffer, bytes_written);

  close(max_execution_time_fd);
}

int get_current_task_id() {
  char buffer[32];
  int bytes_read = 0;
  int current_task_id_fd = open("current_task_id", O_RDONLY, 0666);

  if (current_task_id_fd == -1) {
    return 0;
  }

  bytes_read = readln_v1(current_task_id_fd, buffer, 32);
  buffer[bytes_read - 1] = '\0';
  int task_id = atoi(buffer);

  close(current_task_id_fd);

  return task_id;
}

void update_current_task_id() {
  int current_task_id_fd = open("current_task_id", O_CREAT | O_TRUNC | O_WRONLY, 0666);

  char buffer[32];
  int bytes_written = 0;

  bytes_written = sprintf(buffer, "%d\n", current_task_id);
  write(current_task_id_fd, buffer, bytes_written);

  close(current_task_id_fd);
}

char* get_task_info_filepath(int task_id) {
  char buffer[32];

  sprintf(buffer, "task_info_%d", task_id);

  return strdup(buffer);
}

pid_t get_task_info_pid(int task_id) {
  char buffer[1024];
  int bytes_read = 0;
  int task_info_fd = open(get_task_info_filepath(task_id), O_RDONLY, 0666);

  if (task_info_fd == -1) {
    return -1;
  }

  bytes_read = readln_v1(task_info_fd, buffer, 1024);
  buffer[bytes_read - 1] = '\0';
  pid_t pid = atoi(buffer);
  close(task_info_fd);

  return pid;
}

char* get_task_info_task(int task_id) {
  char buffer[1024];
  int bytes_read = 0;
  int task_info_fd = open(get_task_info_filepath(task_id), O_RDONLY, 0666);

  if (task_info_fd == -1) {
    return "";
  }

  bytes_read = readln_v1(task_info_fd, buffer, 1024);
  bytes_read = readln_v1(task_info_fd, buffer, 1024);
  buffer[bytes_read - 1] = '\0';

  close(task_info_fd);

  return strdup(buffer);
}

int get_task_info_creation_time_in_seconds(int task_id) {
  char buffer[1024];
  int bytes_read = 0;
  int task_info_fd = open(get_task_info_filepath(task_id), O_RDONLY, 0666);

  if (task_info_fd == -1) {
    return 0;
  }

  bytes_read = readln_v1(task_info_fd, buffer, 1024);   // PID
  bytes_read = readln_v1(task_info_fd, buffer, 1024);   // Tarefa
  bytes_read = readln_v1(task_info_fd, buffer, 1024);   // Tempo em segundos
  buffer[bytes_read - 1] = '\0';

  close(task_info_fd);

  return atoi(strdup(buffer));
}

void remove_task_info_file(int task_id) {
  char buffer[32];

  sprintf(buffer, "rm -f task_info_%d", task_id);

  my_system(strdup(buffer));
}

char* get_task_log_filepath(int task_id) {
  char buffer[32];

  sprintf(buffer, "task_log_%d", task_id);

  return strdup(buffer);
}

void remove_task_log_file(int task_id) {
  char buffer[32];

  sprintf(buffer, "rm -f task_log_%d", task_id);

  my_system(strdup(buffer));
}

void append_info_to_log_index_file(int task_id) {
  // Buscar o último de ficheiro de log
  int log_fd = open("log", O_RDONLY, 0666);
  int output_first_byte = lseek(log_fd, 0, SEEK_END) + 1;
  close(log_fd);

  // Buscar o tamanho do log parcial
  int task_log_fd = open(get_task_log_filepath(task_id), O_RDONLY, 0666);
  int output_bytes_count = lseek(task_log_fd, 0, SEEK_END);
  close(task_log_fd);

  char buffer[64];
  int bytes_written = 0;
  int log_index_fd = open("log.idx", O_CREAT | O_APPEND | O_WRONLY, 0666);

  bytes_written = sprintf(buffer, "%d %d %d\n", task_id, output_first_byte, output_bytes_count); // ID start_byte bytes_count
  write(log_index_fd, buffer, bytes_written);
  close(log_index_fd);
}

void append_output_to_log_file(int task_id) {
  char buffer[64];

  sprintf(buffer, "cat %s >> log", get_task_log_filepath(task_id));

  my_system(strdup(buffer));
}

void sig_handler_for_commands(int signal) {
  if (signal == SIGUSR1) {
    _exit(EXECUTION_TIMEOUT_EXIT_STATUS);
  }
  else if (signal == SIGUSR2) {
    _exit(INACTIVITY_TIMEOUT_EXIT_STATUS);
  }
  else if (signal == SIGTERM) {
    _exit(TERMINATED_EXIT_STATUS);
  }
}

void sig_handler_for_timeouts(int signal) {
  if(signal == SIGALRM) {
    int i;
    int task_ids                    = get_current_task_id();
    int current_max_execution_time  = get_max_execution_time();
    int current_max_inactivity_time = get_max_inactivity_time();
    int current_time                = time(NULL);

    for (i = 1; i <= task_ids; i++) {
      pid_t pid = get_task_info_pid(i);

      if (pid > 0) {
        int creation_time_in_seconds = get_task_info_creation_time_in_seconds(i);

        if (current_time - creation_time_in_seconds > current_max_execution_time) {
          kill(pid, SIGUSR1);
        }
        else {
          char buffer[32];
          int bytes_read = 0;

          sprintf(buffer, "stat -f%%c task_log_%d", i);
          char *command = strdup(buffer);

          int popen_fd = my_popen(command, "r");
          bytes_read   = readln_v1(popen_fd, buffer, 32);
          buffer[bytes_read - 1] = '\0';

          int last_update_time_in_seconds = atoi(buffer);

          if (current_time - last_update_time_in_seconds > current_max_inactivity_time) {
            kill(pid, SIGUSR2);
          }
        }
      }
    }
  }
}

void handle_task_finish(int task_id, int status) {
  int argus_history_fd = open("argus_history", O_CREAT | O_APPEND | O_WRONLY, 0666);
  char* exit_status = "";
  char buffer[1024];
  int bytes_written = 0;

  if (status == CONCLUDED_EXIT_STATUS) {
    exit_status = "concluída";
  }
  else if (status == INACTIVITY_TIMEOUT_EXIT_STATUS) {
    exit_status = "max inactividade";
  }
  else if (status == EXECUTION_TIMEOUT_EXIT_STATUS) {
    exit_status = "max execução";
  }
  else if (status == TERMINATED_EXIT_STATUS) {
    exit_status = "terminada pelo utilizador";
  }

  bytes_written = sprintf(buffer, "#%d, %s: %s\n", task_id, exit_status, get_task_info_task(task_id)); // #1, concluída: pwd
  write(argus_history_fd, buffer, bytes_written);

  close(argus_history_fd);

  remove_task_info_file(task_id);

  append_info_to_log_index_file(task_id);
  append_output_to_log_file(task_id);

  remove_task_log_file(task_id);
}

int valid_command(char* command) {
  char *pointer     = strtok(command, " ");
  char *command_aux = "";

  if (equals(command, "tempo-inactividade") || equals(command, "tempo-execucao") ||
      equals(command, "executar")           || equals(command, "listar")         ||
      equals(command, "terminar")           || equals(command, "historico")      ||
      equals(command, "ajuda")              || equals(command, "output") ) {
    command_aux = strdup(pointer);
  } else {
    perror("[ERROR] Invalid command! See usage with ajuda (-h)\n");
    return 0;
  }

  if (command_with_args(command_aux)) {
    pointer = strtok(NULL, "");

    if (pointer == NULL) {
      perror("[ERROR] Missing argument! See usage with ajuda (-h)\n");
      return 0;
    }

    if (command_with_integer_args(command_aux) && atoi(pointer) <= 0) {
      perror("[ERROR] Argument must be an integer greater than zero! See usage with ajuda (-h)\n");
      return 0;
    }
  }

  return 1;
}

void send_response(char* message, int value) {
  int status;

  if (!fork()) {
    int server_to_client_fd = open("server_to_client_fifo", O_WRONLY);
    int bytes_written       = 0;
    char buffer[1024];

    bytes_written = sprintf(buffer, "%s%d\n", message, value);
    write(server_to_client_fd, buffer, bytes_written);

    close(server_to_client_fd);
    _exit(0);
  } else {
    wait(&status);
  }
}

void send_help() {
  int status;

  if (!fork()) {
    int server_to_client_fd = open("server_to_client_fifo", O_WRONLY);

    int i = 0;
    int bytes_written = 0;
    char buffer[1024];

    char* help[8] = {
      "tempo-inactividade seconds (-i seconds)",
      "tempo-execucao seconds (-m seconds)",
      "executar \"p1 | p2 | ... | pn\" (-e \"p1 | p2 | ... | pn\")",
      "listar (-l)",
      "terminar task_id (-t task_id)",
      "historico (-r)",
      "ajuda (-h)",
      "output task_id (-o task_id)"
    };

    for (i = 0; i < 8; i++) {
      bytes_written = sprintf(buffer, "%s\n", help[i]);
      write(server_to_client_fd, buffer, bytes_written);
    }

    close(server_to_client_fd);
    _exit(0);
  } else {
    wait(&status);
  }
}

void send_history() {
  int status;

  if (!fork()) {
    int argus_history_fd    = open("argus_history", O_RDONLY, 0666);
    int server_to_client_fd = open("server_to_client_fifo", O_WRONLY);

    char buffer[1024];
    int bytes_read = 0;

    while ((bytes_read = readln_v1(argus_history_fd, buffer, 1024)) > 0) {
      write(server_to_client_fd, buffer, bytes_read);
    }

    close(server_to_client_fd);
    close(argus_history_fd);

    _exit(0);
  } else {
    wait(&status);
  }
}

void send_open_tasks() {
  int status;

  if (!fork()) {
    int server_to_client_fd = open("server_to_client_fifo", O_WRONLY);
    int i = 0;
    int bytes_written = 0;
    char buffer[1024];
    char* task = "";

    for (i = 0; i <= current_task_id; i++) {
      task = get_task_info_task(i);

      if (!equals(task, "")) {
        bytes_written = sprintf(buffer, "#%d: %s\n", i, task); // #2: cat | wc
        write(server_to_client_fd, buffer, bytes_written);
      }
    }

    close(server_to_client_fd);

    _exit(0);
  } else {
    wait(&status);
  }
}

void terminate_task(int task_id) {
  if (!fork()) {
    pid_t pid = get_task_info_pid(task_id);

    if (pid > 0) {
      kill(pid, SIGTERM);
    }

    _exit(0);
  }
}

void send_task_output(int task_id) {
  int status;

  if (!fork()) {
    int server_to_client_fd = open("server_to_client_fifo", O_WRONLY);
    int log_index_fd = open("log.idx", O_RDONLY, 0666);

    char buffer_read[128];
    char buffer_write[1024];
    int bytes_read    = 0;

    while ((bytes_read = readln_v1(log_index_fd, buffer_read, 128)) > 0) {
      buffer_read[bytes_read - 1] = '\0';

      char *ptr = strtok(buffer_read, " ");

      if (atoi(ptr) == task_id) {
        ptr = strtok(NULL, " ");
        int output_first_byte  = atoi(ptr);

        ptr = strtok(NULL, "");
        int output_bytes_count = atoi(ptr);

        int log_fd = open("log", O_RDONLY, 0666);
        lseek(log_fd, output_first_byte - 1, SEEK_SET);
        int total_bytes_read = 0;

        while ((bytes_read = readln_v1(log_fd, buffer_write, 1024)) > 0 && total_bytes_read < output_bytes_count) {
          total_bytes_read += bytes_read;

          write(server_to_client_fd, buffer_write, bytes_read);
        }

        close(log_fd);
        break;
      }
    }

    close(server_to_client_fd);
    close(log_index_fd);

    _exit(0);
  } else {
    wait(&status);
  }
}

void execute_command(char* command) {
  char *ptr = strtok(command, " ");

  if (equals(ptr, "tempo-inactividade")) {
    ptr = strtok(NULL, "");
    max_inactivity_time = atoi(ptr);

    update_max_inactivity_time();

    send_response("tempo-inactividade = ", max_inactivity_time);
  }
  else if (equals(ptr, "tempo-execucao")) {
    ptr = strtok(NULL, "");
    max_execution_time = atoi(ptr);

    update_max_execution_time();

    send_response("tempo-execucao = ", max_execution_time);
  }
  else if (equals(ptr, "executar")) {
    ptr = strtok(NULL, "");

    if (ptr[0] == '"') {
      ptr = ptr + 1;
    }

    if (ptr[strlen(ptr) - 1] == '"') {
      ptr[strlen(ptr) - 1] = '\0';
    }

    current_task_id++;
    send_response("nova tarefa #", current_task_id);

    update_current_task_id();

    my_bash(ptr);
  }
  else if (equals(ptr, "listar")) {
    send_open_tasks();
  }
  else if (equals(ptr, "terminar")) {
    ptr = strtok(NULL, "");

    int task_id = atoi(ptr);

    send_response("terminar #", task_id);

    terminate_task(task_id);
  }
  else if (equals(ptr, "historico")) {
    send_history();
  }
  else if (equals(ptr, "ajuda")) {
    send_help();
  }
  else if (equals(ptr, "output")) {
    ptr = strtok(NULL, "");

    int task_id = atoi(ptr);

    send_task_output(task_id);
  }
  else {
    perror("[ERROR] Invalid command! See usage with ajuda (-h)\n");
  }
}

int main(int argc, char* argv[]) {
  mkfifo("server_to_client_fifo", 0600); // Servidor --> Cliente
  mkfifo("client_to_server_fifo", 0600); // Cliente  --> Servidor

  cleanup_and_start_server();

  int client_to_server_fd = open("client_to_server_fifo", O_RDONLY);

  char buffer_read[1024];
  int bytes_read = 0;

  while (1) {
    while ((bytes_read = readln_v1(client_to_server_fd, buffer_read, 1024)) > 0) {
      buffer_read[bytes_read - 1] = '\0';

      if (valid_command(strdup(buffer_read))) {
        execute_command(buffer_read);
      }
    }
  }

  return 0;
}
