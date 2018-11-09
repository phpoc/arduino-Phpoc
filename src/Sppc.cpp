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

/* master(arduino) to slave(phpoc) request
 * - bid 0 : command & arguments / ascii string + IDLE_TIME
 * - bid 1 : command data / octet string
 *
 * slave(phpoc) to master(arduino) response
 * - bid 0 : command response / 64bit integer (little endian)
 * - bid 1 : response data / octet string
 */

#include <SPI.h>
#include <Sppc.h>

#define SYNC_MAGIC_V2 0xa5c3
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

#define EAGAIN_WAIT_MS 10

char SppcClass::readstr_buf[READSTR_BUF_SIZE];
uint16_t SppcClass::flags;
uint16_t SppcClass::pkg_ver_id;
uint16_t SppcClass::errno;

/* SPI_MODE3 : clock idle high, capture data on the first clock edge */
static const SPISettings SPI_PHPOC_SETTINGS(1000000, MSBFIRST, SPI_MODE3);

uint16_t SppcClass::spi_request(uint16_t req)
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

uint16_t SppcClass::spi_cmd_status(void)
{
	return spi_request(CMD_STATUS);
}

uint16_t SppcClass::spi_cmd_sync(void)
{
	return spi_request(SYNC_MAGIC_V2);
}

#define IS_V2_R_SYNC(_status) (((_status) & 0x87ff) == 0x823c)

uint16_t SppcClass::spi_resync(void)
{
	uint16_t status;

	status = spi_cmd_sync();

	if(!IS_V2_R_SYNC(status))
	{
#ifdef PF_LOG_SPI
		if((flags & PF_LOG_SPI) && Serial)
			Serial.print(F("log> sppc_spi: resync spi channel v2 ... "));
#endif

		spi_request(0xe000); /* issue BAD_CMD reset */
		delay(105);          /* wait BAD_CMD or TIMEOUT reset */

		status = spi_cmd_sync();

		if(!IS_V2_R_SYNC(status))
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

int SppcClass::spi_cmd_txlen(int bid)
{
	if(bid)
		return spi_request(CMD_TXLEN_B1) & 0x07ff;
	else
		return spi_request(CMD_TXLEN_B0) & 0x07ff;
}

int SppcClass::spi_cmd_rxfree(int bid)
{
	if(bid)
		return spi_request(CMD_RXFREE_B1) & 0x07ff;
	else
		return spi_request(CMD_RXFREE_B0) & 0x07ff;
}

int SppcClass::spi_wait(int bid, int len, int ms)
{
	uint16_t t1_ms16, t2_ms16, elapsed_ms;
	int us_count;

	if(!ms)
		ms = SPI_WAIT_MS;

	t1_ms16 = (uint16_t)millis();

	us_count = 32; /* 4 bytes transfer time */

	while(spi_cmd_txlen(bid) < len)
	{
		t2_ms16 = (uint16_t)millis();

		if(t2_ms16 > t1_ms16)
			elapsed_ms = t2_ms16 - t1_ms16;
		else
			elapsed_ms = (~t1_ms16 + 1) + t2_ms16;

		if(elapsed_ms > ms)
			return 0;

		delayMicroseconds(us_count);

		if(us_count < 16384)
			us_count <<= 1; /* us_count : 16 32 64 128 256 512 1024 2048 4096 8192 16384 */
	}

	return len;
}

int SppcClass::spi_cmd_read(uint16_t cmd, uint8_t *rbuf, size_t rlen)
{
	uint16_t resp;
	int count;

	cmd |= rlen;

	SPI.beginTransaction(SPI_PHPOC_SETTINGS);
	digitalWrite(SPI_NSS_PIN, LOW);

	SPI.transfer(cmd >> 8);
	SPI.transfer(cmd & 0xff);
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

int SppcClass::spi_cmd_write(uint16_t cmd, const uint8_t *wbuf, size_t wlen, boolean pgm)
{
	uint16_t resp;
	int count;

	cmd |= wlen;

	SPI.beginTransaction(SPI_PHPOC_SETTINGS);
	digitalWrite(SPI_NSS_PIN, LOW);

	SPI.transfer(cmd >> 8);
	SPI.transfer(cmd & 0xff);
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

uint16_t SppcClass::sppc_request(const char *wbuf, int wlen)
{
	uint8_t retval64[8];
	uint16_t status;
	long retval;

	errno = 0;

	if(!(flags & PF_SHIELD))
	{
		errno = EPERM;
		return 0;
	}

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		errno = EPERM;
		return 0;;
	}

	/* cleanup slave command/data tx buffer */
	spi_cmd_read(CMD_READ_B0, NULL, spi_cmd_txlen(BID_CMD));
	spi_cmd_read(CMD_READ_B1, NULL, spi_cmd_txlen(BID_DATA));

_again:
	spi_cmd_write(CMD_WRITE_B0, (const uint8_t *)wbuf, wlen, false);

	if(spi_wait(BID_CMD, 8, SPI_WAIT_MS) < 8)
	{
#ifdef PF_LOG_SPI
		if((flags & PF_LOG_SPI) && Serial)
			Serial.println(F("log> command: head wait timeout"));
#endif
		errno = ETIME;
		return 0;
	}

	spi_cmd_read(CMD_READ_B0, retval64, 8);

	retval = *(long *)retval64;

	if(retval < 0)
	{
		if((int)retval == -EAGAIN)
		{
			delay(EAGAIN_WAIT_MS);
			goto _again;
		}

#ifdef PF_LOG_SPI
		if((flags & PF_LOG_SPI) && Serial)
			sppc_printf(F("log> command: error %d\r\n"), (int)retval);
#endif

		errno = -(int)retval;
		return 0;
	}

	return (uint16_t)retval;
}

int SppcClass::sppc_write_data(const uint8_t *wbuf, int wlen, boolean pgm)
{
	uint8_t head[4];
	uint16_t status;

	errno = 0;

	if(!(flags & PF_SHIELD))
	{
		errno = EPERM;
		return 0;
	}

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		errno = EPERM;
		return 0;
	}

	return spi_cmd_write(CMD_WRITE_B1, wbuf, wlen, pgm);
}

uint16_t SppcClass::command(const __FlashStringHelper *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT + 2/*CRLF*/];
	va_list args;
	int cmd_len;

	va_start(args, format);
	cmd_len = sppc_vsprintf(vsp_buf, format, args);
	va_end(args);

	return sppc_request(vsp_buf, cmd_len);
}

uint16_t SppcClass::command(const char *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT + 2/*CRLF*/];
	va_list args;
	int cmd_len;

	va_start(args, format);
	cmd_len = sppc_vsprintf(vsp_buf, format, args);
	va_end(args);

	return sppc_request(vsp_buf, cmd_len);
}

int SppcClass::write(const __FlashStringHelper *wstr)
{
	return sppc_write_data((const uint8_t *)wstr, strlen_P((const char *)wstr), true);
}

int SppcClass::write(const char *wstr)
{
	return sppc_write_data((const uint8_t *)wstr, strlen(wstr), false);
}

int SppcClass::write(const uint8_t *wbuf, size_t wlen)
{
	return sppc_write_data((const uint8_t *)wbuf, wlen, false);
}

int SppcClass::read(uint8_t *rbuf, size_t rlen)
{
	uint16_t status;
	int spi_txlen;

	errno = 0;

	if(!(flags & PF_SHIELD))
	{
		errno = EPERM;
		return 0;
	}

	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		errno = EPERM;
		return 0;
	}

	if((spi_txlen = spi_cmd_txlen(BID_DATA)) > 0)
	{
		if(rlen >= spi_txlen)
			rlen = spi_txlen;

		return spi_cmd_read(CMD_READ_B1, rbuf, rlen);
	}
	else
		return 0;
}

char *SppcClass::readString(void)
{
	int len;

	len = read(readstr_buf, READSTR_BUF_SIZE - 2);
	readstr_buf[len] = 0x00;
	return readstr_buf;
}

#define LOG_BUF_SIZE 32

void SppcClass::logFlush(uint8_t id)
{
	uint8_t log[LOG_BUF_SIZE];
	int len;

	while((len = command(F("log%u read %u"), id, LOG_BUF_SIZE)) > 0)
		read(log, len);
}

void SppcClass::logPrint(uint8_t id)
{
	uint8_t log[LOG_BUF_SIZE];
	int len;

	if(!Serial)
		return;

	while((len = command(F("log%u read %u"), id, LOG_BUF_SIZE)) > 0)
	{
		read(log, len);
		Serial.write(log, len);
	}
}

int SppcClass::beginIP4()
{
	flags |= PF_IP4;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		sppc_printf(F("log> sppc_begin: IPv4 "));

		command(F("net ioctl get ipaddr"));
		sppc_printf(F("%s "), readString());

		command(F("net ioctl get netmask"));
		sppc_printf(F("%s "), readString());

		command(F("net ioctl get gwaddr"));
		sppc_printf(F("%s "), readString());

		command(F("net ioctl get nsaddr"));
		sppc_printf(F("%s\r\n"), readString());
	}
#endif

	return 1;
}

int SppcClass::beginIP6()
{
	if(!command(F("net ioctl get ipaddr6 0")))
		return 0;

	if(!strcmp(readString(), "::0"))
	{
#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> sppc_begin: IPv6 not enabled"));
#endif
		return 0;
	}

	flags |= PF_IP6;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		sppc_printf(F("log> sppc_begin: IPv6 "));

		command(F("net ioctl get ipaddr6 0"));
		sppc_printf(F("%s "), readString());

		command(F("net ioctl get ipaddr6 1"));
		sppc_printf(F("%s/"), readString());

		sppc_printf(F("%d\r\n"), command(F("net ioctl get prefix6")));

		sppc_printf(F("log> sppc_begin: IPv6 "));

		command(F("net ioctl get gwaddr6"));
		sppc_printf(F("%s "), readString());

		command(F("net ioctl get nsaddr6"));
		sppc_printf(F("%s\r\n"), readString());
	}
#endif

	return 1;
}

#define MAX_BOOT_WAIT 500

int SppcClass::begin(uint16_t init_flags)
{
	int retval, net_id, wait_count;
	uint16_t status;
	char *retstr;

	flags |= init_flags;

	if(!(flags & PF_INIT_SPI))
	{
		pinMode(SPI_NSS_PIN, OUTPUT);
		digitalWrite(SPI_NSS_PIN, HIGH);

		SPI.begin();
	}

	wait_count = 0;
	errno = 0;

_retry_sync:
	status = spi_resync();

	if(!(status & S2M_FLAG_SYNC))
	{
		delay(100);
		wait_count += 100;

		if(wait_count >= MAX_BOOT_WAIT)
		{
			errno = EPERM;
			return 0;
		}
		else
			goto _retry_sync;
	}

	flags |= PF_SYNC_V2;

	/* we should set PF_SHIELD flag after spi_resync success */
	flags |= PF_SHIELD;

	Sppc.command(F("uio1 ioctl get 11 mode"));

	if(!strcmp(Sppc.readString(), "und"))
	{
		uint8_t spc_rx;

		Sppc.command(F("uio1 ioctl set 11 mode in_pd"));

		/* uio1.11(spc_rx) pin is pulled-up with 1Kohms on phpoc shield 2 */
		if(Sppc.command(F("uio1 ioctl get 11 input")))
			flags |= PF_SPC;

		Sppc.command(F("uio1 ioctl set 11 mode und"));
	}
	else
		flags |= PF_SPC;

	Sppc.command(F("uio1 close"));

	pkg_ver_id = command(F("sys pkg ver"));

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
	{
		Serial.print(F("log> sppc_begin: "));

		command(F("sys uname -i"));

		if(!strcmp(Sppc.readString(), "P4S-348"))
			Serial.print(F("phpoc shield"));
		else
			Serial.print(F("phpoc wifi shield"));

		if(flags & PF_SPC)
			Serial.print(F(" 2, "));
		else
			Serial.print(F(", "));

		command(F("sys uname -v"));
		sppc_printf(F("firmware %s\r\n"), readString());

		sppc_printf(F("log> sppc_begin: package "));
		sppc_printf(F("%d."), pkg_ver_id / 10000);
		sppc_printf(F("%d."), (pkg_ver_id % 10000) / 100);
		sppc_printf(F("%d\r\n"), pkg_ver_id % 100);
	}
#endif

	if(!pkg_ver_id)
		return 0;

#ifdef PF_LOG_NET
	if((flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> sppc_begin: "));
#endif
		
	command(F("net1 ioctl get mode"));
	retstr = readString();

	if(*retstr)
	{ /* WiFi dongle installed */
		net_id = 1;

#ifdef PF_LOG_NET
		if((flags & PF_LOG_NET) && Serial)
		{
			sppc_printf(F("WiFi %s "), retstr);

			if(pkg_ver_id > 10000)
			{
				command(F("net1 ioctl get ssid"));
				sppc_printf(F("%s "), readString());

				retval = command(F("net1 ioctl get ch"));
				sppc_printf(F("ch%d "), retval);

			}
		}
#endif
	}
	else
	{ /* WiFi dongle not installed */
		net_id = 0;

		command(F("net0 ioctl get mode"));
		retstr = readString();

		if(*retstr)
		{ /* ethernet present */
#ifdef PF_LOG_NET
			if((flags & PF_LOG_NET) && Serial)
				sppc_printf(F("Ethernet %s "), retstr);
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
		if(!command(F("net%u ioctl get speed"), net_id))
		{
			if(net_id)
				Serial.print(F("scanning "));
			else
				Serial.print(F("unplugged "));
		}

		Serial.println();
	}
#endif

	/* init app - smtp */
	command(F("php smtp server")); /* clear MSA server */

	return beginIP4();
}

void sppc_printf(const __FlashStringHelper *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;

	if(!Serial)
		return;

	va_start(args, format);
	sppc_vsprintf(vsp_buf, format, args);
	va_end(args);

	Serial.print(vsp_buf);
}

SppcClass Sppc;
