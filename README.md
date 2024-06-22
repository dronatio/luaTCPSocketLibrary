# tcp_client
### Use
First you need to require the `tcp_client`
~~~lua
tcp_client = require("tcp_client")
~~~
then you init the library
~~~lua
tcp_client.init()
~~~
to start using you need to create a new socket
example:
~~~lua
newsocket = tcp_client.create_socket()
~~~
to connect into a server you need to use the `connect` function
~~~lua
tcp_client.connect(newsocket, "127.0.0.1", 1212)
~~~
to send data use the `send` function
example:
~~~lua
tcp_client.send(newsocket, "Hello server!!!")
~~~
then to manipulate the data received you need so set the function to this and abylitate the async function
example:
~~~lua
tcp_client.set_receive_callback(function(data)
    print("received data from server!!!")
    print("data received: " .. data)
end)
tcp_client.receive_async(newsocket)
~~~
you can define a function when you are disconected from the server
example:
~~~lua
tcp_client.set_disconnect_callback(function()
    print("disconected")
end)
~~~
to disconnect use this `tcp_client.close_socket(<socket>)`
example:
~~~lua
tcp_client.close_socket(newsocket)
~~~

example code:
~~~lua
tcp_client = require("tcp_client")

tcp_client.init()

sock = tcp_client.create_socket()

tcp_client.connect(sock, "127.0.0.1", 1212)

tcp_client.set_receive_callback(function(data)
	print("Received from server:", data)
end)
tcp_client.receive_async(sock)

tcp_client.set_disconnect_callback(function(message)
	print("disconnected")
end)

tcp_client.send(sock, "Hello Server")

while true do
    prmpt = io.read()
    if prmpt=="exit" then
        tcp_client.close_socket(sock)
        tcp_client.cleanup()
    elseif prmpt=="" then
    else
	    tcp_client.send(sock, prmpt)
	end
end
~~~

### Comands list
Command | Syntax | Function
:--------:|:--------:|:---------:
`init`| `tcp_client.init()` | start the library
`create_socket`|`variable = tcp_client.create_socket()`| create a new socket returning the socket to be stored into a variable
`connect`|`tcp_socket.connect(<socket>, <server ip>, <server port>)`| connect the socket into a server with a specified port
`send`|`tcp_socket.send(<socket>, <message>)`| send a message to the server in the especified socket
`set_receive_callback`|`tcp_client.set_receive_callback(<function>)`| set the function to invoque with the data who received, the max buffer is 1024 bytes or 1024 characters
`receive_async`|`tcp_client.receive_async(<socket>)`| activate the execution of the configured function when received data fron the server to the especified socket
`set_disconnect_callback`|`tcp_client.set_disconnect_callback(<function>)`| set the function to be invoked when a socket is disconnected from the server
`close_socket`|`tcp_client.close_socket(<socket>)`| close the connection of the especified socket
`cleanup`|`tcp_client.cleanup()`| clean everything configured during the execution, after that you need to init again the library

# tcp_server
### Use
First you need to require the `tcp_server`
~~~lua
tcp_server = require("tcp_server")
~~~
then you init the library
~~~lua
tcp_server.init()
~~~
to start using you need to create a new socket
example:
~~~lua
newserversocket = tcp_server.create_socket()
~~~
to configurate the ip and the port to host you need to use the `bind` function
~~~lua
tcp_server.bind(newserversocket, "127.0.0.1", 1212)
~~~
then to start the server use the function `listen`, don't forget to specify the maximum of connectons
~~~lua
tcp_server.listen(newserversocket, 32)
~~~
to send data to especified client use the `send` function
example:
~~~lua
tcp_server.send(clientsocket, "Hello client!!!")
~~~
for configurate the function to invoke when a new client connected use theses functions
example:
~~~lua
tcp_server.set_accept_callback(function(client_sock)
    print("new socket: " .. client_sock)
    -- Return 1 or true to accept the connection
    -- Return 0 or false to refuse the connection (disconnect the new client socket)
    return 1
end)
tcp_server.set_accept_success_callback(function(client_sock)
    -- When you accept the new client socket this function is invoked with the same client_sock
    print("Client connected: " .. client_sock)
end)
tcp_client.accept_async(newserversocket)
~~~
you can define a function when any client is disconnected
example:
~~~lua
tcp_server.set_disconnect_callback(function(client_sock)
	print("Client socket: " .. client_sock .. " disconnected")
end)
~~~
and to set the function to be invoked when you receive data from any client use this function
~~~lua
tcp_server.set_receive_callback(function(client_sock, data)
    print("Received data:", data)
	print("socket who send", client_sock)
end)
~~~
to disconnect a socket use this `tcp_server.close_socket(<socket>)`
example:
~~~lua
tcp_server.close_socket(client_sock)
~~~

example code:
~~~lua
tcp_server = require("tcp_server")
socketconnected = nil

tcp_server.init()

server_sock = tcp_server.create_socket()

tcp_server.bind(server_sock, "127.0.0.1", 1212)

tcp_server.listen(server_sock, 32)

tcp_server.set_accept_callback(function(client_sock)
	print("new socket: " .. client_sock)
    return 1
end)
tcp_server.set_accept_success_callback(function(client_sock)
    print("Client connected: " .. client_sock)
	socketconnected = client_sock
end)

tcp_server.set_receive_callback(function(client_sock, data)
    print("Received data:", data)
	print("socket who send", client_sock)
end)

tcp_server.set_disconnect_callback(function(client_sock)
	print("Client socket: " .. client_sock .. " disconnected")
end)

tcp_server.accept_async(server_sock)
while true do
	prmpt = io.read()
	if prmpt=="exit" then
		tcp_server.close_socket(socketconnected)
	elseif prmpt=="" then
	else
		tcp_server.send(socketconnected, prmpt)
	end
end
~~~

### Comands list
Command | Syntax | Function
:--------:|:--------:|:---------:
`init`| `tcp_server.init()` | start the library
`create_socket`|`variable = tcp_server.create_socket()`| create a new socket returning the socket to be stored into a variable
`bind`|`tcp_server.bind(<socket>, <ip>, <port>)`| configure the ip and port of a especified socket
`listen`|`tcp_server.listen(<socket>, <max connections>)`| start the server with a limit of connections
`send`|`tcp_server.send(<socket>, <message>)`| send a message to the especified socket
`set_accept_callback`|`tcp_server.set_accept_callback(<function>)`| set the function to be invoked with the client socket when a client is connected, you need to return true or false to accept or refuse the connection
`set_accept_success_callback`|`tcp_server.set_accept_success_callback(<function>)`| set the function to invoque with the client socket when you accept teh connected client
`accept_async`|`tcp_server.accept_async(<server socket>)`| activate the execution of the configured function when a client connect
`set_disconnect_callback`|`tcp_server.set_disconnect_callback(<function>)`| set the function to be invoked with the client socket when a client is disconnected
`set_receive_callback`|`tcp_server.set_receive_callback(<function>)`| set the function to invoque with the client socket and data who received, the max buffer is 1024 bytes or 1024 characters
`close_socket`|`tcp_server.close_socket(<socket>)`| close the connection with a client socket or a server socket
`cleanup`|`tcp_server.cleanup()`| clean everything configured during the execution, after that you need to init again the library