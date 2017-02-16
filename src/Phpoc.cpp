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

/* API command & data
 *   0                 1                2                3                4
 *  +--------+--------+----------------+----------------+----------------+---------------
 *  | D 0 0 C|len8..11|    len 0..7    |             csum/crc            | cmd or data...
 *  +--------+--------+----------------+----------------+----------------+---------------
 *  - D : 0 - command, 1 - data
 *  - C : csum/crc select (0 - checksum, 1 - crc)
 *  - len0..11 : command/data length excluding header
 *
 * API response
 *   0                 1                2                3                4
 *  +--------+--------+----------------+----------------+----------------+--------
 *  | 1 0 N C|len8..11|    len 0..7    |             csum/crc            | data...
 *  +--------+--------+----------------+----------------+----------------+--------
 *  - N : command NAK
 *  - C : csum/crc select (0 - checksum, 1 - crc)
 *  - len0..11 : response data length excluding header
 *
 * command NAK response data
 *  - command error
 *    . response data : "Ennn" + human readable string
 *    . nnn : error code ("000" ~ "999")
 *  - command wait
 *    . response data : "Wnnn"
 *    . nnn : wait time in second ("000" ~ "999")
 *
 * csum/crc
 *  - csum : 1's complement sum
 *  - crc : CRC16-CCITT
 *  - zero(0x0000) csum means "csum not computed"
 *  - if csum computed is 0xffff, csum field should be set 0xffff, not 0x0000
 *  - crc : CRC16-CCITT(init - 0x1d0f, divisor - 0x1021)
 *  - set csum to zero(0x0000) before computing csum
 *  - set crc to zero(0x0000) before testing or computing crc
 */

#include <SPI.h>
#include <Phpoc.h>

#define SPI_FLAG_TXB  0x1000 /* data is in tx buffer */
#define SPI_FLAG_RXB  0x2000 /* data is in rx buffer */
#define SPI_FLAG_SYNC 0x8000 /* SPI SYNC ok */
#define SPI_FLAG_SBZ  0x4fff /* should be zero */

#define API_FLAG_DATA  0x80 /* 0 - command, 1 - data */
#define API_FLAG_NAK   0x20 /* 0 - command ok, 1 - command error/wait */
#define API_FLAG_CRC   0x10 /* 0 - 1's complement sum, 1 - CRC16-CCITT */

#define SPI_NSS_PIN 10
#define SPI_WAIT_MS 200 /* 0.2 second */

static const SPISettings SPI_PHPOC_SETTINGS(1000000, MSBFIRST, SPI_MODE3);

/* atoi_u16() doesn't support negative value */

static uint16_t atoi_u16(const char *nptr)
{
	uint16_t u16;

	u16 = 0;

	while(isdigit(*nptr))
	{
		u16 *= 10;
		u16 += (*nptr - '0');
		nptr++;
	}

	return u16;
}

uint16_t PhpocClass::spi_request(int req)
{
	uint16_t resp;

	SPI.beginTransaction(SPI_PHPOC_SETTINGS);
	digitalWrite(SPI_NSS_PIN, LOW);

	SPI.transfer(req >> 8);
	SPI.transfer(req & 0xff);
	resp  = SPI.transfer(0x00) << 8;
	resp |= SPI.transfer(0x00);

	digitalWrite(SPI_NSS_PIN, HIGH);
	SPI.endTransaction();

	return resp;
}

uint16_t PhpocClass::spi_resync()
{
	uint16_t status;

	status = spi_cmd_status();

	if(!(status & SPI_FLAG_SYNC) || (status & SPI_FLAG_SBZ))
	{
#ifdef PF_LOG_SPI
		if((flags & PF_LOG_SPI) && Serial)
			Serial.print(F("log> phpoc_spi: resync spi channel..."));
#endif

		delay(100);          /* wait TIMEOUT reset */
		spi_request(0xf000); /* issue BAD_CMD reset */
		delay(1);
		spi_cmd_sync();
		status = spi_cmd_status();

		if(!(status & SPI_FLAG_SYNC) || (status & SPI_FLAG_SBZ))
		{
#ifdef PF_LOG_SPI
			if((flags & PF_LOG_SPI) && Serial)
				Serial.println(F("failed"));
#endif
			return 0x0000;
		}
		else
		{
#ifdef PF_LOG_SPI
			if((flags & PF_LOG_SPI) && Serial)
				Serial.println(F("success"));
#endif
		}
	}

	return status;
}

int PhpocClass::spi_wait(int len, int ms)
{
	uint16_t next_millis;

	next_millis = (uint16_t)millis() + ms;

	while((next_millis != (uint16_t)millis()) && (spi_cmd_txlen() < len))
		delayMicroseconds(50);

	if(next_millis == (uint16_t)millis())
		return 0;
	else
		return 1;
}

int PhpocClass::spi_cmd_status()
{
	return spi_request(0x0000);
}

int PhpocClass::spi_cmd_txlen()
{
	return spi_request(0x1000) & 0x0fff;
}

int PhpocClass::spi_cmd_rxbuf()
{
	return spi_request(0x2000) & 0x0fff;
}

int PhpocClass::spi_cmd_sync()
{
	return spi_request(0x5a3c);
}

int PhpocClass::spi_cmd_read(uint8_t *rbuf, size_t rlen)
{
	uint16_t req, resp;
	int count;

	req = 0x3000 | rlen;

	SPI.beginTransaction(SPI_PHPOC_SETTINGS);
	digitalWrite(SPI_NSS_PIN, LOW);

	SPI.transfer(req >> 8);
	SPI.transfer(req & 0xff);
	resp  = SPI.transfer(0x00) << 8;
	resp |= SPI.transfer(0x00);

	count = rlen;

	while(count)
	{
		if(rbuf)
			*rbuf++ = SPI.transfer(0x00);
		else
			SPI.transfer(0x00);
		count--;
	}

	digitalWrite(SPI_NSS_PIN, HIGH);
	SPI.endTransaction();

	return rlen;
}

int PhpocClass::spi_cmd_write(const uint8_t *wbuf, size_t wlen, boolean pgm)
{
	uint16_t req, resp;
	int count;

	req = 0x4000 | wlen;

	SPI.beginTransaction(SPI_PHPOC_SETTINGS);
	digitalWrite(SPI_NSS_PIN, LOW);

	SPI.transfer(req >> 8);
	SPI.transfer(req & 0xff);
	resp  = SPI.transfer(0x00) << 8;
	resp |= SPI.transfer(0x00);

	count = wlen;

	while(count)
	{
		if(pgm)
			SPI.transfer(pgm_read_byte(wbuf));
		else
			SPI.transfer(*wbuf);
		wbuf++;
		count--;
	}

	digitalWrite(SPI_NSS_PIN, HIGH);
	SPI.endTransaction();

	return wlen;
}

int PhpocClass::api_write_command(const char *wbuf, int wlen)
{
	int resp_len, api_wait_ms, err;
	uint8_t head[4], msg[8];
	uint16_t status;

	status = spi_resync();

	if(status & SPI_FLAG_SYNC)
	{
		if(status & SPI_FLAG_TXB)
			spi_cmd_read(NULL, spi_cmd_txlen()); /* cleanup slave tx buffer */

		head[0] = 0x00 | (wlen >> 8);  /* data & csum bit clear, len 8..11 */
		head[1] = (uint8_t)wlen;
		*(uint16_t *)(head + 2) = 0x0000; /* csum/crc */
		spi_cmd_write(head, 4, false);

		spi_cmd_write((const uint8_t *)wbuf, wlen, false);

		api_wait_ms = spi_wait_ms;

_again:
		if(!spi_wait(4, api_wait_ms))
		{
#ifdef PF_LOG_SPI
			if((flags & PF_LOG_SPI) && Serial)
				Serial.println(F("log> phpoc_cmd: head wait timeout"));
#endif
			return -PE_TIMEOUT;
		}

		spi_cmd_read(head, 4);

		resp_len  = head[0] << 8;
		resp_len |= head[1];
		resp_len &= 0x0fff;

		if(resp_len && !spi_wait(resp_len, spi_wait_ms))
		{
#ifdef PF_LOG_SPI
			if((flags & PF_LOG_SPI) && Serial)
				Serial.println(F("log> phpoc_cmd: data wait timeout"));
#endif
			return -PE_TIMEOUT;
		}

		if(head[0] & API_FLAG_NAK)
		{
			spi_cmd_read(msg, 4); /* Wnnn */
			msg[4] = 0x00;

			if(msg[0] == 'E')
			{
				err = atoi_u16((const char *)msg + 1);

#ifdef PF_LOG_SPI
				if((flags & PF_LOG_SPI) && Serial)
				{
					resp_len -= 4;

					Serial.print(F("log> phpoc_cmd: error "));
					Serial.print(err);
					Serial.print(F(", "));

					while(resp_len > 0)
					{
						spi_cmd_read(msg, 1);
						Serial.write(*msg);
						resp_len--;
					}

					Serial.println();
				}
#endif
				return -err;
			}
			else
			if(msg[0] == 'W')
			{
				api_wait_ms = atoi_u16((const char *)msg + 1) * 1000;

				/*
				if((flags & PF_LOG_SPI) && Serial)
				{
					Serial.print(F("phpoc_cmd: api wait "));
					Serial.print(api_wait_ms);
					Serial.println();
				}
				*/
				goto _again;
			}
			else
				return -PE_PROTOCOL;
		}
		else
			return resp_len;
	}
	else
		return 0;
}

int PhpocClass::api_write_data(const uint8_t *wbuf, int wlen, boolean pgm)
{
	uint8_t head[4];

	if(spi_resync() & SPI_FLAG_SYNC)
	{
		head[0] = API_FLAG_DATA | (wlen >> 8);  /* data bit set, csum bit clear, len 8..11 */
		head[1] = (uint8_t)wlen;
		*(uint16_t *)(head + 2) = 0x0000; /* csum/crc */
		spi_cmd_write(head, 4, false);

		return spi_cmd_write(wbuf, wlen, pgm);
	}
	else
		return 0;
}

IPAddress PhpocClass::inet_aton(const char *str)
{
	const char *digit_ptr;
	int i, len, digit;
	IPAddress ipaddr;

	len = 0;
	ipaddr = INADDR_NONE;

	for(i = 0; i < 4; i++)
	{
		if(!*str)
			return INADDR_NONE;

		digit_ptr = str;

		while(isdigit(*str))
		{
			str++;
			len++;
		}

		if((str - digit_ptr) > 3)
			return INADDR_NONE;

		if(i < 3)
		{
			if(*str != '.')
				return INADDR_NONE;
			str++;
			len++;
		}

		digit = atoi_u16(digit_ptr);
		if(digit > 255)
			return INADDR_NONE;

		ipaddr[i] |= digit;
	}

	return ipaddr;
}

int PhpocClass::command(const __FlashStringHelper *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

	if(!(flags & PF_SHIELD))
		return -PE_NO_SHIELD;

	va_start(args, format);
	cmd_len = PhpocClass::vsprintf(vsp_buf, format, args);
	va_end(args);

	return api_write_command(vsp_buf, cmd_len);
}

int PhpocClass::command(const char *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

	if(!(flags & PF_SHIELD))
		return -PE_NO_SHIELD;

	va_start(args, format);
	cmd_len = PhpocClass::vsprintf(vsp_buf, format, args);
	va_end(args);

	return api_write_command(vsp_buf, cmd_len);
}

int PhpocClass::write(const __FlashStringHelper *wstr)
{
	if(!(flags & PF_SHIELD))
		return 0;

	return api_write_data((const uint8_t *)wstr, strlen_P((const char *)wstr), true);
}

int PhpocClass::write(const char *wstr)
{
	if(!(flags & PF_SHIELD))
		return 0;

	return api_write_data((const uint8_t *)wstr, strlen(wstr), false);
}

int PhpocClass::write(const uint8_t *wbuf, size_t wlen)
{
	if(!(flags & PF_SHIELD))
		return 0;

	return api_write_data((const uint8_t *)wbuf, wlen, false);
}

int PhpocClass::read(uint8_t *rbuf, size_t rlen)
{
	int spi_txlen;

	if(!(flags & PF_SHIELD))
		return 0;

	if(spi_resync() & SPI_FLAG_SYNC)
	{
		if((spi_txlen = spi_cmd_txlen()))
		{
			if(rlen >= spi_txlen)
				rlen = spi_txlen;
			return spi_cmd_read(rbuf, rlen);
		}
	}

	return 0;
}

uint16_t PhpocClass::readInt()
{
	char int_str[6]; /* 65535 + NULL(0x00) */
	uint16_t u16;
	char *nptr;
	int rlen;

	if(!(flags & PF_SHIELD))
		return 0;

	if((rlen = read((uint8_t *)int_str, 5)))
	{
		int_str[rlen] = 0x00;
		return atoi_u16(int_str);
	}
	else
		return 0;
}

IPAddress PhpocClass::readIP()
{
	char addr_str[16]; /* x.x.x.x (min 7), xxx.xxx.xxx.xxx (max 15) */
	int rlen;

	if(!(flags & PF_SHIELD))
		return INADDR_NONE;

	if((rlen = read((uint8_t *)addr_str, 15)))
	{
		addr_str[rlen] = 0x00;
		return inet_aton(addr_str);
	}
	else
		return INADDR_NONE;
}

IP6Address PhpocClass::readIP6()
{
	char addr_str[40]; /* ::0 (min 3), xxxx:..:xxxx (max 39) */
	int rlen;

	if(!(flags & PF_SHIELD))
		return IN6ADDR_NONE;

	if((rlen = read((uint8_t *)addr_str, 40)))
	{
		addr_str[rlen] = 0x00;
		return IP6Address(addr_str);
	}
	else
		return IN6ADDR_NONE;
}

int PhpocClass::getHostByName(const char *hostname, IPAddress &ipaddr, int wait_ms)
{
	int len;

	if(!(flags & PF_SHIELD))
	{
		ipaddr = INADDR_NONE;
		return 4;
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(F("log> phpoc_dns: query A "));
		Serial.print(hostname);
		Serial.print(F(" >> "));
	}
#endif

	len = Phpoc.command(F("dns query A %s %u"), hostname, wait_ms);

	if(len > 0)
		ipaddr = Phpoc.readIP();
	else
		ipaddr = INADDR_NONE;

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(ipaddr);
		Serial.println();
	}
#endif

	return 4;
}

int PhpocClass::getHostByName6(const char *hostname, IP6Address &ip6addr, int wait_ms)
{
	int len;

	if(!(flags & PF_SHIELD))
	{
		ip6addr = IN6ADDR_NONE;
		return 16;
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(F("log> phpoc_dns: query AAAA "));
		Serial.print(hostname);
		Serial.print(F(" >> "));
	}
#endif

	len = Phpoc.command(F("dns query AAAA %s %u"), hostname, wait_ms);

	if(len > 0)
		ip6addr = Phpoc.readIP6();
	else
		ip6addr = IN6ADDR_NONE;

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(ip6addr);
		Serial.println();
	}
#endif

	return 16;
}

void PhpocClass::logFlush(uint8_t id)
{
	Phpoc.command(F("sys log%u flush"), id);
}

#define LOG_BUF_SIZE 32

void PhpocClass::logPrint(uint8_t id)
{
	char log[LOG_BUF_SIZE + 2];
	int len;

	if(!Serial)
		return;

	while((len = Phpoc.command(F("sys log%u read %u"), id, LOG_BUF_SIZE)) > 0)
	{
		Phpoc.read((uint8_t *)log, len);
		log[len] = 0x00;
		Serial.write(log, len);
	}
}

#define MAX_BEGIN_WAIT 10

int PhpocClass::beginIP4()
{
	int wait_count;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: IPv4 "));
#endif

	wait_count = 0;

	while(!localIP())
	{
		if(wait_count >= MAX_BEGIN_WAIT)
		{
#ifdef PF_LOG_NET
			if((flags & PF_LOG_NET) && Serial)
				Serial.println();
#endif
			return 0;
		}

#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
		{
			if(!wait_count)
				Serial.print(F("acquiring IP address ."));
			else
				Serial.print('.');
		}
#endif

		delay(1000);
		wait_count++;
	}

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		Serial.print(localIP());
		Serial.print(' ');
		Serial.print(subnetMask());
		Serial.print(' ');
		Serial.print(gatewayIP());
		Serial.print(' ');
		Serial.print(dnsServerIP());
		Serial.println();
	}
#endif

	return 1;
}

int PhpocClass::beginIP6()
{
	IP6Address ip6addr;
	int wait_count = 0;

	/* don't call localIP6() before PF_IP6 flag is set */
	if(command(F("net0 get ipaddr6 0")) >= 0)
		ip6addr = readIP6();

	if(ip6addr == IN6ADDR_NONE)
	{
#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
		{
			Serial.print(F("log> phpoc_begin: IPv6 not enabled"));
			Serial.println();
		}
#endif
		return 0;
	}

	flags |= PF_IP6;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: IPv6 "));
#endif

	wait_count = 0;

	while(globalIP6() == IN6ADDR_NONE)
	{
		if(wait_count >= MAX_BEGIN_WAIT)
		{
#ifdef PF_LOG_NET
			if((flags & PF_LOG_NET) && Serial)
				Serial.println();
#endif
			return 0;
		}

#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
		{
			if(!wait_count)
				Serial.print(F("acquiring global IP address ."));
			else
				Serial.print('.');
		}
#endif

		delay(1000);
		wait_count++;
	}

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		Serial.print(localIP6());
		Serial.print(' ');
		Serial.print(globalIP6());
		Serial.print('/');
		Serial.print(globalPrefix6());
		Serial.println();

		Serial.print(F("log> phpoc_begin: IPv6 "));
		Serial.print(gatewayIP6());
		Serial.print(' ');
		Serial.print(dnsServerIP6());
		Serial.println();
	}
#endif

	return 1;
}

#define MSG_BUF_SIZE 16

int PhpocClass::begin(uint8_t init_flags)
{
	uint8_t net_id, wait_count;
	uint8_t msg[MSG_BUF_SIZE];
	int len, sock_id;

	flags |= init_flags;
	spi_wait_ms = SPI_WAIT_MS;

	pinMode(SPI_NSS_PIN, OUTPUT);
	SPI.begin();

	if(!(spi_resync() & SPI_FLAG_SYNC))
		return 0;

	/* we should set PF_SHIELD flag after spi_resync success */
	flags |= PF_SHIELD;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: "));
#endif
		
	if(Phpoc.command(F("net1 get mode")) < 0)
		return 0;
	len = Phpoc.read(msg, MSG_BUF_SIZE);

	if(len > 0)
	{ /* WiFi dongle installed */
		net_id = 1;

#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
			Serial.print(F("WiFi "));
#endif
	}
	else
	{ /* WiFi dongle not installed */
		net_id = 0;

		if(Phpoc.command(F("net0 get mode")) < 0)
			return 0;
		len = Phpoc.read(msg, MSG_BUF_SIZE);

		if(len)
		{ /* ethernet present */
#ifdef PF_LOG_NET
			if((flags & PF_LOG_NET) && Serial)
				Serial.print(F("Ethernet "));
#endif
		}
		else
		{ /* ethernet absent */
#ifdef PF_LOG_NET
			if((flags & PF_LOG_NET) && Serial)
				Serial.println(F("WiFi dongle not installed"));
#endif
			return 0;
		}
	}
		
#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		Serial.write(msg, len);
		Serial.print(' ');
	}
#endif

	wait_count = 0;

	while(1)
	{
		if(Phpoc.command(F("net%u get speed"), net_id) < 0)
			return 0;

		if(readInt() > 0) /* Ethernet plugged, or WiFi associated ? */
			break;

		if(wait_count >= MAX_BEGIN_WAIT)
			break;

#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
		{
			if(!wait_count)
			{
				if(net_id)
					Serial.print(F("scanning ."));
				else
					Serial.print(F("unplugged ."));
			}
			else
				Serial.print('.');
		}
#endif

		delay(1000);
		wait_count++;
	}

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
		Serial.println();
#endif

	for(sock_id = 0; sock_id < MAX_SOCK_TCP; sock_id++)
		Phpoc.command(F("tcp%u close"), sock_id);

	return beginIP4();
}

int PhpocClass::maintain()
{
	return 0; /* DHCP_CHECK_NONE */
}

IPAddress PhpocClass::localIP()
{
	if(!(flags & PF_SHIELD))
		return INADDR_NONE;

	if(command(F("net0 get ipaddr")) < 0)
		return INADDR_NONE;

	return readIP();
}

IPAddress PhpocClass::subnetMask()
{
	if(!(flags & PF_SHIELD))
		return INADDR_NONE;

	if(command(F("net0 get netmask")) < 0)
		return INADDR_NONE;

	return readIP();
}

IPAddress PhpocClass::gatewayIP()
{
	if(!(flags & PF_SHIELD))
		return INADDR_NONE;

	if(command(F("net0 get gwaddr")) < 0)
		return INADDR_NONE;

	return readIP();
}

IPAddress PhpocClass::dnsServerIP()
{
	if(!(flags & PF_SHIELD))
		return INADDR_NONE;

	if(command(F("net0 get nsaddr")) < 0)
		return INADDR_NONE;

	return readIP();
}

IP6Address PhpocClass::localIP6()
{
	if(!(flags & PF_SHIELD) || !(flags & PF_IP6))
		return IN6ADDR_NONE;

	if(command(F("net0 get ipaddr6 0")) < 0)
		return IN6ADDR_NONE;

	return readIP6();
}

IP6Address PhpocClass::globalIP6()
{
	if(!(flags & PF_SHIELD) || !(flags & PF_IP6))
		return IN6ADDR_NONE;

	if(command(F("net0 get ipaddr6 1")) < 0)
		return IN6ADDR_NONE;

	return readIP6();
}

IP6Address PhpocClass::gatewayIP6()
{
	if(!(flags & PF_SHIELD) || !(flags & PF_IP6))
		return IN6ADDR_NONE;

	if(command(F("net0 get gwaddr6")) < 0)
		return IN6ADDR_NONE;

	return readIP6();
}

IP6Address PhpocClass::dnsServerIP6()
{
	if(!(flags & PF_SHIELD) || !(flags & PF_IP6))
		return IN6ADDR_NONE;

	if(command(F("net0 get nsaddr6")) < 0)
		return IN6ADDR_NONE;

	return readIP6();
}

int PhpocClass::globalPrefix6()
{
	if(!(flags & PF_SHIELD) || !(flags & PF_IP6))
		return 0;

	if(command(F("net0 get prefix6")) < 0)
		return 0;

	return readInt();
}

PhpocClass Phpoc;
