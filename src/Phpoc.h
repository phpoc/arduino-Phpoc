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

#ifndef Phpoc_h
#define Phpoc_h

#include <IPAddress.h>
#include <IP6Address.h>

#define INCLUDE_LIB_V1

#include <Sppc.h>
#include <PhpocClient.h>
#include <PhpocServer.h>
#include <PhpocEmail.h>
#include <PhpocDateTime.h>

class PhpocClass
{
	private:
		/* SPI priviate member functions */
#ifdef INCLUDE_LIB_V1
		uint16_t spi_cmd_status(void);
		uint16_t spi_cmd_sync(void);
		uint16_t spi_resync(void);
		int spi_cmd_txlen(void);
		int spi_cmd_rxfree(void);
		int spi_wait(int len, int ms);
#endif

	private:
#ifdef INCLUDE_LIB_V1
		uint16_t php_request(const char *wbuf, int wlen);
		int php_write_data(const uint8_t *wbuf, int wlen, boolean pgm);
#endif

	public:
		int tcpIoctlReadInt(const __FlashStringHelper *args, int sock_id);
		int getHostByName(const char *hostname, IPAddress &ipaddr, int wait_ms = 2000);
		int getHostByName6(const char *hostname, IP6Address &ip6addr, int wait_ms = 2000);
		uint16_t readInt(); /* read & parse short integer */
		IPAddress readIP(); /* read & parse IP address */
		IP6Address readIP6(); /* read & parse IPv6 address */
		void logFlush(uint8_t id);
		void logPrint(uint8_t id);

	public:
		uint16_t command(const __FlashStringHelper *format, ...);
		uint16_t command(const char *format, ...);
		int write(const __FlashStringHelper *wstr);
		int write(const char *wstr);
		int write(const uint8_t *wbuf, size_t wlen);
		int read(uint8_t *rbuf, size_t rlen);

	public:
		int beginIP4();
		int beginIP6();

	public:
		/* Arduino Ethernet compatible public member functions */
		int begin(uint16_t init_flags = 0x0000);
		int maintain();
		IPAddress localIP();
		IPAddress subnetMask();
		IPAddress gatewayIP();
		IPAddress dnsServerIP();

	public:
		IP6Address localIP6();  /* IPv6 link local address */
		IP6Address globalIP6(); /* IPv6 global unicast address */
		IP6Address gatewayIP6();
		IP6Address dnsServerIP6();
		int globalPrefix6();
};

extern PhpocClass Phpoc;

#endif
