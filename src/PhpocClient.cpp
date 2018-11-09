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

#include "Phpoc.h"

char PhpocClient::read_line_buf[SOCK_LINE_BUF_SIZE + 2];
uint8_t PhpocClient::conn_flags;
uint8_t PhpocClient::init_flags;

PhpocClient::PhpocClient()
{
	sock_id = MAX_SOCK_TCP;
}

PhpocClient::PhpocClient(uint8_t id)
{
	if(id >= MAX_SOCK_TCP)
		sock_id = MAX_SOCK_TCP;
	//else
	//if(id < SOCK_ID_TCP)
	//	sock_id = MAX_SOCK_TCP;
	else
		sock_id = id;
}

uint16_t PhpocClient::command(const __FlashStringHelper *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

	if(Sppc.flags & PF_SYNC_V1)
		return 0;

	cmd_len = sppc_sprintf(vsp_buf, F("tcp%u "), sock_id);

	va_start(args, format);
	cmd_len += sppc_vsprintf(vsp_buf + cmd_len, format, args);
	va_end(args);

	return Sppc.sppc_request(vsp_buf, cmd_len);
}

uint16_t PhpocClient::command(const char *format, ...)
{
	char vsp_buf[VSP_COUNT_LIMIT];
	va_list args;
	int cmd_len;

	if(Sppc.flags & PF_SYNC_V1)
		return 0;

	cmd_len = sppc_sprintf(vsp_buf, F("tcp%u "), sock_id);

	va_start(args, format);
	cmd_len += sppc_vsprintf(vsp_buf + cmd_len, format, args);
	va_end(args);

	return Sppc.sppc_request(vsp_buf, cmd_len);
}

int PhpocClient::connectSSL_ipstr(const char *ipstr, uint16_t port)
{
	uint8_t state;

	sock_id = SOCK_ID_SSL;

	if(!(init_flags & (1 << sock_id)))
	{
		Phpoc.command(F("tcp%u ioctl close"), sock_id);
		init_flags |= (1 << sock_id);
		goto _skip_check_sock;
	}

#ifdef INCLUDE_NET_CACHE
	nc_update(sock_id, NC_FLAG_RENEW_RXLEN | NC_FLAG_RENEW_STATE);
	state = nc_tcp_state[sock_id];
#else
	state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
#endif

	/* Phpoc/Sppc.command() function returns 0 on error.
	 * we should check Sppc.errno if state is 0(TCP_CLOSED)
	 */
	if((state != TCP_CLOSED) || Sppc.errno || available())
	{
#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> phpoc_client: SSL socket not available"));
#endif
		return 0;
	}

_skip_check_sock:

	if(conn_flags & (1 << sock_id))
	{
#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			sppc_printf(F("log> phpoc_client: closed %d\r\n"), sock_id);
#endif
		conn_flags &= ~(1 << sock_id);
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		sppc_printf(F("log> phpoc_client: connectSSL %d >> "), sock_id);
#endif

	Phpoc.command(F("tcp%u ioctl set api ssl"), sock_id);

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		Phpoc.command(F("tcp%u ioctl set ssl method tls1_client"), sock_id);
	else
#endif
		Phpoc.command(F("tcp%u ioctl set ssl method client"), sock_id);

	Phpoc.command(F("tcp%u connect %s %u"), sock_id, ipstr, port);

	while(1)
	{
		state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);

		if((state == TCP_CLOSED) || (state == SSL_CONNECTED))
			break;

		delay(10);
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
	{
		if(state)
			Serial.println(F("success"));
		else
			Serial.println(F("failed"));
	}
#endif

	if(state)
	{
#ifdef INCLUDE_NET_CACHE
		nc_init(sock_id, SSL_CONNECTED);
#endif
		conn_flags |= (1 << sock_id);
		return 1;
	}
	else
	{
#ifdef INCLUDE_NET_CACHE
		nc_init(sock_id, TCP_CLOSED);
#endif
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
}

int PhpocClient::connectSSL(IP6Address ip6addr, uint16_t port)
{
	return connectSSL_ipstr(ip6addr.toString(), port);
}

int PhpocClient::connectSSL(IPAddress ipaddr, uint16_t port)
{
	char ipstr[16]; /* x.x.x.x (7 bytes), xxx.xxx.xxx.xxx (15 bytes) */

	sppc_sprintf(ipstr, F("%u.%u.%u.%u"), ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	return connectSSL_ipstr(ipstr, port);
}

int PhpocClient::connectSSL(const char *host, uint16_t port)
{
	int len;

	if(Sppc.flags & PF_IP6)
	{
		IP6Address ip6addr;

		Phpoc.getHostByName6(host, ip6addr, 500);
		if(ip6addr == IN6ADDR_NONE)
			Phpoc.getHostByName6(host, ip6addr, 2000);

		if(ip6addr != IN6ADDR_NONE)
			return connectSSL(ip6addr, port);
		else
			return 0;
	}
	else
	{
		IPAddress ipaddr;

		Phpoc.getHostByName(host, ipaddr, 500);
		if(ipaddr == INADDR_NONE)
			Phpoc.getHostByName(host, ipaddr, 2000);

		if(ipaddr != INADDR_NONE)
			return connectSSL(ipaddr, port);
		else
			return 0;
	}

}

int PhpocClient::connect_ipstr(const char *ipstr, uint16_t port)
{
	uint8_t state;

	if(sock_id < MAX_SOCK_TCP)
		return 0;

	for(sock_id = SOCK_ID_TCP; sock_id < MAX_SOCK_TCP; sock_id++)
	{
		if(!(init_flags & (1 << sock_id)))
		{
			Phpoc.command(F("tcp%u ioctl close"), sock_id);
			init_flags |= (1 << sock_id);
			break;
		}

#ifdef INCLUDE_NET_CACHE
		nc_update(sock_id, NC_FLAG_RENEW_RXLEN | NC_FLAG_RENEW_STATE);
		state = nc_tcp_state[sock_id];
#else
		state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
#endif

		/* Phpoc/Sppc.command() function returns 0 on error.
		 * we should check Sppc.errno if state is 0(TCP_CLOSED)
		 */
		if((state == TCP_CLOSED) && !Sppc.errno && !available())
			break;
	}

	if(sock_id == MAX_SOCK_TCP)
	{
#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> phpoc_client: socket not available"));
#endif
		return 0;
	}

	if(conn_flags & (1 << sock_id))
	{
#ifdef PF_LOG_NET
		if((Sppc.flags & PF_LOG_NET) && Serial)
			sppc_printf(F("log> phpoc_client: closed %d\r\n"), sock_id);
#endif
		conn_flags &= ~(1 << sock_id);
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		sppc_printf(F("log> phpoc_client: connect %d >> "), sock_id);
#endif

	Phpoc.command(F("tcp%u ioctl set api tcp"), sock_id);
	Phpoc.command(F("tcp%u connect %s %u"), sock_id, ipstr, port);

	while(1)
	{
		state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);

		if((state == TCP_CLOSED) || (state == TCP_CONNECTED))
			break;

		delay(10);
	}

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
	{
		if(state)
			Serial.println(F("success"));
		else
			Serial.println(F("failed"));
	}
#endif

	if(state)
	{
#ifdef INCLUDE_NET_CACHE
		nc_init(sock_id, TCP_CONNECTED);
#endif
		conn_flags |= (1 << sock_id);
		return 1;
	}
	else
	{
#ifdef INCLUDE_NET_CACHE
		nc_init(sock_id, TCP_CLOSED);
#endif
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
}

int PhpocClient::connect(IP6Address ip6addr, uint16_t port)
{
		return connect_ipstr(ip6addr.toString(), port);
}

int PhpocClient::connect(IPAddress ipaddr, uint16_t port)
{
	char ipstr[16]; /* x.x.x.x (7 bytes), xxx.xxx.xxx.xxx (15 bytes) */

	sppc_sprintf(ipstr, F("%u.%u.%u.%u"), ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	return connect_ipstr(ipstr, port);
}

int PhpocClient::connect(const char *host, uint16_t port)
{
	int len;

	if(Sppc.flags & PF_IP6)
	{
		IP6Address ip6addr;

		Phpoc.getHostByName6(host, ip6addr, 500);
		if(ip6addr == IN6ADDR_NONE)
			Phpoc.getHostByName6(host, ip6addr, 2000);

		if(ip6addr != IN6ADDR_NONE)
			return connect(ip6addr, port);
		else
			return 0;
	}
	else
	{
		IPAddress ipaddr;

		Phpoc.getHostByName(host, ipaddr, 500);
		if(ipaddr == INADDR_NONE)
			Phpoc.getHostByName(host, ipaddr, 2000);

		if(ipaddr != INADDR_NONE)
			return connect(ipaddr, port);
		else
			return 0;
	}
}

size_t PhpocClient::write(uint8_t byte)
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

	return write(&byte, 1);
}

size_t PhpocClient::write(const uint8_t *buf, size_t size)
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

#ifdef INCLUDE_NET_CACHE
	return nc_write(sock_id, buf, size);
#else /* INCLUDE_NET_CACHE */
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("tcp%u send"), sock_id);

		if(!Sppc.errno)
			return Phpoc.write(buf, size);
		else
			return 0;
	}
	else
#endif
	{
		Phpoc.write(buf, size);

		if(!Sppc.errno)
			return Phpoc.command(F("tcp%u send"), sock_id);
		else
			return 0;
	}
#endif
}

int PhpocClient::available()
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

#ifdef INCLUDE_NET_CACHE
	nc_update(sock_id, 0);
	return nc_read_len[sock_id] + nc_tcp_rxlen[sock_id];
#else
	return Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);
#endif
}

int PhpocClient::read()
{
	uint8_t byte;

	if(sock_id >= MAX_SOCK_TCP)
		return EOF;

#ifdef INCLUDE_NET_CACHE
	return nc_read(sock_id);
#else
	if(Phpoc.command(F("tcp%u recv 1"), sock_id) > 0)
	{
		Phpoc.read(&byte, 1);
		return (int)byte;
	}
	else
		return EOF;
#endif
}

int PhpocClient::read(uint8_t *buf, size_t size)
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

	if(!size)
		return 0;

#ifdef INCLUDE_NET_CACHE
	return nc_read(sock_id, buf, size);
#else
	int len;

	len = Phpoc.command(F("tcp%u recv %u"), sock_id, size);

	if(len > 0)
	{
		Phpoc.read(buf, len);
		return len;
	}
	else
		return 0;
#endif
}

char *PhpocClient::readLine()
{
	int len;

	read_line_buf[0] = 0x00;

	if(sock_id >= MAX_SOCK_TCP)
		return read_line_buf;

	if((len = readLine((uint8_t *)read_line_buf, SOCK_LINE_BUF_SIZE)))
		read_line_buf[len] = 0x00;

	return read_line_buf;
}

int PhpocClient::readLine(uint8_t *buf, size_t size)
{
	int line_len, rxlen;

	if(sock_id >= MAX_SOCK_TCP)
		return 0;

	if(!size)
		return 0;

#ifdef INCLUDE_NET_CACHE
	return nc_read_line(sock_id, buf, size);
#else /* INCLUDE_NET_CACHE */

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("tcp%u ioctl get rxlen 0d0a"), sock_id);

		if(!Sppc.errno)
			line_len = Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
		line_len = Phpoc.command(F("tcp%u ioctl get rxlen \r\n"), sock_id);

	if(line_len)
	{
		if(line_len > size)
			line_len = size;

		rxlen = Phpoc.command(F("tcp%u recv %u"), sock_id, line_len);

		if(rxlen > 0)
		{
			Phpoc.read(buf, rxlen);
			return rxlen;
		}
		else
			return 0;
	}
	else
		return 0;
#endif
}

int PhpocClient::availableForWrite(void)
{
	return Phpoc.tcpIoctlReadInt(F("txfree"), sock_id);
}

int PhpocClient::peek()
{
	uint8_t byte;

	if(sock_id >= MAX_SOCK_TCP)
		return EOF;

#ifdef INCLUDE_NET_CACHE
	return nc_peek(sock_id);
#else
	int len;

	len = Phpoc.command(F("tcp%u peek 1"), sock_id);

	if(len > 0)
	{
		Phpoc.read(&byte, 1);
		return (int)byte;
	}
	else
		return EOF;
#endif
}

void PhpocClient::flush()
{
	if(sock_id >= MAX_SOCK_TCP)
		return;

#ifdef INCLUDE_NET_CACHE
	nc_update(sock_id, NC_FLAG_FLUSH_WRITE);
#endif

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		while(Phpoc.tcpIoctlReadInt(F("txlen"), sock_id))
			delay(10);
	}
	else
#endif
	{
		int txfree, txbuf;

		while(1)
		{
			txfree = Phpoc.tcpIoctlReadInt(F("txfree"), sock_id);
			txbuf = Phpoc.tcpIoctlReadInt(F("txbuf"), sock_id);

			if(txfree == txbuf)
				break;

			delay(10);
		}
	}
}

void PhpocClient::stop()
{
	if(sock_id >= MAX_SOCK_TCP)
		return;

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		sppc_printf(F("log> phpoc_client: close %d >> "), sock_id);
#endif

  Phpoc.command(F("tcp%u ioctl close"), sock_id);

	while(Phpoc.tcpIoctlReadInt(F("state"), sock_id) != TCP_CLOSED)
		delay(10);

#ifdef INCLUDE_NET_CACHE
	nc_init(sock_id, TCP_CLOSED);
#endif

#ifdef PF_LOG_NET
	if((Sppc.flags & PF_LOG_NET) && Serial)
		Serial.println(F("closed"));
#endif

	conn_flags &= ~(1 << sock_id);

	sock_id = MAX_SOCK_TCP;
}

uint8_t PhpocClient::connected()
{
	int state;

	if(sock_id >= MAX_SOCK_TCP)
		return false;

#ifdef INCLUDE_NET_CACHE
	nc_update(sock_id, 0);
	state = nc_tcp_state[sock_id];

	/* connected() should return true if ssl renegotiation is in progress */
	if((state == TCP_CONNECTED) || (state > SSL_STOP) || (state == SSH_CONNECTED))
		return true;
	else
	{
		if(available())
			return true;
		else
		{
			nc_update(sock_id, NC_FLAG_RENEW_RXLEN);

			if(available())
				return true;
			else
				return false;
		}
	}
#else
	state = Phpoc.tcpIoctlReadInt(F("state"), sock_id);

	if((state == TCP_CONNECTED) || (state == SSL_CONNECTED) || (state == SSH_CONNECTED))
		return true;
	else
	{
		if(available())
			return true;
		else
			return false;
	}
#endif
}

PhpocClient::operator bool()
{
	return sock_id < MAX_SOCK_TCP;
}

bool PhpocClient::operator==(const PhpocClient& rhs)
{
	return (sock_id == rhs.sock_id) && (sock_id < MAX_SOCK_TCP);
}


