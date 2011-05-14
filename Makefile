
LIBS = -lenet
GCC = gcc

enet.so: enet.c
	$(GCC) -o $@ -fpic -shared $< $(LIBS) -Wall

