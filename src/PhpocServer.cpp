/*
 * Copyright (c) 2016, Sollae Systems. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Sollae Systems nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOLLAE SYSTEMS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SOLLAE SYSTEMS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Phpoc.h>

uint16_t PhpocServer::server_port[MAX_SOCK_TCP];
const char *PhpocServer::server_ws_path[MAX_SOCK_TCP];

PhpocServer::PhpocServer(uint16_t port)
{
	listen_port = port;
	server_api = SERVER_API_TCP;
}

void PhpocServer::listen()
{
	uint8_t sock_id, state;

	/* create listening socket & initialize NC(Net Cache) */
	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		PhpocClient client(sock_id);

		if(!(client.init_flags & (1 << sock_id)))
		{
			Phpoc.command(F("tcp%u ioctl close"), sock_id);
			client.init_flags |= (1 << sock_id);
			break;
		}

#ifdef INCLUDE_NET_CACHE
		nc_update(sock_id, 0);
		state = nc_tcp_state[sock_id];
#else
		state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
#endif

		if((state == TCP_CLOSED) && !Sppc.errno && !client.available())
			break;
	}

	if(sock_id == MAX_SOCK_TCP)
	{
#ifdef PF_LOG_NET
		//if((Sppc.flags & PF_LOG_NET) && Serial)
		//	sppc_printf(F("log> phpoc_server : socket not available\r\n"));
#endif
		return;
	}

	switch(server_api)
	{
		case SERVER_API_TCP:
			Phpoc.command(F("tcp%u ioctl set api tcp"), sock_id);
			break;

		case SERVER_API_TELNET:
			Phpoc.command(F("tcp%u ioctl set api telnet"), sock_id);
			break;

		case SERVER_API_WS:
			Phpoc.command(F("tcp%u ioctl set api ws"), sock_id);
			Phpoc.command(F("tcp%u ioctl set ws path %s"), sock_id, listen_ws_path);
			Phpoc.command(F("tcp%u ioctl set ws proto %s"), sock_id, listen_ws_proto);
			Phpoc.command(F("tcp%u ioctl set ws mode %d"), sock_id, ws_mode);
			break;
	}

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		Phpoc.command(F("tcp%u listen %u"), sock_id, listen_port);
	else
#endif
	{
		Phpoc.command(F("tcp%u bind ANY %u"), sock_id, listen_port);
		Phpoc.command(F("tcp%u listen"), sock_id);
	}

	if(!Sppc.errno)
	{
		server_port[sock_id] = listen_port;

		if(server_api == SERVER_API_WS)
			server_ws_path[sock_id] = listen_ws_path;

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			sppc_printf(F("log> phpoc_server : listen %d/%d\r\n"), sock_id, listen_port);
#endif

#ifdef INCLUDE_NET_CACHE
		nc_init(sock_id, TCP_LISTEN);
#endif
	}
}

void PhpocServer::begin()
{
	server_api = SERVER_API_TCP;
	listen();
}

void PhpocServer::beginTelnet()
{
	server_api = SERVER_API_TELNET;
	listen();
}

void PhpocServer::beginWebSocket(const char *path, const char *proto)
{
	if(!path || !path[0])
		return;

	server_api = SERVER_API_WS;
	listen_ws_path = path;

	if(proto && proto[0])
		listen_ws_proto = proto;
	else
		listen_ws_proto = "text.phpoc";

	listen();
}

void PhpocServer::beginWebSocketText(const char *path, const char *proto)
{
	ws_mode = 0;
	beginWebSocket(path, proto);
}

void PhpocServer::beginWebSocketBinary(const char *path, const char *proto)
{
	ws_mode = 1;
	beginWebSocket(path, proto);
}

void PhpocServer::accept()
{
	uint8_t sock_id, state, listening;

	if(!listen_port)
		return;

	listening = 0;

	/* find listening socket & close disconnected socket */
	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		if(listen_port == server_port[sock_id])
		{
			PhpocClient client(sock_id);

			if((server_api == SERVER_API_WS) && (listen_ws_path != server_ws_path[sock_id]))
				continue;

#ifdef INCLUDE_NET_CACHE
			nc_update(sock_id, 0);
			state = nc_tcp_state[sock_id];
#else
			state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
#endif

			if(state == TCP_LISTEN)
				listening = 1;
			else
			if((state < TCP_LISTEN) || (state > TCP_CONNECTED))
			{
				/* PHPoC firmware doesn't support tcp CLOSE_WAIT state */
				if(!client.available())
				{
					client.stop(); /* conn_flags is cleared in function client.stop() */
					server_port[sock_id] = 0;
					server_ws_path[sock_id] = NULL;
				}
			}
			else
			if(state == TCP_CONNECTED)
			{
				if(!(client.conn_flags & (1 << sock_id)))
				{
#ifdef PF_LOG_NET
					if((Sppc.flags & PF_LOG_NET) && Serial)
						sppc_printf(F("log> phpoc_server: connected %d\r\n"), sock_id);
#endif
					client.conn_flags |= (1 << sock_id);
				}
			}
		}
	}

	if(!listening)
		listen();
}

PhpocClient PhpocServer::available()
{
	uint8_t sock_id;

	accept();

	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		if(listen_port == server_port[sock_id])
		{
			PhpocClient client(sock_id);

			if((server_api == SERVER_API_WS) && (listen_ws_path != server_ws_path[sock_id]))
				continue;

			if(client.available())
				return client;
		}
	}

	return PhpocClient(MAX_SOCK_TCP);
}

size_t PhpocServer::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t PhpocServer::write(const uint8_t *buf, size_t size)
{
	uint8_t sock_id, state;
	uint16_t wlen;

	wlen = 0;

	accept();

	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		if(listen_port == server_port[sock_id])
		{
			PhpocClient client(sock_id);

			if((server_api == SERVER_API_WS) && (listen_ws_path != server_ws_path[sock_id]))
				continue;

#ifdef INCLUDE_NET_CACHE
			nc_update(sock_id, 0);
			state = nc_tcp_state[sock_id];
#else
			state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
#endif

			if(state == TCP_CONNECTED)
				wlen += client.write(buf, size);
		}
	}

	return wlen;
}

