package = "enet"
version = "1.0-0"
source = {
	url = "http://leafo.net/"
}
description = {
	summary = "A library for doing network communication in Lua",
	detailed = [[
		Binding to ENet, network communication layer on top of UDP.
	]],
	homepage = "http://leafo.net/enet-lua",
	license = "MIT"
}
dependencies = {
	"lua >= 5.1"
}
external_dependencies = {
	ENET = {
		header = "enet/enet.h"
	}
}
build = {
	type = "builtin",
	modules = {
		enet = {
			sources = {"enet.c"},
			libraries = {"enet"},
			incdirs = {"$(ENET_INCDIR)"},
			libdirs = {"$(ENET_LIBDIR)"}
		}
	}
}