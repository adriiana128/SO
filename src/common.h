#ifndef __COMMON__
#define __COMMON__

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>

#define READ    0
#define WRITE   1
#define STD_IN  0
#define STD_OUT 1
#define STD_ERR 2

ssize_t readln_v1(int fd, char* line, size_t size);

//Compara 2 strings usando o strcmp
int equals(char* str1, char* str2);
// Verifica se o comando tem argumentos inteiros
int command_with_integer_args(char* command);
// Verifica se o comando tem argumentos
int command_with_args(char* command);

#endif