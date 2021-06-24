#ifndef __ARGUSD__
#define __ARGUSD__

#define DEFAULT_MAX_EXECUTION_TIME  5
#define DEFAULT_MAX_INACTIVITY_TIME 5

#define CONCLUDED_EXIT_STATUS          10  // Tarefa concluída com êxito
#define INACTIVITY_TIMEOUT_EXIT_STATUS 11
#define EXECUTION_TIMEOUT_EXIT_STATUS  12
#define TERMINATED_EXIT_STATUS         13  // Tarefa terminada com -t ou terminar


int my_system(char* comando);
int my_popen(char* comando, char* tipo);
void my_bash(char *linhaComandos);

// Limpa ficheiros de tarefas a serem executadas e os logs parciais das tarefas a serem executadas
void cleanup_and_start_server();
// Cria monitor
void start_execution_time_monitor();

// Vai buscar a variável global max_inactivity_time
int get_max_inactivity_time();
// Faz actualização do valor da variável global max_inactivity_time
void update_max_inactivity_time();

// Vai buscar a variável global max_execution_time
int get_max_execution_time();
// Faz actualização do valor da variável global max_execution_time
void update_max_execution_time();

// Vai buscar o valor da variável global task_id, que é o id da tarefa
int get_current_task_id();
// Faz actualização do valor da variável global task_id, que é o id da tarefa 
void update_current_task_id();

// Vai buscar a string task_info_id (pode ser utilizada para criar ficheiro, ler ficheiro, remover ficheiro, etc.) 
char* get_task_info_filepath(int task_id);
// Função que vai buscar o pid da tarefa (do ficheiro task_info_id, primeira linha)
pid_t get_task_info_pid(int task_id);
// Função que vai buscar a tarefa (do ficheiro task_info_id, segunda linha)
char* get_task_info_task(int task_id);
// Função que vai buscar o tempo em segundos que vai aparecer no file_task_info_id na última linha
int get_task_info_creation_time_in_seconds(int task_id);
// Remove o ficheiro task_info_id
void remove_task_info_file(int task_id);

// Vai buscar a string task_log_id (pode ser utilizada para criar ficheiro, ler ficheiro, remover ficheiro, etc.) 
char* get_task_log_filepath(int task_id);
// Remove o ficheiro task_log_id
void remove_task_log_file(int task_id);

// Adiciona uma entrada no ficheiro com o índice
void append_info_to_log_index_file(int task_id);
void append_output_to_log_file(int task_id);

void sig_handler_for_commands(int signal);
void sig_handler_for_timeouts(int signal);
// Com o status retornado, vai colocar no histórico como acabou a tarefa
void handle_task_finish(int task_id, int status);

// Verifica se um comando é válido
int valid_command(char* command);

// Envia respostas genéricas ao Cliente
void send_response(char* message, int value);
// Envia ajuda
void send_help();
// Envia listagem do histórico
void send_history();
// Envia listagem de tarefas a serem executadas
void send_open_tasks();
// Termina tarefa
void terminate_task(int task_id);
// Envia o output de uma tarefa
void send_task_output(int task_id);
// Executa comando
void execute_command(char* command);

#endif