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
#include <PhpocClient.h>
#include <PhpocServer.h>
#include <PhpocEmail.h>
#include <PhpocDateTime.h>

/* PHPoC flags */
#define PF_SHIELD  0x01 // PHPoC shield installed
#define PF_IP6     0x02 // IPv6 enabled
#define PF_LOG_SPI 0x10
#define PF_LOG_NET 0x20
#define PF_LOG_APP 0x40

/* PHPoC shield runtime error */
#define PE_NO_SHIELD 1
#define PE_TIMEOUT   2
#define PE_PROTOCOL  3

/* PHPoC shield state */
#define PS_NO_SHIELD      0 // PHPoC Shield not found
#define PS_NET_STOP       1 // ethernet enabled, but unplugged
#define PS_NET_SCAN       2 // wifi enabled, but doesn't associated
#define PS_NET_ADDRESS    3 // acquiring network address (ip address)
#define PS_NET_CONNECTED  4 // ethernet or wifi ready

#define VSP_COUNT_LIMIT 64 // string + NULL(0x00)

class PhpocClass
{
	private:
		/* SPI priviate member functions */
		uint16_t spi_request(int req);
		uint16_t spi_resync();
		int spi_wait(int len, int ms);
		int spi_cmd_status();
		int spi_cmd_txlen();
		int spi_cmd_rxbuf();
		int spi_cmd_sync();
		int spi_cmd_read(uint8_t *rbuf, size_t rlen);
		int spi_cmd_write(const uint8_t *wbuf, size_t wlen, boolean pgm);
		int api_write_command(const char *wbuf, int wlen);
		int api_write_data(const uint8_t *wbuf, int wlen, boolean pgm);

	private:
		int vsprintf(char *str, const __FlashStringHelper *format, va_list args);
		int vsprintf(char *str, const char *format, va_list args);
		IPAddress inet_aton(const char *str);

	public:
		int sprintf(char *str, const __FlashStringHelper *format, ...);
		int sprintf(char *str, const char *format, ...);

	public:
		uint8_t flags;
		uint16_t spi_wait_ms;
		uint16_t pkg_ver_id;
		int command(const __FlashStringHelper *format, ...);
		int command(const char *format, ...);
		int write(const __FlashStringHelper *wstr);
		int write(const char *wstr);
		int write(const uint8_t *wbuf, size_t wlen);
		int read(uint8_t *rbuf, size_t rlen);
		uint16_t readInt();
		IPAddress readIP();
		IP6Address readIP6();
		int getHostByName(const char *hostname, IPAddress &ipaddr, int wait_ms = 2000);
		int getHostByName6(const char *hostname, IP6Address &ip6addr, int wait_ms = 2000);
		void logFlush(uint8_t id);
		void logPrint(uint8_t id);
		int beginIP4();
		int beginIP6();

	public:
		/* Arduino Ethernet compatible public member functions */
		int begin(uint8_t init_flags = 0x00);
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
