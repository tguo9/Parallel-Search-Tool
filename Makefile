psearch: psearch.c
	gcc -pthread -o psearch psearch.c

clean:
	rm -f psearch
