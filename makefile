build:
	gcc main.c -std=c99 -Os -lX11 -pthread -o snake

debug:
	tcc main.c -std=c99 -lX11 -pthread -run