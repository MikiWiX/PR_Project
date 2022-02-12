FD0 = arrays
FD1 = tools
DEPS = $(FD0)/taskSet.h $(FD0)/int-array.h $(FD1)/receiver.h $(FD1)/printer.h
FILE = main.c $(FD0)/taskSet.c $(FD0)/int-array.c $(FD1)/receiver.c $(FD1)/printer.c

biuld: compile run

.PHONY : compie run

compile: $(FILE) $(DEPS)
	mpicc $(FILE) -O0 -lpthread

run: 
	mpirun -np 4 ./a.out
