.SUFFIXES: .c .so

CC = gcc -Wall -O3 -fopenmp -ffast-math -funroll-loops -shared -fPIC

.c.so   :
	$(CC) $? -o $@

cg: cg.so
	python cg.py
clean:
	@make -C .. clean
commit:
	@make -C .. commit
save:
	@make -C .. save
revert:
	@make -C .. revert
