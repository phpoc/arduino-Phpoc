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
 *  | D 0 0 0|len8..11|    len 0..7    |             csum/crc            | cmd or data...
 *  +--------+--------+----------------+----------------+----------------+---------------
 *  - D : 0 - command, 1 - data
 *  - len0..11 : command/data length excluding header
 *
 * API response
 *   0                 1                2                3                4
 *  +--------+--------+----------------+----------------+----------------+--------
 *  | 1 0 N 0|len8..11|    len 0..7    |             csum/crc            | data...
 *  +--------+--------+----------------+----------------+----------------+--------
 *  - N : command NAK
 *  - len0..11 : response data length excluding header
 *
 * command NAK response data
 *  - command error
 *    . response data : "Ennn" + human readable string
 *    . nnn : error code ("000" ~ "999")
 *  - command wait
 *    . response data : "Wnnn"
 *    . nnn : wait time in second ("000" ~ "999")
 */

#include <SPI.h>
#include <Phpoc.h>

/* last measured V1 command time : 4~7ms */
//#define MEASURE_COMMAND_TIME

#define SYNC_MAGIC_V1 0x5a3c

#define S2M_FLAG_SYNC 0x8000 /* SPI SYNC ok */
#define S2M_FLAG_RXB  0x2000 /* data is in rx buffer */
#define S2M_FLAG_TXB  0x1000 /* data is in tx buffer */
#define S2M_FLAG_SBZ  0x4fff

#define API_FLAG_DATA  0x80 /* 0 - command, 1 - data */
#define API_FLAG_NAK   0x20 /* 0 - command ok, 1 - command error/wait */

#define SPI_NSS_PIN 10

/* - crypto computation time
 *   2048bit RSA public key op  : 16ms
 *   1024bit RSA private key op : 75ms
 *   2048bit RSA private key op : 404ms
 *   DH new key : 270ms
 *   DH new key + 2048bit RSA private key op : 700ms ?
 * - SPI_WAIT_MS time should be longer than crypto computation time.
 * - SSL : if debug_ssl is ON & fdc0_txbuf_size is 0, SPI_WAIT_MS should be set 700ms
 * - SSH : recommended SPI_WAIT_MS is 900ms
 */
#define SPI_WAIT_MS 1000 /* 1 second */

#ifdef INCLUDE_LIB_V1

uint16_t PhpocClass::spi_cmd_status(void)
{
	return Sppc.spi_request(0x0000);
}

uint16_t PhpocClass::spi_cmd_sync(void)
{
	return Sppc.spi_request(SYNC_MAGIC_V1);
}

uint16_t PhpocClass::spi_resync(void)
{
	uint16_t status;

	status = spi_cmd_status();

	if(!(status & S2M_FLAG_SYNC) || (status & S2M_FLAG_SBZ))
	{
#ifdef PF_LOG_SPI
		if((Sppc.flags & PF_LOG_SPI) && Serial)
			Serial.print(F("log> phpoc_spi: resync spi channel v1 ... "));
#endif

		Sppc.spi_request(0xf000); /* issue BAD_CMD reset */
		delay(105);          /* wait BAD_CMD or TIMEOUT reset */

		spi_cmd_sync();
		status = spi_cmd_status();

		if(!(status & S2M_FLAG_SYNC) || (status & S2M_FLAG_SBZ))
		{
#ifdef PF_LOG_SPI
			if((Sppc.flags & PF_LOG_SPI) && Serial)
				Serial.println(F("failed"));
#endif
			return 0x0000;
		}
		else
		{
#ifdef PF_LOG_SPI
			if((Sppc.flags & PF_LOG_SPI) && Serial)
				Serial.println(F("success"));
#endif
		}
	}

	return status;
}

int PhpocClass::spi_cmd_txlen(void)
{
	return Sppc.spi_request(0x1000) & 0x0fff;
}

int PhpocClass::spi_cmd_rxfree(void)
{
	return Sppc.spi_request(0x2000) & 0x0fff;
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

uint16_t PhpocClass::php_request(const char *wbuf, int wlen)
{
	int resp_len, api_wait_ms, err;
	uint8_t head[4], msg[8];
	uint16_t status;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return 0;
	}

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		Sppc.errno = EPERM;
		return 0;
	}

	/* PhpocEmail command arguments
	 * - v1 : smtp server/login/from/to/subject/data/send/status
	 * - v2 : php smtp server/login/from/to/subject/data/send/status
	 *
	 * PhpocServer/Client command arguments
	 * - v1 : tcp0/1/2/3/4/5 get/set/send/recv/peek/listen/connect/close
	 * - v2 : tcp0/1/2/3/4/5 ioctl get/set/close
	 *        tcp0/1/2/3/4/5 send/recv/peek/bind/listen/connect
	 *
	 * Phpoc net command arguments
	 * - v1 : net0/1 get (PhpocClass::begin)
	 *        net1 get
	 * - v2 : net0/1 ioctl get (SppcClass::begin)
	 *        net ioctl get
	 */
	if(!strncmp(wbuf, "php ", 4))
	{
		wbuf += 4;
		wlen -= 4;
	}
	else
	{
		char *ptr;

		ptr = wbuf;
		while(*ptr != ' ')
			ptr++;

		if(!strncmp(ptr, " ioctl ", 7))
		{
			strcpy(ptr, ptr + 6);
			wlen -= 6;
		}
	}

	/* cleanup slave tx buffer */
	if(status & S2M_FLAG_TXB)
		Sppc.spi_cmd_read(0x3000, NULL, spi_cmd_txlen());

	head[0] = 0x00 | (wlen >> 8);  /* data & csum bit clear, len 8..11 */
	head[1] = (uint8_t)wlen;
	*(uint16_t *)(head + 2) = 0x0000; /* csum/crc */
	Sppc.spi_cmd_write(0x4000, head, 4, false);

	Sppc.spi_cmd_write(0x4000, (const uint8_t *)wbuf, wlen, false);

	api_wait_ms = SPI_WAIT_MS;

_again:
	if(!spi_wait(4, api_wait_ms))
	{
#ifdef PF_LOG_SPI
		if((Sppc.flags & PF_LOG_SPI) && Serial)
			Serial.println(F("log> phpoc_cmd: head wait timeout"));
#endif
		Sppc.errno = ETIME;
		return 0;
	}

	Sppc.spi_cmd_read(0x3000, head, 4);

	resp_len  = head[0] << 8;
	resp_len |= head[1];
	resp_len &= 0x0fff;

	if(resp_len && !spi_wait(resp_len, SPI_WAIT_MS))
	{
#ifdef PF_LOG_SPI
		if((Sppc.flags & PF_LOG_SPI) && Serial)
			Serial.println(F("log> phpoc_cmd: data wait timeout"));
#endif
		Sppc.errno = ETIME;
		return 0;
	}

	if(head[0] & API_FLAG_NAK)
	{
		Sppc.spi_cmd_read(0x3000, msg, 4); /* Wnnn */
		msg[4] = 0x00;

		if(msg[0] == 'E')
		{
			err = atoi((const char *)msg + 1);

#ifdef PF_LOG_SPI
			if((Sppc.flags & PF_LOG_SPI) && Serial)
			{
				resp_len -= 4;

				sppc_printf(F("log> phpoc_cmd: error %d, "), err);

				while(resp_len > 0)
				{
					Sppc.spi_cmd_read(0x3000, msg, 1);
					Serial.write(*msg);
					resp_len--;
				}

				Serial.println();
			}
#endif
			Sppc.errno = err;
			return 0;
		}
		else
		if(msg[0] == 'W')
		{
			api_wait_ms = atoi((const char *)msg + 1) * 1000;

			/*
			if((Sppc.flags & PF_LOG_SPI) && Serial)
				sppc_printf(F("log> phpoc_cmd: api wait %d\r\n"), api_wait_ms);
			*/
			goto _again;
		}
		else
		{
			Sppc.errno = EPROTO;
			return 0;
		}
	}
	else
		return resp_len;
}

int PhpocClass::php_write_data(const uint8_t *wbuf, int wlen, boolean pgm)
{
	uint8_t head[4];
	uint16_t status;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return 0;
	}

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		Sppc.errno = EPERM;
		return 0;
	}

	head[0] = API_FLAG_DATA | (wlen >> 8);  /* data bit set, csum bit clear, len 8..11 */
	head[1] = (uint8_t)wlen;
	*(uint16_t *)(head + 2) = 0x0000; /* csum/crc */
	Sppc.spi_cmd_write(0x4000, head, 4, false);

	return Sppc.spi_cmd_write(0x4000, wbuf, wlen, pgm);
}

#endif /* INCLUDE_LIB_V1 */

uint16_t PhpocClass::command(const __FlashStringHelper *format, ...)
{
#ifdef MEASURE_COMMAND_TIME
	uint16_t t1_ms;
	int retval;
#endif
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

#ifdef MEASURE_COMMAND_TIME
	t1_ms = (uint16_t)millis();
#endif

	va_start(args, format);
	cmd_len = sppc_vsprintf(vsp_buf, format, args);
	va_end(args);

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
#ifdef MEASURE_COMMAND_TIME
	{
		retval = php_request(vsp_buf, cmd_len);
		sppc_printf(F("<%d>"), (uint16_t)millis() - t1_ms);
		return retval;
	}
#else
		return php_request(vsp_buf, cmd_len);
#endif
	else
#endif
		return Sppc.sppc_request(vsp_buf, cmd_len);
}

uint16_t PhpocClass::command(const char *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

	va_start(args, format);
	cmd_len = sppc_vsprintf(vsp_buf, format, args);
	va_end(args);

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		return php_request(vsp_buf, cmd_len);
	else
#endif
		return Sppc.sppc_request(vsp_buf, cmd_len);
}

int PhpocClass::write(const __FlashStringHelper *wstr)
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		return php_write_data((const uint8_t *)wstr, strlen_P((const char *)wstr), true);
	else
#endif
		return Sppc.write(wstr);
}

int PhpocClass::write(const char *wstr)
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		return php_write_data((const uint8_t *)wstr, strlen(wstr), false);
	else
#endif
		return Sppc.write(wstr);
}

int PhpocClass::write(const uint8_t *wbuf, size_t wlen)
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		return php_write_data((const uint8_t *)wbuf, wlen, false);
	else
#endif
		return Sppc.write(wbuf, wlen);
}

int PhpocClass::read(uint8_t *rbuf, size_t rlen)
{
	uint16_t status;
	int spi_txlen;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return 0;
	}

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		status = spi_resync();

		if(!(status & S2M_FLAG_SYNC))
		{
			Sppc.errno = EPERM;
			return 0;
		}

		if((spi_txlen = spi_cmd_txlen()))
		{
			if(rlen >= spi_txlen)
				rlen = spi_txlen;

			return Sppc.spi_cmd_read(0x3000, rbuf, rlen);
		}
	}
	else
#endif
		return Sppc.read(rbuf, rlen);
}

int PhpocClass::getHostByName(const char *hostname, IPAddress &ipaddr, int wait_ms)
{
	int len;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;

		ipaddr = INADDR_NONE;
		return 4;
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		sppc_printf(F("log> dns: query A %s >> "), hostname);
#endif

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		len = command(F("dns query A %s %u"), hostname, wait_ms);
	else
#endif
	{
		Sppc.command(F("dns set timeout %d"), wait_ms);
		Sppc.command(F("dns query %s A"), hostname);
		len = Sppc.command(F("dns get answer A"));
	}

	if(len > 0)
		ipaddr = readIP();
	else
		ipaddr = INADDR_NONE;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(ipaddr);
		Serial.println();
	}
#endif

	return 4;
}

int PhpocClass::tcpIoctlReadInt(const __FlashStringHelper *args, int sock_id)
{
	int retval;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		retval = command(F("tcp%u ioctl get %S"), sock_id, args);

		if(retval > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
		return Sppc.command(F("tcp%u ioctl get %S"), sock_id, args);
}

int PhpocClass::getHostByName6(const char *hostname, IP6Address &ip6addr, int wait_ms)
{
	int len;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;

		ip6addr = IN6ADDR_NONE;
		return 16;
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		sppc_printf(F("log> dns: query AAAA %s >> "), hostname);
#endif

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		len = command(F("dns query AAAA %s %u"), hostname, wait_ms);
	else
#endif
	{
		Sppc.command(F("dns set timeout %d"), wait_ms);
		Sppc.command(F("dns query %s AAAA"), hostname);
		len = Sppc.command(F("dns get answer AAAA"));
	}

	if(len > 0)
		ip6addr = readIP6();
	else
		ip6addr = IN6ADDR_NONE;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(ip6addr);
		Serial.println();
	}
#endif

	return 16;
}

uint16_t PhpocClass::readInt()
{
	char int_str[6]; /* 65535 + NULL(0x00) */
	uint16_t u16;
	char *nptr;
	int rlen;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return 0;
	}

	if((rlen = read((uint8_t *)int_str, 5)))
	{
		int_str[rlen] = 0x00;
		return atoi(int_str);
	}
	else
		return 0;
}

IPAddress PhpocClass::readIP()
{
	char addr_str[16]; /* x.x.x.x (min 7), xxx.xxx.xxx.xxx (max 15) */
	IPAddress ipaddr;
	int rlen;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return INADDR_NONE;
	}

	if((rlen = read((uint8_t *)addr_str, 15)))
	{
		addr_str[rlen] = 0x00;
		ipaddr.fromString(addr_str);
		return ipaddr;
	}
	else
		return INADDR_NONE;
}

IP6Address PhpocClass::readIP6()
{
	char addr_str[40]; /* ::0 (min 3), xxxx:..:xxxx (max 39) */
	int rlen;

	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return IN6ADDR_NONE;
	}

	if((rlen = read((uint8_t *)addr_str, 40)))
	{
		addr_str[rlen] = 0x00;
		return IP6Address(addr_str);
	}
	else
		return IN6ADDR_NONE;
}

void PhpocClass::logFlush(uint8_t id)
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		command(F("sys log%u flush"), id);
	else
#endif
		Sppc.logFlush(id);
}

#define LOG_BUF_SIZE 32

void PhpocClass::logPrint(uint8_t id)
{
	char log[LOG_BUF_SIZE];
	int len;

	if(!Serial)
		return;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		while((len = command(F("sys log%u read %u"), id, LOG_BUF_SIZE)) > 0)
		{
			read((uint8_t *)log, len);
			Serial.write(log, len);
		}
	}
	else
#endif
		Sppc.logPrint(id);
}


#define MAX_NET_WAIT 10000

#ifdef INCLUDE_LIB_V1
int PhpocClass::beginIP4()
{
	int wait_count;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: IPv4 "));
#endif

	wait_count = 0;

	while(!localIP())
	{
		if(wait_count >= MAX_NET_WAIT)
		{
#ifdef PF_LOG_NET
			if((Sppc.flags & PF_LOG_NET) && Serial)
				Serial.println();
#endif
			return 0;
		}

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
		{
			if(!wait_count)
				Serial.print(F("acquiring IP address ."));
			else
				Serial.print('.');
		}
#endif

		delay(1000);
		wait_count += 1000;
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
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
#endif /* INCLUDE_LIB_V1 */

int PhpocClass::beginIP6()
{
#ifdef INCLUDE_LIB_V1
	IP6Address ip6addr;
	int wait_count = 0;

	if(Sppc.flags & PF_SYNC_V2)
		return Sppc.beginIP6();

	/* don't call localIP6() before PF_IP6 flag is set */
	command(F("net1 ioctl get ipaddr6 0"));
	ip6addr = readIP6();

	if(ip6addr == IN6ADDR_NONE)
	{
#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> phpoc_begin: IPv6 not enabled"));
#endif
		return 0;
	}

	Sppc.flags |= PF_IP6;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: IPv6 "));
#endif

	wait_count = 0;

	while(globalIP6() == IN6ADDR_NONE)
	{
		if(wait_count >= MAX_NET_WAIT)
		{
#ifdef PF_LOG_NET
			if((Sppc.flags & PF_LOG_NET) && Serial)
				Serial.println();
#endif
			return 0;
		}

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
		{
			if(!wait_count)
				Serial.print(F("acquiring global IP address ."));
			else
				Serial.print('.');
		}
#endif

		delay(1000);
		wait_count += 1000;
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
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

	return 1;
#endif
#else /* INCLUDE_LIB_V1 */
	return Sppc.beginIP6();
#endif
}

#define MAX_BOOT_WAIT 500
#define MSG_BUF_SIZE 16

int PhpocClass::begin(uint16_t init_flags)
{
#ifdef INCLUDE_LIB_V1
	uint8_t net_id, wait_count;
	uint8_t msg[MSG_BUF_SIZE];
	int len, sock_id;
#endif
	uint16_t status;

	Sppc.flags |= init_flags;

	pinMode(SPI_NSS_PIN, OUTPUT);
	digitalWrite(SPI_NSS_PIN, HIGH);

	SPI.begin();

	Sppc.flags |= PF_INIT_SPI;

#ifdef INCLUDE_LIB_V1
	wait_count = 0;

_retry_sync:
	status = Sppc.spi_resync();

	if(status & S2M_FLAG_SYNC)
#endif
		return Sppc.begin(init_flags);

#ifdef INCLUDE_LIB_V1

	Sppc.errno = 0;

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		delay(100);
		wait_count += 100;

		if(wait_count >= MAX_BOOT_WAIT)
		{
			Sppc.errno = EPERM;
			return 0;
		}
		else
			goto _retry_sync;
	}

	Sppc.flags |= PF_SYNC_V1;

	/* we should set PF_SHIELD flag after spi_resync success */
	Sppc.flags |= PF_SHIELD;

	if(command(F("sys pkg ver")) > 0)
	{
		char *ver;

		len = read(msg, MSG_BUF_SIZE);
		msg[len] = 0x00;

		ver = (char *)msg;
		Sppc.pkg_ver_id = 0;

		for(int i = 0; i < 3; i++)
		{
			if(i == 0)
				Sppc.pkg_ver_id += atoi(ver) * 10000;
			else
			if(i == 1)
				Sppc.pkg_ver_id += atoi(ver) * 100;
			else
				Sppc.pkg_ver_id += atoi(ver);

			while(*ver && (*ver != '.'))
				ver++;
			if(ver)
				ver++;
		}

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			sppc_printf(F("log> phpoc_begin: package %s\r\n"), msg);
#endif
	}
	else
		Sppc.pkg_ver_id = 10000;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_begin: "));
#endif
		
	command(F("net1 ioctl get mode"));
	len = read(msg, MSG_BUF_SIZE);

	if(len > 0)
	{ /* WiFi dongle installed */
		net_id = 1;

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
		{
			msg[len] = 0x00;
			sppc_printf(F("WiFi %s "), msg);

			if(Sppc.pkg_ver_id > 10000)
			{
				command(F("net1 ioctl get ssid"));
				len = read(msg, MSG_BUF_SIZE);
				msg[len] = 0x00;
				sppc_printf(F("%s "), msg);

				command(F("net1 ioctl get ch"));
				len = read(msg, MSG_BUF_SIZE);
				msg[len] = 0x00;
				sppc_printf(F("ch%s "), msg);
			}
		}
#endif
	}
	else
	{ /* WiFi dongle not installed */
		net_id = 0;

		command(F("net0 ioctl get mode"));
		len = read(msg, MSG_BUF_SIZE);

		if(len)
		{ /* ethernet present */
#ifdef PF_LOG_NET
			if((Sppc.flags & PF_LOG_NET) && Serial)
			{
				msg[len] = 0x00;
				sppc_printf(F("Ethernet %s "), msg);
			}
#endif
		}
		else
		{ /* ethernet absent */
#ifdef PF_LOG_NET
			if((Sppc.flags & PF_LOG_NET) && Serial)
				Serial.println(F("WiFi dongle not installed"));
#endif
			return 0;
		}
	}
		
	wait_count = 0;

	while(1)
	{
		int speed;

		command(F("net%u ioctl get speed"), net_id);
		speed = readInt();

		if(speed > 0) /* Ethernet plugged, or WiFi associated ? */
			break;

		if(wait_count >= MAX_NET_WAIT)
			break;

#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
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
		wait_count += 1000;
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		Serial.println();
#endif

	/* init app - smtp */
	command(F("php smtp server")); /* clear MSA server */

	return beginIP4();

#endif /* INCLUDE_LIB_V1 */
}

int PhpocClass::maintain()
{
	return 0; /* DHCP_CHECK_NONE */
}

IPAddress PhpocClass::localIP()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return INADDR_NONE;
	}

	command(F("net1 ioctl get ipaddr"));
	return readIP();
}

IPAddress PhpocClass::subnetMask()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return INADDR_NONE;
	}

	command(F("net1 ioctl get netmask"));
	return readIP();
}

IPAddress PhpocClass::gatewayIP()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return INADDR_NONE;
	}

	command(F("net1 ioctl get gwaddr"));
	return readIP();
}

IPAddress PhpocClass::dnsServerIP()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD))
	{
		Sppc.errno = EPERM;
		return INADDR_NONE;
	}

	command(F("net1 ioctl get nsaddr"));
	return readIP();
}

IP6Address PhpocClass::localIP6()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD) || !(Sppc.flags & PF_IP6))
	{
		Sppc.errno = EPERM;
		return IN6ADDR_NONE;
	}

	command(F("net1 ioctl get ipaddr6 0"));
	return readIP6();
}

IP6Address PhpocClass::globalIP6()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD) || !(Sppc.flags & PF_IP6))
	{
		Sppc.errno = EPERM;
		return IN6ADDR_NONE;
	}

	command(F("net1 ioctl get ipaddr6 1"));
	return readIP6();
}

IP6Address PhpocClass::gatewayIP6()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD) || !(Sppc.flags & PF_IP6))
	{
		Sppc.errno = EPERM;
		return IN6ADDR_NONE;
	}

	command(F("net1 ioctl get gwaddr6"));
	return readIP6();
}

IP6Address PhpocClass::dnsServerIP6()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD) || !(Sppc.flags & PF_IP6))
	{
		Sppc.errno = EPERM;
		return IN6ADDR_NONE;
	}

	command(F("net1 ioctl get nsaddr6"));
	return readIP6();
}

int PhpocClass::globalPrefix6()
{
	Sppc.errno = 0;

	if(!(Sppc.flags & PF_SHIELD) || !(Sppc.flags & PF_IP6))
	{
		Sppc.errno = EPERM;
		return 0;
	}

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		command(F("net1 ioctl get prefix6"));
		return readInt();
	}
	else
#endif
		return command(F("net1 ioctl get prefix6"));
}

PhpocClass Phpoc;
