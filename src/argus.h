#ifndef __ARGUS__
#define __ARGUS__

// Verifica se o comando tem argumentos inteiros
int option_with_integer_args(char* option);
// Verifica se o comando tem argumentos
int option_with_args(char* option);
// Vai buscar comando a partir da opção
char* get_command(char* option);

// Verifica se um comando é valido
int valid_command(char* command);

// Escreve no stdout "argus$ "
void print_prompt();
// Imprime as respostas que vêm do servidor
void print_response(int prompt);

#endif