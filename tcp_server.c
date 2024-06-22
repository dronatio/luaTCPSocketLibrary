#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

// Lua state and callback references
static lua_State *gL = NULL;
static int gAcceptCallbackRef = LUA_NOREF;
static int gAcceptSuccessCallbackRef = LUA_NOREF;
static int gReceiveCallbackRef = LUA_NOREF;
static int gDisconnectCallbackRef = LUA_NOREF;

// Initialize Winsock
static int l_init(lua_State *L) {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        lua_pushstring(L, "WSAStartup failed");
        lua_error(L);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Create a TCP socket
static int l_create_socket(lua_State *L) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        lua_pushstring(L, "Socket creation failed");
        lua_error(L);
    }
    lua_pushlightuserdata(L, (void *)sock);
    return 1;
}

// Bind the socket to an IP address and port
static int l_bind(lua_State *L) {
    SOCKET sock = (SOCKET)lua_touserdata(L, 1);
    const char *ip = luaL_checkstring(L, 2);
    int port = luaL_checkinteger(L, 3);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    int iResult = bind(sock, (struct sockaddr *)&server, sizeof(server));
    if (iResult == SOCKET_ERROR) {
        lua_pushstring(L, "Bind failed");
        lua_error(L);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Listen for incoming connections
static int l_listen(lua_State *L) {
    SOCKET sock = (SOCKET)lua_touserdata(L, 1);
    int backlog = luaL_checkinteger(L, 2);

    int iResult = listen(sock, backlog);
    if (iResult == SOCKET_ERROR) {
        lua_pushstring(L, "Listen failed");
        lua_error(L);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Set the Lua callback function for accepting connections
static int l_set_accept_callback(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        gAcceptCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        gL = L;
    }
    return 0;
}

// Set the Lua callback function for successful connections
static int l_set_accept_success_callback(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        gAcceptSuccessCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        gL = L;
    }
    return 0;
}

// Set the Lua callback function for receiving data
static int l_set_receive_callback(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        gReceiveCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        gL = L;
    }
    return 0;
}

// Set the Lua callback function for client disconnection
static int l_set_disconnect_callback(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        gDisconnectCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        gL = L;
    }
    return 0;
}

// Internal function to invoke the Lua callback
static int invoke_lua_callback(int callbackRef, SOCKET clientSock, const char *data) {
    if (gL != NULL && callbackRef != LUA_NOREF) {
        lua_rawgeti(gL, LUA_REGISTRYINDEX, callbackRef);
        lua_pushlightuserdata(gL, (void *)clientSock);
        lua_pushstring(gL, data);
        if (lua_pcall(gL, 2, 1, 0) != LUA_OK) {
            const char *error = lua_tostring(gL, -1);
            printf("Error calling Lua callback: %s\n", error);
            lua_pop(gL, 1);
            return 0; // Indicate failure
        }
        int result = lua_toboolean(gL, -1);
        lua_pop(gL, 1);
        return result; // Return the result of the callback
    }
    return 0; // Default to not accepting if callback is not set
}

// Internal function to handle incoming connections
static void handle_client(void *param) {
    SOCKET clientSock = (SOCKET)param;
    char buffer[1024];
    int iResult;

    do {
        iResult = recv(clientSock, buffer, sizeof(buffer), 0);
        if (iResult > 0) {
            buffer[iResult] = '\0';
            invoke_lua_callback(gReceiveCallbackRef, clientSock, buffer);
        } else if (iResult == 0) {
            printf("Connection closed\n");
        } else {
            int resulterrorgeted = WSAGetLastError();
            if (resulterrorgeted == 10054) {
                printf("Connection exited\n");
                iResult = 0;
            }
        }
    } while (iResult > 0);

    // Invoke the disconnect callback when the client disconnects
    invoke_lua_callback(gDisconnectCallbackRef, clientSock, "Client disconnected");

    closesocket(clientSock);
}

// Internal function to accept connections and handle them asynchronously
static void accept_connections(void *param) {
    SOCKET serverSock = (SOCKET)param;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (1) {
        SOCKET clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSock == INVALID_SOCKET) {
            printf("Accept failed with error: %d\n", WSAGetLastError());
            continue;
        }

        // Invoke the accept callback to decide whether to accept the connection
        if (invoke_lua_callback(gAcceptCallbackRef, clientSock, "New client connected")) {
            // If accepted, invoke the success callback with the client socket
            if (gL != NULL && gAcceptSuccessCallbackRef != LUA_NOREF) {
                lua_rawgeti(gL, LUA_REGISTRYINDEX, gAcceptSuccessCallbackRef);
                lua_pushlightuserdata(gL, (void *)clientSock);
                if (lua_pcall(gL, 1, 0, 0) != LUA_OK) {
                    const char *error = lua_tostring(gL, -1);
                    printf("Error calling Lua success callback: %s\n", error);
                    lua_pop(gL, 1);
                }
            }

            // Handle the client connection asynchronously
            _beginthread(handle_client, 0, (void *)clientSock);
        } else {
            // If not accepted, close the client socket
            closesocket(clientSock);
        }
    }
}

// Accept connections asynchronously
static int l_accept_async(lua_State *L) {
    SOCKET serverSock = (SOCKET)lua_touserdata(L, 1);

    // Start a new thread to accept connections asynchronously
    _beginthread(accept_connections, 0, (void *)serverSock);

    lua_pushboolean(L, 1);
    return 1;
}

// Send data
static int l_send(lua_State *L) {
    SOCKET sock = (SOCKET)lua_touserdata(L, 1);
    const char *data = luaL_checkstring(L, 2);
    int len = strlen(data);

    int iResult = send(sock, data, len, 0);
    if (iResult == SOCKET_ERROR) {
        lua_pushstring(L, "Send failed");
        lua_error(L);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Close the socket
static int l_close_socket(lua_State *L) {
    SOCKET sock = (SOCKET)lua_touserdata(L, 1);
    closesocket(sock);
    lua_pushboolean(L, 1);
    return 1;
}

// Clean up Winsock
static int l_cleanup(lua_State *L) {
    WSACleanup();
    lua_pushboolean(L, 1);
    return 1;
}

// Register the functions
static const struct luaL_Reg mylib[] = {
    {"init", l_init},
    {"create_socket", l_create_socket},
    {"bind", l_bind},
    {"listen", l_listen},
    {"accept_async", l_accept_async},
    {"send", l_send},
    {"close_socket", l_close_socket},
    {"cleanup", l_cleanup},
    {"set_accept_callback", l_set_accept_callback},
    {"set_accept_success_callback", l_set_accept_success_callback},
    {"set_receive_callback", l_set_receive_callback},
    {"set_disconnect_callback", l_set_disconnect_callback},
    {NULL, NULL}
};

int luaopen_tcp_server(lua_State *L) {
    luaL_newlib(L, mylib);
    return 1;
}
