# SO 2019/2020

A Makefile está linda! Antes de darem commit em alguma coisa façam:

    $ make clean

Para compilar:

    $ make

Para correr um servidor em background e um client na mesma janela:

    $ make run

Para correr um servidor

    $ make s

Para correr um client

    $ make c

Para limpar todo o historico e recompilar:

    $ make power




3 ficheiros para as variáveis globais => Não existem caso não altere os valores
  current_task_id -> Só é criada quando crio uma tarefa
  max_execution_time
  max_inactivity_time

argus_history
  histórico de comandos terminados

  #ID, (concluída / max execução / max inatividade / terminada pelo utilizador): tarefa (e.g cat | wc)

  #1, max execução: cat | wc
  #2, max execução: cat | wc
  #4, max execução: wc | cat
  #5, terminada pelo utilizador: wc | cat
  #6, concluída: pwd
  #7, max execução: wc | cat
  #8, concluída: pwd sdf
  #9, concluída: pwd sdf

2 ficheiros para tarefas em execução
  task_info_ID - tem informação necessária da tarefa 3 linhas com PID da tarefa a ser executada, tarefa (e.g cat | wc), data de criação da tarefa 1591436572 (data de crição do ficheiro task_info_ID)
  task_log_ID  - o log da tarefa - um log parcial do servidor (para evitar confusão no log)

2 ficheiros de log
  log     - Contém o output sequencial (que pode ser vazio) de todas as tarefas terminadas
  log.idx - Contém o índice do log. Cada linha tem ID start_byte count_bytes. ID é o ID da tarefa, o start_byte é o byte em que começa o log da tarefa e o count_bytes é o tamanho do output


Diferença entre tarefa em execução e tarefa terminada:
  - Em execução:
    - existem os ficheiros task_info_ID, task_log_ID
  - Terminada:
    - não existem os ficheiros task_info_ID, task_log_ID,
    - o ficheiro de log tem mais conteúdo
    - o log.idx tem mais entrada
    - o argus_history tem mais uma entrada