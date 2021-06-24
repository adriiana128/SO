all: 
	gcc -Wall src/common.c src/argusd.c -o argusd
	gcc -Wall src/common.c src/argus.c -o argus

run: 
	./argusd &>/dev/null &
	./argus

c: 
	./argus

s: 
	./argusd

clean:
	rm -f argus
	rm -f argusd
	rm -f client_to_server_fifo
	rm -f server_to_client_fifo
	rm -f argus_history
	rm -f current_task_id
	rm -f max_execution_time
	rm -f max_inactivity_time
	rm -f log
	rm -f log.idx

power:
	make clean && make



