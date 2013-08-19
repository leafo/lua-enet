enet.so: enet.c
	luarocks make --local


enet.dll: enet.c
	gcc -O2 -shared -o $@ $< -lenet -llua5.1 -lws2_32 -lwinmm

