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

#ifndef PhpocClient_h
#define PhpocClient_h

#include <Arduino.h>
#include <Print.h>
#include <Client.h>
#include <IPAddress.h>

#define INCLUDE_PHPOC_CACHE

#ifdef INCLUDE_PHPOC_CACHE
#define READ_CACHE_SIZE 18  /* 3x 6bytes websocket data */
#define WRITE_CACHE_SIZE 16
#endif

#ifdef READ_CACHE_SIZE
#define LINE_BUF_SIZE READ_CACHE_SIZE
#else
#define LINE_BUF_SIZE 32
#endif

#define MAX_SOCK_TCP 6 /* 1 SSL + 1 SSH + 4 TCP */
#define MAX_SOCK_UDP 4

/* socket ID */
#define SOCK_ID_SSL 0 /* tcp0 */
#define SOCK_ID_SSH 1 /* tcp1 */
#define SOCK_ID_TCP 2 /* tcp2/3/4/5 */

/* tcp/ssl/ssh state */
#define TCP_CLOSED     0
#define TCP_LISTEN     1
#define TCP_CONNECTED  4
#define SSL_STOP      11
#define SSL_CONNECTED 19
#define SSH_STOP      11
#define SSH_AUTH      17
#define SSH_CONNECTED 19

#define SSH_AUTH_SIZE 16 /* buffer size for ssh username/password */

class PhpocClient : public Client
{
	private:
#ifdef INCLUDE_PHPOC_CACHE
		static uint32_t tick_32ms[MAX_SOCK_TCP];
		static uint8_t  state_32ms[MAX_SOCK_TCP];
		static uint16_t rxlen_32ms[MAX_SOCK_TCP];
		static uint8_t  read_cache_len[MAX_SOCK_TCP];
		static uint8_t  write_cache_len[MAX_SOCK_TCP];
		static uint8_t  read_cache_buf[MAX_SOCK_TCP][READ_CACHE_SIZE];
		static uint8_t  write_cache_buf[MAX_SOCK_TCP][WRITE_CACHE_SIZE];
		static void update_cache(uint8_t id);
#endif
		static char read_line_buf[LINE_BUF_SIZE + 2];

	private:
		uint8_t sock_id;
		int read_line_from_cache(uint8_t *buf, size_t size);
		int connectSSL_ipstr(const char *ipstr, uint16_t port);
		int connect_ipstr(const char *ipstr, uint16_t port);

	public:
		int connectSSL(IP6Address ip6addr, uint16_t port);
		int connectSSL(IPAddress ipaddr, uint16_t port);
		int connectSSL(const char *host, uint16_t port);
		char *readLine();
		int readLine(uint8_t *buf, size_t size);

	public:
		/* Arduino EthernetClient compatible public member functions */
		PhpocClient();
		PhpocClient(uint8_t id);
		int connect(IP6Address ip6addr, uint16_t port);
		virtual int connect(IPAddress ipaddr, uint16_t port);
		virtual int connect(const char *host, uint16_t port);
		virtual size_t write(uint8_t byte);
		virtual size_t write(const uint8_t *buf, size_t size);
		virtual int available();
		virtual int read();
		virtual int read(uint8_t *buf, size_t size);
		virtual int peek();
		virtual void flush();
		virtual void stop();
		virtual uint8_t connected();
		virtual operator bool();
		virtual bool operator==(const bool value) { return bool() == value; }
		virtual bool operator!=(const bool value) { return bool() != value; }
		virtual bool operator==(const PhpocClient&);
		virtual bool operator!=(const PhpocClient& rhs) { return !this->operator==(rhs); };

		friend class PhpocServer;

		using Print::write;
};

#endif
