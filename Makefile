

mapach: src/mapach.c src/mapach.h src/main.c
	gcc -Wall -g src/mapach.c src/main.c -lpng -lz -o mapach
