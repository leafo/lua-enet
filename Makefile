all: enet.so docs/index.html

enet.so: enet.c
	luarocks make --local

docs/index.html: README.md
	mkdir -p docs
	markdown $< -o $@


