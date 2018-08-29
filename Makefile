
enet2.dll: enet.c
	gcc -O2 -shared -o $@ $< -lenet -llua51 -lws2_32 -lwinmm

enet.so: enet.c
	luarocks make --local enet-dev-1.rockspec

