**lua-enet** is a binding to the [ENet](http://enet.bespin.org/) library for
Lua. ENet is a thin network communication layer over UDP that provides high
performance and reliable communication that is suitable for games. The
interface exposes an asynchronous way of creating clients and servers that is
easy to integrate into existing event loops.

To get an idea of how ENet works and what it provides have a look at 
[Features and Achritecture](http://enet.bespin.org/Features.html) from the
original documentation.

<a name="install"></a>
## Download & Install

### Linux & OSX

Before installing **lua-enet**, you must make sure you have
[ENet](http://enet.bespin.org/) installed. Consult your system's package
management.

If you've got [Lua Rocks](http://www.luarocks.org/) then installation is very easy:

    $ luarocks install enet

Otherwise you can download the source can from GitHub:
<https://github.com/leafo/lua-enet>

### Windows

Prebuilt binaries are provided:  

Download <http://leafo.net/lua-enet/bin/lua-enet-1.0.zip>.  
Unzip and install alongside your Lua installation.

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


Data, whether sent or received, is always a Lua string. Primitive types can be
converted to a binary string to reduce the number of bytes sent by using a binary
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
and then tell the host to flush (unless `host:service` will be called again)
otherwise the server will have to wait for the client to disconnect from timeout.

<a name="reference"></a>
<div class="api">
## Reference

### `enet.host_create([bind_address, peer_count, channel_count, in_bandwidth, out_bandwidth])`
Returns a new host. All arguments are optional.

A `bind_address` of `nil` makes a host that can not be connected to (typically
a client).  Otherwise the address can either be of the form
`<ipaddress>:<port>`, `<hostname>:<port>`, or `*:<port>`.

Example addresses include `"127.0.0.1:8888"`, `"localhost:2232"`, and `"*:6767"`.

Parameters:

 * `peer_count` max number of peers, defaults to `64`
 * `channel_count` max number of channels, defaults to `1`
 * `in_bandwidth` downstream bandwidth in bytes/sec, defaults to `0`
   (unlimited)
 * `out_bandwidth` upstream bandwidth in bytes/sec, defaults to `0`
   (unlimited)

### `host:connect(address [, channel_count, data])`
Connects a host to a remote host. Returns peer object associated with remote
host.  The actual connection will not take place until the next `host:service`
is done, in which a `"connect"` event will be generated.

`channel_count` is the number of channels to allocate. It should be the same as
the channel count on the server. Defaults to `1`.

`data` is an integer value that can be associated with the connect event.
Defaults to `0`.

### `host:service([timeout])`
Wait for events, send and receive any ready packets. `timeout` is the max
number of milliseconds to be waited for an event. By default `timeout` is `0`.
Returns `nil` on timeout if no events occurred.

If an event happens, an event table is returned. All events have a `type` entry,
which is one of `"connect"`, `"disconnect"`, or `"receive"`. Events also have a
`peer` entry which holds the peer object of who triggered the event.

A `"receive"` event also has a `data` entry which is a Lua string containing the
data received.

### `host:check_events()`
Checks for any queued events and dispatches one if available. Returns the
associated event if something was dispatched, otherwise `nil`.

### `host:flush()`
Sends any queued packets. This is only required to send packets earlier than
the next call to `host:service`, or if `host:service` will not be called again.

### `host:broadcast(data [, channel, flag])`
Queues a packet to be sent to all connected peers. See
[peer:send](#peersenddata__channel_flag) for arguments.

### `host:channel_limit(limit)`
Sets the maximum number of channels allowed. If it is `0` then the system
maximum allowable value is used.

### `host:bandwidth_limit(incoming, outgoing)`
Sets the bandwidth limits of the host in bytes/sec. Set to `0` for unlimited.

### `host:total_sent_data()`
Returns the number of bytes that were sent through the given host.

### `host:total_received_data()`
Returns the number of bytes that were received by the given host. 

### `host:peer_count()`
Returns the number of peers that are allocated for the given host. This
represents the maximum number of possible connections.

### `host:connected_peer_count()`
Returns the number of connected (i.e. active) peers.

### `host:get_peer(index)`
Returns the connected peer at the specified index (starting at 1). Please
note that it is not guaranteed that the index stays the same for a single
peer over time as enet pre-allocates memory in an array for all peers and
sets their state to `connected` when a connection is established.

For peers with closed connections the internal state is set to
`disconnected`. The function `host:get_peer(i)` loops over all connections
and returns the i-th connection that is active. If a peer with index j and
j < i is disconnected all peers with index i and i > j have now index i -
1.

### `peer:send(data [, channel, flag])`
Queues a packet to be sent to peer. `data` is the contents of the packet, it
must be a Lua string.

`channel` is the channel to send the packet on. Defaults to `0`.

`flag` is one of `"reliable"`, `"unsequenced"`, or `"unreliable"`. Reliable
packets are guaranteed to arrive, and arrive in the order in which they are sent.
Unsequenced packets are unreliable and have no guarantee on the order they
arrive. Defaults to reliable.

### `peer:disconnect([data])`
Requests a disconnection from the peer. The message is sent on the next
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

### `peer:throttle_configure(interval, acceleration, deceleration)`
Changes the probability at which unreliable packets should not be dropped.

Parameters:

 * `interval` interval in milliseconds to measure lowest mean RTT
 * `acceleration` rate at which to increase throttle probability as mean RTT
   declines
 * `deceleration` rate at which to decrease throttle probability as mean RTT
   increases

### `peer:receive()`
Attempts to dequeue an incoming packet for this peer.
Returns `nil` if there are no packets waiting. Otherwise returns two values:
the string representing the packet data, and the channel the packet came from.

### `peer:round_trip_time(value)`
Returns or sets the current round trip time (i.e. ping). If value is nil
the current value of the peer is returned. Otherwise the value roundTripTime
is set to the specified value and returned.

Enet performs some filtering on
the round trip times and it takes some time until the parameters are
accurate.

### `peer:round_trip_time(value)`
Returns or sets the round trip time of the previous round trip time
computation. If value is nil the current value of the peer is returned.
Otherwise the value lastRoundTripTime is set to the specified value and
returned. 

Enet performs some filtering on the round trip times and it takes
some time until the parameters are accurate.

### `limit, minimum, maximum peer:timeout(limit, minimum, maximum)`
Returns or sets the parameters when a timeout is detected. This is happens
either after a fixed timeout or a variable timeout of time that takes the
round trip time into account. The former is specified with the `maximum`
parameter.

Parameters:

  * `limit` a factor that is multiplied with a value that based on the 
	average round trip time to compute the timeout limit
  * `minimum` timeout value in milliseconds that a reliable packet has to
	be acknowledged if the variable timeout limit was exceeded
  * `maximum` fixed timeout in milliseconds for which any packet has to be acknowledged

See official enet documentation for detailed description.

</div>

<a name="license"></a>
## License (MIT)

    Copyright (C) 2011 by Leaf Corcoran
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

<a name="contact"></a>
## Contact

Author: Leaf Corcoran ([leafo](http://github.com/leafo)) ([@moonscript](http://twitter.com/moonscript))  
Email: <leafot@gmail.com>  
Homepage: <http://leafo.net>  

