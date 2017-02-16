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

#ifndef PhpocServer_h
#define PhpocServer_h

#include <Server.h>

#define SERVER_API_TCP    0
#define SERVER_API_TELNET 1
#define SERVER_API_WS     2
#define SERVER_API_SSL    3
#define SERVER_API_SSH    4

class PhpocServer : public Server
{
	private:
		static uint16_t server_port[MAX_SOCK_TCP];
		uint16_t listen_port;
		uint8_t server_api;
		const char *ws_path;
		const char *ssh_username;
		const char *ssh_password;
		void session_loop_tcp();
		void session_loop_ssl();
		void session_loop_ssh();

	public:
		void beginTelnet();
		void beginWebSocket(const char *path = NULL);
		void beginSSL();
		void beginSSH(const char *username = NULL, const char *password = NULL);

	public:
		/* Arduino EthernetServer compatible public member functions */
		PhpocServer(uint16_t port);
		PhpocClient available();
		virtual void begin();
		virtual size_t write(uint8_t byte);
		virtual size_t write(const uint8_t *buf, size_t size);
		using Print::write;
};

#endif
