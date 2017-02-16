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

PhpocServer::PhpocServer(uint16_t port)
{
	listen_port = port;
}

void PhpocServer::session_loop_tcp()
{
	uint8_t sock_id, listen_count, state;

	listen_count = 0;

	/* find listening socket & close disconnected socket */
	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		if(listen_port == server_port[sock_id])
		{
			PhpocClient client(sock_id);

#ifdef INCLUDE_PHPOC_CACHE
			client.update_cache(sock_id);
			state = client.state_32ms[sock_id];
#else
			if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
				state = Phpoc.readInt();
			else
				continue;
#endif

			if(state == TCP_LISTEN)
				listen_count++;
			else
			if((state < TCP_LISTEN) || (state > TCP_CONNECTED))
			{
				if(!client.available())
				{
					client.stop();
					server_port[sock_id] = 0;
				}
			}
		}
	}

	if(listen_count)
		return;

	/* create listening socket & initialize phpoc cache */
	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
#ifdef INCLUDE_PHPOC_CACHE
		PhpocClient::update_cache(sock_id);
		state = PhpocClient::state_32ms[sock_id];
#else
		if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
			state = Phpoc.readInt();
		else
			continue;
#endif

		if(state == TCP_CLOSED)
		{
			switch(server_api)
			{
				case SERVER_API_TCP:
					if(Phpoc.command(F("tcp%u set api tcp"), sock_id) < 0)
						continue;
					break;

				case SERVER_API_TELNET:
					if(Phpoc.command(F("tcp%u set api telnet"), sock_id) < 0)
						continue;
					break;

				case SERVER_API_WS:
					if(Phpoc.command(F("tcp%u set api ws"), sock_id) < 0)
						continue;
					if(Phpoc.command(F("tcp%u set ws path %s"), sock_id, ws_path) < 0)
						continue;
					if(Phpoc.command(F("tcp%u set ws proto text.phpoc"), sock_id) < 0)
						continue;
					if(Phpoc.command(F("tcp%u set ws mode 0"), sock_id) < 0) /* Unicode text mode */
						continue;
					break;

				default:
					continue;
			}

			if(Phpoc.command(F("tcp%u listen %u"), sock_id, listen_port) >= 0)
			{
				server_port[sock_id] = listen_port;

#ifdef PF_LOG_NET
				if((Phpoc.flags & PF_LOG_NET) && Serial)
				{
					Serial.print(F("log> phpoc_server: listen "));
					Serial.print(sock_id);
					Serial.print('/');
					Serial.print(listen_port);
					Serial.println();
				}
#endif

#ifdef INCLUDE_PHPOC_CACHE
				PhpocClient::state_32ms[sock_id] = TCP_LISTEN;
				PhpocClient::rxlen_32ms[sock_id] = 0;
				PhpocClient::read_cache_len[sock_id] = 0;
				PhpocClient::write_cache_len[sock_id] = 0;
#endif
				return;
			}
		}
	}
}

void PhpocServer::session_loop_ssl()
{
	PhpocClient client(SOCK_ID_SSL);
	uint8_t sock_id, state;
	int len;

	sock_id = SOCK_ID_SSL;

#ifdef INCLUDE_PHPOC_CACHE
	client.update_cache(sock_id);
	state = client.state_32ms[sock_id];
#else
	if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
		state = Phpoc.readInt();
	else
		return;
#endif

	if(state == TCP_LISTEN)
		return;
	else
	if((state < TCP_LISTEN) || ((state > TCP_CONNECTED) && (state < SSL_STOP)))
	{
		if(!client.available())
		{
			client.stop();
			server_port[sock_id] = 0;
		}
	}

	if(state == TCP_CLOSED)
	{
		if(Phpoc.command(F("tcp%u set api ssl"), sock_id) < 0)
			return;

		if(Phpoc.command(F("tcp%u set ssl method tls1_server"), sock_id) < 0)
			return;

		if(Phpoc.command(F("tcp%u listen %u"), sock_id, listen_port) >= 0)
		{
			server_port[sock_id] = listen_port;

#ifdef PF_LOG_NET
			if((Phpoc.flags & PF_LOG_NET) && Serial)
			{
				Serial.print(F("log> phpoc_server: listen "));
				Serial.print(sock_id);
				Serial.print('/');
				Serial.print(listen_port);
				Serial.println();
			}
#endif

#ifdef INCLUDE_PHPOC_CACHE
			PhpocClient::state_32ms[sock_id] = TCP_LISTEN;
			PhpocClient::rxlen_32ms[sock_id] = 0;
			PhpocClient::read_cache_len[sock_id] = 0;
			PhpocClient::write_cache_len[sock_id] = 0;
#endif
			return;
		}
	}
}

void PhpocServer::session_loop_ssh()
{
	PhpocClient client(SOCK_ID_SSH);
	uint8_t auth_buf[SSH_AUTH_SIZE + 1];
	uint8_t sock_id, state;
	int len;

	sock_id = SOCK_ID_SSH;

#ifdef INCLUDE_PHPOC_CACHE
	client.update_cache(sock_id);
	state = client.state_32ms[sock_id];
#else
	if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
		state = Phpoc.readInt();
	else
		return;
#endif

	if(state == TCP_LISTEN)
		return;
	else
	if((state < TCP_LISTEN) || ((state > TCP_CONNECTED) && (state < SSH_STOP)))
	{
		if(!client.available())
		{
			client.stop();
			server_port[sock_id] = 0;
		}
	}
	else
	if(state == SSH_AUTH)
	{
		if(ssh_username)
		{
			if(Phpoc.command(F("tcp%u get ssh username"), sock_id) < 0)
				return;

			len = Phpoc.read(auth_buf, SSH_AUTH_SIZE);
			auth_buf[len] = 0x00;

			if(!len || strcmp((char *)auth_buf, ssh_username))
			{
				Phpoc.command(F("tcp%u set ssh auth reject"), sock_id);
				return;
			}
		}

		if(ssh_password)
		{
			if(Phpoc.command(F("tcp%u get ssh password"), sock_id) < 0)
				return;

			len = Phpoc.read(auth_buf, SSH_AUTH_SIZE);
			auth_buf[len] = 0x00;

			if(!len || strcmp((char *)auth_buf, ssh_password))
			{
				Phpoc.command(F("tcp%u set ssh auth reject"), sock_id);
				return;
			}
		}

		Phpoc.command(F("tcp%u set ssh auth accept"), sock_id);
	}

	if(state == TCP_CLOSED)
	{
		if(Phpoc.command(F("tcp%u set api ssh"), sock_id) < 0)
			return;

		if(Phpoc.command(F("tcp%u listen %u"), sock_id, listen_port) >= 0)
		{
			server_port[sock_id] = listen_port;

#ifdef PF_LOG_NET
			if((Phpoc.flags & PF_LOG_NET) && Serial)
			{
				Serial.print(F("log> phpoc_server: listen "));
				Serial.print(sock_id);
				Serial.print('/');
				Serial.print(listen_port);
				Serial.println();
			}
#endif

#ifdef INCLUDE_PHPOC_CACHE
			PhpocClient::state_32ms[sock_id] = TCP_LISTEN;
			PhpocClient::rxlen_32ms[sock_id] = 0;
			PhpocClient::read_cache_len[sock_id] = 0;
			PhpocClient::write_cache_len[sock_id] = 0;
#endif
			return;
		}
	}
}

void PhpocServer::begin()
{
	server_api = SERVER_API_TCP;
	session_loop_tcp();
}

void PhpocServer::beginTelnet()
{
	server_api = SERVER_API_TELNET;
	session_loop_tcp();
}

void PhpocServer::beginWebSocket(const char *path)
{
	server_api = SERVER_API_WS;

	if(path && path[0])
		ws_path = path;
	else
		ws_path = "remocon";

	session_loop_tcp();
}

void PhpocServer::beginSSL()
{
	server_api = SERVER_API_SSL;

	/* - crypto computation time
	 *   1024bit RSA private key op : 75ms
	 *   2048bit RSA private key op : 404ms
	 * - spi_wait time should be longer than crypto computation time.
	 */
	Phpoc.spi_wait_ms = 600; /* 0.6 second */

	session_loop_ssl();
}

void PhpocServer::beginSSH(const char *username, const char *password)
{
	server_api = SERVER_API_SSH;

	if(username && username[0])
		ssh_username = username;
	else
		ssh_username = NULL;

	if(password && password[0])
		ssh_password = password;
	else
		ssh_password = NULL;

	/* - crypto computation time
	 *   DH new key : 270ms
	 *   1024bit RSA private key op : 75ms
	 *   2048bit RSA private key op : 404ms
	 * - spi_wait time should be longer than crypto computation time.
	 */
	Phpoc.spi_wait_ms = 900; /* 0.9 second */

	session_loop_ssh();
}

PhpocClient PhpocServer::available()
{
	PhpocClient client;
	uint8_t sock_id;

	switch(server_api)
	{
		case SERVER_API_TCP:
		case SERVER_API_TELNET:
		case SERVER_API_WS:
			session_loop_tcp();

			for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
			{
				if(listen_port == server_port[sock_id])
				{
					client.sock_id = sock_id;

					if(client.available())
						return client;
				}
			}
			break;

		case SERVER_API_SSL:
			session_loop_ssl();

			client.sock_id = SOCK_ID_SSL;

			if(client.available())
				return client;
			break;

		case SERVER_API_SSH:
			session_loop_ssh();

			client.sock_id = SOCK_ID_SSH;

			if(client.available())
				return client;
			break;
	}

	return PhpocClient(MAX_SOCK_TCP);
}

size_t PhpocServer::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t PhpocServer::write(const uint8_t *buf, size_t size)
{
	PhpocClient client;
	uint8_t sock_id, state;
	uint16_t wlen;

	wlen = 0;

	switch(server_api)
	{
		case SERVER_API_TCP:
		case SERVER_API_TELNET:
		case SERVER_API_WS:
			session_loop_tcp();

			for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
			{
				if(listen_port == server_port[sock_id])
				{
					client.sock_id = sock_id;

#ifdef INCLUDE_PHPOC_CACHE
					PhpocClient::update_cache(sock_id);
					state = PhpocClient::state_32ms[sock_id];
#else
					if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
						state = Phpoc.readInt();
					else
						continue;
#endif

					if(state == TCP_CONNECTED)
						wlen += client.write(buf, size);
				}
			}
			break;

		case SERVER_API_SSL:
			session_loop_ssl();

			client.sock_id = SOCK_ID_SSL;
#ifdef INCLUDE_PHPOC_CACHE
			PhpocClient::update_cache(SOCK_ID_SSL);
			state = PhpocClient::state_32ms[SOCK_ID_SSL];
#else
			if(Phpoc.command(F("tcp%u get state"), SOCK_ID_SSL) > 0)
				state = Phpoc.readInt();
			else
				break;
#endif

			if(state == SSL_CONNECTED)
				wlen += client.write(buf, size);
			break;

		case SERVER_API_SSH:
			session_loop_ssh();

			client.sock_id = SOCK_ID_SSH;

#ifdef INCLUDE_PHPOC_CACHE
			PhpocClient::update_cache(SOCK_ID_SSH);
			state = PhpocClient::state_32ms[SOCK_ID_SSH];
#else
			if(Phpoc.command(F("tcp%u get state"), SOCK_ID_SSH) > 0)
				state = Phpoc.readInt();
			else
				break;
#endif

			if(state == SSH_CONNECTED)
				wlen += client.write(buf, size);
			break;
	}

	return wlen;
}

