
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <enet/enet.h>

#define check_host(l, idx)\
	*(ENetHost**)luaL_checkudata(l, idx, "enet_host")

#define check_peer(l, idx)\
	*(ENetPeer**)luaL_checkudata(l, idx, "enet_peer")

/**
 * Parse address string, eg:
 * 	*:5959
 * 	127.0.0.1:*
 * 	website.com:8080
 */
static void parse_address(lua_State *l, const char *addr_str, ENetAddress *address) {
	int host_i = 0, port_i = 0;
	char host_str[128] = {0};
	char port_str[32] = {0};
	int scanning_port = 0;

	char *c = (char *)addr_str;

	while (*c != 0) {
		if (host_i >= 128 || port_i >= 32 ) luaL_error(l, "Hostname too long");
		if (scanning_port) {
			port_str[port_i++] = *c;
		} else {
			if (*c == ':') {
				scanning_port = 1;
			} else {
				host_str[host_i++] = *c;
			}
		}
		c++;
	}
	host_str[host_i] = '\0';
	port_str[port_i] = '\0';

	if (host_i == 0) luaL_error(l, "Failed to parse address");
	if (port_i == 0) luaL_error(l, "Missing port in address");

	if (strcmp("*", host_str) == 0) {
		address->host = ENET_HOST_ANY;
	} else {
		if (enet_address_set_host(address, host_str) != 0) {
			luaL_error(l, "Failed to resolve host name");
		}
	}

	if (strcmp("*", port_str) == 0) {
		address->port = ENET_PORT_ANY;
	} else {
		address->port = atoi(port_str);
	}
}

static void push_peer(lua_State *l, ENetPeer *peer) {
	// try to find in peer table
	lua_getfield(l, LUA_REGISTRYINDEX, "enet_peers");
	lua_pushlightuserdata(l, peer);
	lua_gettable(l, -2);

	if (lua_isnil(l, -1)) {
		printf("creating new peer\n");
		lua_pop(l, 1);

		*(ENetPeer**)lua_newuserdata(l, sizeof(void*)) = peer;
		luaL_getmetatable(l, "enet_peer");
		lua_setmetatable(l, -2);

		lua_pushlightuserdata(l, peer);
		lua_pushvalue(l, -2);

		lua_settable(l, -4);
	}
	lua_remove(l, -2); // remove enet_peers
}

/**
 * Read a packet off the stack as a string
 * idx is position of string
 */
static ENetPacket *read_packet(lua_State *l, int idx, enet_uint8 *channel_id) {
	size_t size;
	const void *data = luaL_checklstring(l, idx, &size);

	enet_uint32 flags = ENET_PACKET_FLAG_RELIABLE;
	*channel_id = 0;

	int argc = lua_gettop(l);
	if (argc >= idx+2) { /* flags */ }
	if (argc >= idx+1) {
		*channel_id = luaL_checkint(l, 3);
	}

	ENetPacket *packet = enet_packet_create(data, size, flags);
	if (packet == NULL) {
		luaL_error(l, "Failed to create packet");
	}

	return packet;
}



/**
 * Create a new host
 * Args:
 * 	address (nil for client)
 */
static int host_create(lua_State *l) {
	size_t peer_count = 64, channel_count = 1;
	enet_uint32 in_bandwidth = 0, out_bandwidth = 0;

	int have_address = 1;
	ENetAddress address;

	if (lua_gettop(l) == 0 || lua_isnil(l, 1)) {
		have_address = 0;
	} else {
		parse_address(l, luaL_checkstring(l, 1), &address);
	}

	switch (lua_gettop(l)) {
		case 5:
			out_bandwidth = luaL_checkint(l, 5);
		case 4:
			in_bandwidth = luaL_checkint(l, 4);
		case 3:
			channel_count = luaL_checkint(l, 3);
		case 2:
			peer_count = luaL_checkint(l, 2);
	}

	ENetHost *host = enet_host_create(have_address ? &address : NULL, peer_count,
			channel_count, in_bandwidth, out_bandwidth);

	if (host == NULL) {
		return luaL_error(l, "Failed to create host");
	}

	*(ENetHost**)lua_newuserdata(l, sizeof(void*)) = host;
	luaL_getmetatable(l, "enet_host");
	lua_setmetatable(l, -2);

	return 1;
}

/**
 * Serice a host
 * Args:
 * 	timeout
 *
 * Return
 * 	nil on no event
 * 	an event table on event
 */
static int host_service(lua_State *l) {
	ENetHost *host = check_host(l, 1);
	int timeout = 0;

	if (lua_gettop(l) > 1)
		timeout = luaL_checkint(l, 2);

	ENetEvent event;
	int out = enet_host_service(host, &event, timeout);
	if (out == 0) return 0;
	if (out < 0) return luaL_error(l, "Error during service");

	// extract event and return it
	lua_newtable(l);

	push_peer(l, event.peer);
	lua_setfield(l, -2, "peer");

	switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			lua_pushstring(l, "connect");
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			lua_pushstring(l, "disconnect");
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			lua_pushlstring(l, (const char *)event.packet->data, event.packet->dataLength);
			lua_setfield(l, -2, "data");
			lua_pushstring(l, "receive");
			break;
		case ENET_EVENT_TYPE_NONE: break;
	}
	lua_setfield(l, -2, "type");
	return 1;
}

/**
 * Connect a host to an address
 * Args:
 * 	the address
 */
static int host_connect(lua_State *l) {
	ENetHost *host = check_host(l, 1);
	ENetAddress address;

	enet_uint32 data = 0;
	size_t channel_count = 1;

	parse_address(l, luaL_checkstring(l, 2), &address);

	switch (lua_gettop(l)) {
	}

	ENetPeer *peer = enet_host_connect(host, &address, channel_count, data);

	if (peer == NULL) {
		return luaL_error(l, "Failed to create peer");
	}


	push_peer(l, peer);

	return 1;
}

static int host_flush(lua_State *l) {
	ENetHost *host = check_host(l, 1);
	enet_host_flush(host);
	return 0;
}

static int host_broadcast(lua_State *l) {
	ENetHost *host = check_host(l, 1);

	enet_uint8 channel_id;
	ENetPacket *packet = read_packet(l, 2, &channel_id);

	enet_host_broadcast(host, channel_id, packet);
	return 0;
}

static int host_gc(lua_State *l) {
	ENetHost *host = check_host(l, 1);
	enet_host_destroy(host);
	return 0;
}

static int peer_tostring(lua_State *l) {
	ENetPeer *peer = check_peer(l, 1);
	char host_str[128];
	enet_address_get_host_ip(&peer->address, host_str, 128);

	lua_pushstring(l, host_str);
	lua_pushstring(l, ":");
	lua_pushinteger(l, peer->address.port);
	lua_concat(l, 3);
	return 1;
}

static int peer_disconnect(lua_State *l) {
	ENetPeer *peer = check_peer(l, 1);

	enet_uint32 data = 0;
	if (lua_gettop(l) > 1) {
		data = luaL_checkint(l, 2);
	}

	enet_peer_disconnect(peer, data);
	return 0;
}

static int peer_reset(lua_State *l) {
	ENetPeer *peer = check_peer(l, 1);
	enet_peer_reset(peer);
	return 0;
}

static int peer_receive(lua_State *l) {
	ENetPeer *peer = check_peer(l, 1);

	enet_uint8 channel_id = 0;
	if (lua_gettop(l) > 1) {
		channel_id = luaL_checkint(l, 2);
	}

	ENetPacket *packet = enet_peer_receive(peer, channel_id);
	if (packet == NULL) return 0;
	return 0;
}


/**
 * Send a lua string to a peer
 * Args:
 * 	packet data, string
 * 	channel id
 * 	flags ["reliable", nil]
 *
 */
static int peer_send(lua_State *l) {
	ENetPeer *peer = check_peer(l, 1);

	enet_uint8 channel_id;
	ENetPacket *packet = read_packet(l, 2, &channel_id);

	enet_peer_send(peer, channel_id, packet);
	return 0;
}

static const struct luaL_Reg enet_funcs [] = {
	{"host_create", host_create},
	{NULL, NULL}
};

static const struct luaL_Reg enet_host_funcs [] = {
	{"service", host_service},
	{"connect", host_connect},
	{"broadcast", host_broadcast},
	{"flush", host_flush},
	{NULL, NULL}
};

static const struct luaL_Reg enet_peer_funcs [] = {
	{"disconnect", peer_disconnect},
	{"reset", peer_reset},
	{"receive", peer_receive},
	{"send", peer_send},
	{NULL, NULL}
};

static const struct luaL_Reg enet_event_funcs [] = {
	{NULL, NULL}
};

LUALIB_API int luaopen_enet(lua_State *l) {
	enet_initialize();
	atexit(enet_deinitialize);

	// create metatables
	luaL_newmetatable(l, "enet_host");
	lua_newtable(l); // index
	luaL_register(l, NULL, enet_host_funcs);
	lua_setfield(l, -2, "__index");
	lua_pushcfunction(l, host_gc);
	lua_setfield(l, -2, "__gc");

	luaL_newmetatable(l, "enet_peer");
	lua_newtable(l);
	luaL_register(l, NULL, enet_peer_funcs);
	lua_setfield(l, -2, "__index");
	lua_pushcfunction(l, peer_tostring);
	lua_setfield(l, -2, "__tostring");


	// set up peer table
	lua_newtable(l);

	lua_newtable(l); // metatable
	lua_pushstring(l, "v");
	lua_setfield(l, -2, "__mode");
	lua_setmetatable(l, -2);

	lua_setfield(l, LUA_REGISTRYINDEX, "enet_peers");

	luaL_register(l, "enet", enet_funcs);
	printf("loaded enet\n");
	return 0;
}

