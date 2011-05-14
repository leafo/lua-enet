# lua-enet

**lua-enet** is a binding to the [ENet](http://enet.bespin.org/) library for
Lua. ENet is a thin network communication layer over UDP that provides high
performance and reliable communication that is suitable for games. The
interface exposes an asynchronous non-blocking way of creating clients and
servers that is easy to integrate into existing event loops.

To get an idea of how ENet works and what it provides have a look at 
[Features and Achritecture](http://enet.bespin.org/Features.html) from the
original documentation.

* [Download & Install](#install)
* [Tutorial](#tutorial)
* [Reference](#reference)
* [Contact](#contact)


<a name="install"></a>
## Download & Install
The easiest way to install **lua-enet** is to use [Lua Rocks](http://www.luarocks.org/):

    $ luarocks install enet

The source can be obtained from github:
<http://leafo.net/github/lua-enet>


<a name="tutorial"></a>
## Tutorial

We'll need a client and a server. A server is a *host* bound to an address.
A client is an unbound *host* that connects to an address.
`enet.host_create` is used to create a new host. `host:service` is used to wait
for events and send packets. It can optionally have a specified timeout.

When `host:service` receives an event it returns an event object, which is a
plain Lua table. The `type` entry holds the kind of event as a string, and the
`peer` entry has the associated peer who triggered the event.

Using this information, we can make a simple echo server:


    -- server.lua
    require "enet"
    local host = enet.host_create"localhost:6789"
    while true do
        local event = host:service(100)
        if event and event.type == "receive" then
            print("Got message: ", event.data, event.peer)
            event.peer:send(event.data)
        end
    end


Data, whether sent or received is always a Lua string. Primitive types can be
converted to a binary string to reduce the number of bytes used using a binary
serialization library.

The client for our server can be written like so:

    -- client.lua
    require "enet"
    local host = enet.host_create()
    local server = host:connect("localhost:6789")

    local done = false
    while not done do
        local event = host:service(100)
        if event then
            if event.type == "connect" then
                print("Connected to", event.peer)
                event.peer:send("hello world")
            elseif event.type == "receive" then
                print("Got message: ", event.data, event.peer)
                done = true
            end
        end
    end

    server:disconnect()
    host:flush()

Upon receiving the connect message we send `"hello world"` to the server, then
wait for the response.

When a client disconnects, make sure to call `disconnect` on the server object,
and then tell the host to flush (unless `host_service` will be called again)
otherwise the server will have to wait for the client to disconnect from timeout.

<a name="reference"></a>
## Reference


### `enet.host_create([bind_address, peer_count, channel_count, in_bandwidth, out_bandwidth])`
Returns a new host. All arguments are optional.

A `bind_address` of `nil` makes a host that can not be connected to (typically
a client).  Otherwise the  address can either be of the form
`<ipaddress>:<port>`, `<hostname>:<port>`, or `*:<port>`.

Examples addresses include `"127.0.0.1:8888"`, `"localhost:2232"`, and `"*:6767"`.

Parameters:

 * `peer_count` max number of peers, defaults to `64`
 * `channel_count` max number of channels, defaults to `1`
 * `in_bandwidth` downstream bandwidth in bytes/sec, defaults to `0`
   (unlimited)
 * `out_bandwidth` upstream bandwidth in bytes/sec, defaults to `0`
   (unlimited)

### `host:connect(address [, channels_count, data])`
Connects a host to a remote host. Returns peer object associated with remote host.

The actual connection will not take place until the next `host:service` done,
in which a `"connect"` event will be generated.

`channel_count` is the number of channels to allocate. It should be the same as
the channel count on the server. Defaults to `1`.

`data` is an integer value that can be associated with the connect event.
Defaults to `0`.

### `host:service([timeout])`
Wait for events, send and receive any ready packets. `timeout` is the max
number of milliseconds to be wait for an event. By default `timeout` is `0`.

Returns `nil` on timeout if not events occurred.

If an event happens an event table is returned. All events have a `type` entry,
which is one of `"connect"`, `"disconnect"`, `"receive"`. Events also have a
`peer` entry which holds the peer object of who triggered the event.

A receive event also has a `data` entry which is a Lua string containing the
data received.

### `host:flush()`
Sends any queued packets. This is only required to send packets earlier than
the next call to `host:service`, or if `host:service` will not be called again.

### `peer:disconnect([data])`
Requests a disconnection from the peer. The message is sent on next
`host:service` or `host:flush`.

`data` is optional integer value to be associated with the disconnect.

### `peer:disconnect_now([data])`
Force immediate disconnection from peer. Foreign peer not guaranteed to receive
disconnect notification.

`data` is optional integer value to be associated with the disconnect.

### `peer:disconnect_later([data])`
Request a disconnection from peer, but only after all queued outgoing packets
are sent.

`data` is optional integer value to be associated with the disconnect.

### `peer:reset()`
Forcefully disconnects peer. The peer is not notified of the disconnection.

### `peer:ping()`
Send a ping request to peer, updates `round_trip_time`. This is called
automatically at regular intervals.

### `peer:throttle_configure(interval, acceleration, decceleration)`
Changes the probability at which unreliable packets should not be dropped.

Parameters:

 * `interval` interval in milliseconds to measure lowest mean RTT
 * `acceleration` rate at which to increase throttle probability as mean RTT
   declines
 * `decceleration` rate at which to decrease throttle probability as mean RTT
   increases

### `peer:receive()`
Attempts to dequeue an incoming packet for this peer.

Returns `nil` if there are no packets waiting. Otherwise returns two values:
the string representing the packet data, and the channel the packet came from.

<a name="contact"></a>
## Contact

