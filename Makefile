all: enet.so docs/index.html

enet.so: enet.c
	luarocks make --local

docs/index.html: docs/docs.md
	markdown $< -o $@


