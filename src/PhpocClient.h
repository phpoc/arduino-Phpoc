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

#ifdef INCLUDE_LIB_V1
#define INCLUDE_NET_CACHE
#endif

#ifdef INCLUDE_NET_CACHE
#define SOCK_READ_CACHE_SIZE 18  /* 3x 6bytes websocket data */
#define SOCK_WRITE_CACHE_SIZE 16
#endif

#define SOCK_LINE_BUF_SIZE 32

class PhpocClient : public Client
{
	private:
		static char read_line_buf[SOCK_LINE_BUF_SIZE + 2];

	private:
		uint8_t sock_id;
		int read_line_from_cache(uint8_t *buf, size_t size);
		int connectSSL_ipstr(const char *ipstr, uint16_t port);
		int connect_ipstr(const char *ipstr, uint16_t port);

	public:
		static uint8_t conn_flags;
		static uint8_t init_flags;

	public:
		uint16_t command(const __FlashStringHelper *format, ...);
		uint16_t command(const char *format, ...);
		int connectSSL(IP6Address ip6addr, uint16_t port);
		int connectSSL(IPAddress ipaddr, uint16_t port);
		int connectSSL(const char *host, uint16_t port);
		char *readLine(void);
		int readLine(uint8_t *buf, size_t size);
		int availableForWrite(void);

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

		using Print::write;
};

#define NC_FLAG_RENEW_RXLEN 0x01
#define NC_FLAG_RENEW_STATE 0x02
#define NC_FLAG_FLUSH_WRITE 0x04

#ifdef INCLUDE_NET_CACHE
/* NetCache.cpp */
extern uint8_t  nc_tcp_state[MAX_SOCK_TCP];
extern uint16_t nc_tcp_rxlen[MAX_SOCK_TCP];
extern uint8_t  nc_read_len[MAX_SOCK_TCP];
extern uint8_t  nc_write_len[MAX_SOCK_TCP];
extern uint8_t  nc_read_buf[MAX_SOCK_TCP][SOCK_READ_CACHE_SIZE];
extern uint8_t  nc_write_buf[MAX_SOCK_TCP][SOCK_WRITE_CACHE_SIZE];
extern void nc_init(uint8_t id, int tcp_state);
extern void nc_update(uint8_t id, uint8_t flags);
extern int  nc_peek(uint8_t id);
extern int  nc_read(uint8_t id);
extern int  nc_read(uint8_t id, uint8_t *rbuf, size_t rlen);
extern int  nc_read_line(uint8_t id, uint8_t *rbuf, size_t rlen);
extern int  nc_write(uint8_t id, const uint8_t *wbuf, size_t wlen);
#endif

#endif
