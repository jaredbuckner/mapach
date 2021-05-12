

mapach: src/indexarray.c src/mapach.c src/main.c src/maptypes.h src/indexarray.h src/mapach.h
	gcc -Wall -g $(filter %.c,$^) -lpng -lz -lm -o mapach

test_indexarray: src/indexarray.c src/indexarray.h src/maptypes.h
	gcc -Wall -g $(filter %.c,$^) -Wl,--entry=_$@ -nostartfiles -o $@
