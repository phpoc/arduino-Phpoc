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

#ifdef INCLUDE_PHPOC_CACHE
uint32_t PhpocClient::tick_32ms[MAX_SOCK_TCP];
uint8_t  PhpocClient::state_32ms[MAX_SOCK_TCP];
uint16_t PhpocClient::rxlen_32ms[MAX_SOCK_TCP];
uint8_t  PhpocClient::read_cache_len[MAX_SOCK_TCP];
uint8_t  PhpocClient::write_cache_len[MAX_SOCK_TCP];
uint8_t  PhpocClient::read_cache_buf[MAX_SOCK_TCP][READ_CACHE_SIZE];
uint8_t  PhpocClient::write_cache_buf[MAX_SOCK_TCP][WRITE_CACHE_SIZE];
#endif
char PhpocClient::read_line_buf[LINE_BUF_SIZE + 2];

PhpocClient::PhpocClient()
{
	sock_id = MAX_SOCK_TCP;
}

PhpocClient::PhpocClient(uint8_t id)
{
	if(id >= MAX_SOCK_TCP)
		sock_id = MAX_SOCK_TCP;
	else
	if(id < SOCK_ID_TCP)
		sock_id = MAX_SOCK_TCP;
	else
		sock_id = id;
}

#ifdef INCLUDE_PHPOC_CACHE
void PhpocClient::update_cache(uint8_t id)
{
	int len;

	if(rxlen_32ms[id] && !read_cache_len[id])
	{
		len = Phpoc.command(F("tcp%u recv %u"), id, READ_CACHE_SIZE);

		if(len > 0)
		{
			Phpoc.read(read_cache_buf[id] + READ_CACHE_SIZE - len, len);
			read_cache_len[id] = len;

			if(Phpoc.command(F("tcp%u get rxlen"), id) <= 0)
				rxlen_32ms[id] = 0;
			else
				rxlen_32ms[id] = Phpoc.readInt();
		}
	}

	if(tick_32ms[id] == (millis() & 0xffffffe0))
		return;

	if(write_cache_len[id])
	{
		if(Phpoc.command(F("tcp%u send"), id) >= 0)
			Phpoc.write(write_cache_buf[id], write_cache_len[id]);
		write_cache_len[id] = 0;
	}

	if(Phpoc.command(F("tcp%u get state"), id) <= 0)
		state_32ms[id] = 0;
	else
		state_32ms[id] = Phpoc.readInt();

	if(Phpoc.command(F("tcp%u get rxlen"), id) <= 0)
		rxlen_32ms[id] = 0;
	else
		rxlen_32ms[id] = Phpoc.readInt();

	tick_32ms[id] = millis() & 0xffffffe0;
}
#endif

int PhpocClient::connectSSL_ipstr(const char *ipstr, uint16_t port)
{
	uint8_t state;

	sock_id = SOCK_ID_SSL;

	if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
	{
		if(Phpoc.readInt() == TCP_CLOSED)
		{
#ifdef INCLUDE_PHPOC_CACHE
			if(read_cache_len[sock_id])
				goto _sock_na;
#endif

			if(Phpoc.command(F("tcp%u get rxlen"), sock_id) <= 0)
				goto _sock_na;

			if(Phpoc.readInt())
				goto _sock_na;
		}
		else
			goto _sock_na;
	}
	else
	{
_sock_na:
#ifdef PF_LOG_NET
		if((Phpoc.flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> phpoc_client: SSL socket not available"));
#endif
		return 0;
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
		Serial.print(F("log> phpoc_client: connectSSL >> "));
#endif

	if(Phpoc.command(F("tcp%u connect %s %u"), sock_id, ipstr, port) < 0)
		return 0;

	if(Phpoc.command(F("tcp%u set ssl method tls1_client"), sock_id) < 0)
		return 0;

	while(1)
	{
		if(Phpoc.command(F("tcp%u get state"), sock_id) <= 0)
		{
			state = 0;
			break;
		}

		state = Phpoc.readInt();

		if((state == TCP_CLOSED) || (state == SSL_CONNECTED))
			break;

		delay(10);
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		if(state)
			Serial.println(F("success"));
		else
			Serial.println(F("failed"));
	}
#endif

#ifdef INCLUDE_PHPOC_CACHE
	rxlen_32ms[sock_id] = 0;
	read_cache_len[sock_id] = 0;
	write_cache_len[sock_id] = 0;

	if(state)
	{
		state_32ms[sock_id] = SSL_CONNECTED;
		return 1;
	}
	else
	{
		state_32ms[sock_id] = TCP_CLOSED;
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
#else
	if(state)
		return 1;
	else
	{
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
#endif
}

int PhpocClient::connectSSL(IP6Address ip6addr, uint16_t port)
{
	return connectSSL_ipstr(ip6addr.toString(), port);
}

int PhpocClient::connectSSL(IPAddress ipaddr, uint16_t port)
{
	char ipstr[16]; /* x.x.x.x (7 bytes), xxx.xxx.xxx.xxx (15 bytes) */

	Phpoc.sprintf(ipstr, F("%u.%u.%u.%u"), ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	return connectSSL_ipstr(ipstr, port);
}

int PhpocClient::connectSSL(const char *host, uint16_t port)
{
	int len;

	if(Phpoc.flags & PF_IP6)
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
		if(Phpoc.command(F("tcp%u get state"), sock_id) > 0)
		{
			state = Phpoc.readInt();

			if(state == TCP_CLOSED)
			{
#ifdef INCLUDE_PHPOC_CACHE
				if(read_cache_len[sock_id])
					continue;
#endif

				if(Phpoc.command(F("tcp%u get rxlen"), sock_id) <= 0)
					continue;

				if(Phpoc.readInt())
					continue;

				break;
			}
		}
	}

	if(sock_id == MAX_SOCK_TCP)
	{
#ifdef PF_LOG_NET
		if((Phpoc.flags & PF_LOG_NET) && Serial)
			Serial.println(F("log> phpoc_client: socket not available"));
#endif
		return 0;
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(F("log> phpoc_client: connect "));
		Serial.print(sock_id);
		Serial.print(F(" >> "));
	}
#endif

	if(Phpoc.command(F("tcp%u set api tcp"), sock_id) < 0)
		return 0;

	if(Phpoc.command(F("tcp%u connect %s %u"), sock_id, ipstr, port) < 0)
		return 0;

	while(1)
	{
		if(Phpoc.command(F("tcp%u get state"), sock_id) <= 0)
		{
			state = 0;
			break;
		}

		state = Phpoc.readInt();

		if((state == TCP_CLOSED) || (state == TCP_CONNECTED))
			break;

		delay(10);
	}

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		if(state)
			Serial.println(F("success"));
		else
			Serial.println(F("failed"));
	}
#endif

#ifdef INCLUDE_PHPOC_CACHE
	rxlen_32ms[sock_id] = 0;
	read_cache_len[sock_id] = 0;
	write_cache_len[sock_id] = 0;

	if(state)
	{
		state_32ms[sock_id] = TCP_CONNECTED;
		return 1;
	}
	else
	{
		state_32ms[sock_id] = TCP_CLOSED;
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
#else
	if(state)
		return 1;
	else
	{
		sock_id = MAX_SOCK_TCP;
		return 0;
	}
#endif
}

int PhpocClient::connect(IP6Address ip6addr, uint16_t port)
{
		return connect_ipstr(ip6addr.toString(), port);
}

int PhpocClient::connect(IPAddress ipaddr, uint16_t port)
{
	char ipstr[16]; /* x.x.x.x (7 bytes), xxx.xxx.xxx.xxx (15 bytes) */

	Phpoc.sprintf(ipstr, F("%u.%u.%u.%u"), ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	return connect_ipstr(ipstr, port);
}

int PhpocClient::connect(const char *host, uint16_t port)
{
	int len;

	if(Phpoc.flags & PF_IP6)
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

#ifdef INCLUDE_PHPOC_CACHE
	if(write_cache_len[sock_id] + size >= WRITE_CACHE_SIZE)
	{
		if(write_cache_len[sock_id])
		{
			if(Phpoc.command(F("tcp%u send"), sock_id) >= 0)
				Phpoc.write(write_cache_buf[sock_id], write_cache_len[sock_id]);
			write_cache_len[sock_id] = 0;
		}

		if(size)
		{
			if(Phpoc.command(F("tcp%u send"), sock_id) >= 0)
				Phpoc.write(buf, size);
		}

		return size;
	}
	else
	{
		memcpy(write_cache_buf[sock_id] + write_cache_len[sock_id], buf, size);
		write_cache_len[sock_id] += size;
		return size;
	}
#else
	if(Phpoc.command(F("tcp%u send"), sock_id) >= 0)
		return Phpoc.write(buf, size);
	else
		return 0;
#endif
}

int PhpocClient::available()
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

#ifdef INCLUDE_PHPOC_CACHE
	update_cache(sock_id);

	return read_cache_len[sock_id] + rxlen_32ms[sock_id];
#else
	int len;

	if(Phpoc.command(F("tcp%u get rxlen"), sock_id) <= 0)
		return 0;

	len = Phpoc.readInt();
	return len;
#endif
}

int PhpocClient::read()
{
	uint8_t byte;

	if(sock_id >= MAX_SOCK_TCP)
		return -1;

#ifdef INCLUDE_PHPOC_CACHE
	update_cache(sock_id);

	if(read_cache_len[sock_id])
	{
		int cache_offset;

		cache_offset = READ_CACHE_SIZE - read_cache_len[sock_id];
		byte = read_cache_buf[sock_id][cache_offset];
		read_cache_len[sock_id]--;
		return (int)byte;
	}
	else
		return -1;
#else
	int len;

	len = Phpoc.command(F("tcp%u recv 1"), sock_id);
	if(len <= 0)
		return -1;

	Phpoc.read(&byte, 1);
	return (int)byte;
#endif
}

int PhpocClient::read(uint8_t *buf, size_t size)
{
	if(sock_id >= MAX_SOCK_TCP)
		return 0;

	if(!size)
		return 0;

#ifdef INCLUDE_PHPOC_CACHE
	int copy_len, recv_len;

	if(read_cache_len[sock_id])
	{
		int cache_offset;

		if(size > read_cache_len[sock_id])
		{
			copy_len = read_cache_len[sock_id];
			recv_len = size - read_cache_len[sock_id];
		}
		else
		{
			copy_len = size;
			recv_len = 0;
		}

		cache_offset = READ_CACHE_SIZE - read_cache_len[sock_id];
		memcpy(buf, read_cache_buf[sock_id] + cache_offset, copy_len);
		read_cache_len[sock_id] -= copy_len;
	}
	else
	{
		copy_len = 0;
		recv_len = size;
	}

	if(recv_len)
	{
		recv_len = Phpoc.command(F("tcp%u recv %u"), sock_id, recv_len);
		if(recv_len <= 0)
			return copy_len;

		Phpoc.read(buf + copy_len, recv_len);

		if(Phpoc.command(F("tcp%u get rxlen"), sock_id) <= 0)
			rxlen_32ms[sock_id] = 0;
		else
			rxlen_32ms[sock_id] = Phpoc.readInt();
	}

	update_cache(sock_id);

	return copy_len + recv_len;
#else
	int len;

	len = Phpoc.command(F("tcp%u recv %u"), sock_id, size);
	if(len <= 0)
		return 0;

	Phpoc.read(buf, len);
	return len;
#endif
}

char *PhpocClient::readLine()
{
	int len;

	read_line_buf[0] = 0x00;

	if(sock_id >= MAX_SOCK_TCP)
		return read_line_buf;

	if((len = readLine((uint8_t *)read_line_buf, LINE_BUF_SIZE)))
		read_line_buf[len] = 0x00;

	return read_line_buf;
}

#ifdef INCLUDE_PHPOC_CACHE
static int find_crlf(uint8_t *buf, int size)
{
	uint8_t crlf_count;
	uint8_t *ptr;

	ptr = buf;
	crlf_count = 0;

	while(size)
	{
		if(crlf_count)
		{
			if(*ptr == 0x0a)
				return ptr + 1 - buf;
			else
				crlf_count = 0;
		}
		else
		{
			if(*ptr == 0x0d)
				crlf_count++;
		}

		ptr++;
		size--;
	}

	return 0;
}

int PhpocClient::read_line_from_cache(uint8_t *buf, size_t size)
{
	if(read_cache_len[sock_id])
	{
		int cache_offset, line_len;

		cache_offset = READ_CACHE_SIZE - read_cache_len[sock_id];

		line_len = find_crlf(read_cache_buf[sock_id] + cache_offset, read_cache_len[sock_id]);

		if(line_len)
		{
			if(line_len > size)
				line_len = size;

			memcpy(buf, read_cache_buf[sock_id] + cache_offset, line_len);
			read_cache_len[sock_id] -= line_len;
		}

		return line_len;
	}
	else
		return 0;
}
#endif

int PhpocClient::readLine(uint8_t *buf, size_t size)
{
	int line_len, rxlen;

	if(sock_id >= MAX_SOCK_TCP)
		return 0;

	if(!size)
		return 0;

#ifdef INCLUDE_PHPOC_CACHE
	line_len = read_line_from_cache(buf, size);

	if(!line_len)
	{
		uint8_t *cache_ptr;
		int cache_offset;

		if(read_cache_len[sock_id] == READ_CACHE_SIZE)
			goto _flush_cache;

		cache_offset = READ_CACHE_SIZE - read_cache_len[sock_id];

		rxlen = Phpoc.command(F("tcp%u recv %u"), sock_id, cache_offset);
		if(rxlen < 0)
			return 0;

		if(rxlen)
		{
			cache_ptr = read_cache_buf[sock_id] + cache_offset;
			memcpy(cache_ptr - rxlen, cache_ptr, read_cache_len[sock_id]);

			cache_offset -= rxlen;

			Phpoc.read(read_cache_buf[sock_id] + cache_offset + read_cache_len[sock_id], rxlen);
			read_cache_len[sock_id] += rxlen;

			if(Phpoc.command(F("tcp%u get rxlen"), sock_id) <= 0)
				rxlen_32ms[sock_id] = 0;
			else
				rxlen_32ms[sock_id] = Phpoc.readInt();

			line_len = read_line_from_cache(buf, size);

			if(!line_len)
			{
				if(read_cache_len[sock_id] == READ_CACHE_SIZE)
				{
_flush_cache:
					line_len = read_cache_len[sock_id];

					if(line_len > size)
						line_len = size;

					memcpy(buf, read_cache_buf[sock_id], line_len);
					read_cache_len[sock_id] -= line_len;
				}
			}
		}
	}

	update_cache(sock_id);

	return line_len;
#else
	if(Phpoc.command(F("tcp%u get rxlen 0d0a"), sock_id) <= 0)
		return 0;

	line_len = Phpoc.readInt();

	if(line_len)
	{
		if(line_len > size)
			line_len = size;

		rxlen = Phpoc.command(F("tcp%u recv %u"), sock_id, line_len);
		if(rxlen <= 0)
			return 0;

		Phpoc.read(buf, rxlen);
		return rxlen;
	}
	else
		return 0;
#endif
}

int PhpocClient::peek()
{
	uint8_t byte;
	int len;

	if(sock_id >= MAX_SOCK_TCP)
		return -1;

#ifdef INCLUDE_PHPOC_CACHE
	update_cache(sock_id);

	if(read_cache_len[sock_id])
	{
		int cache_offset;

		cache_offset = READ_CACHE_SIZE - read_cache_len[sock_id];
		byte = read_cache_buf[sock_id][cache_offset];
		return (int)byte;
	}
	else
		return -1;
#else
	len = Phpoc.command(F("tcp%u peek 1"), sock_id);
	if(len <= 0)
		return -1;

	Phpoc.read(&byte, 1);
	return (int)byte;
#endif
}

void PhpocClient::flush()
{
	if(sock_id >= MAX_SOCK_TCP)
		return;

#ifdef INCLUDE_PHPOC_CACHE
	update_cache(sock_id);
#endif

	while(1)
	{
		if(Phpoc.command(F("tcp%u get txlen"), sock_id) <= 0)
			break;
		if(!Phpoc.readInt())
			break;
	}
}

void PhpocClient::stop()
{
	if(sock_id >= MAX_SOCK_TCP)
		return;

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
	{
		Serial.print(F("log> phpoc_client: close "));
		Serial.print(sock_id);
		Serial.print(F(" >> "));
	}
#endif

  if(Phpoc.command(F("tcp%u close"), sock_id) < 0)
		return;

	while(1)
	{
		if(Phpoc.command(F("tcp%u get state"), sock_id) <= 0)
			break;

		if(Phpoc.readInt() == TCP_CLOSED)
			break;

		delay(10);
	}

#ifdef INCLUDE_PHPOC_CACHE
	state_32ms[sock_id] = TCP_CLOSED;
	rxlen_32ms[sock_id] = 0;
	read_cache_len[sock_id] = 0;
	write_cache_len[sock_id] = 0;
#endif

#ifdef PF_LOG_NET
	if((Phpoc.flags & PF_LOG_NET) && Serial)
		Serial.println(F("closed"));
#endif

	sock_id = MAX_SOCK_TCP;
}

uint8_t PhpocClient::connected()
{
	int state;

	if(sock_id >= MAX_SOCK_TCP)
		return false;

#ifdef INCLUDE_PHPOC_CACHE
	update_cache(sock_id);

	state = state_32ms[sock_id];

	if((state == TCP_CONNECTED) || (state == SSL_CONNECTED) || (state == SSH_CONNECTED))
		return true;
	else
	{
		if(read_cache_len[sock_id] + rxlen_32ms[sock_id])
			return true;
		else
			return false;
	}
#else
	if(Phpoc.command(F("tcp%u get state"), sock_id) <= 0)
		return false;
	state = Phpoc.readInt();

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


