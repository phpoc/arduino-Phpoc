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

#ifdef INCLUDE_NET_CACHE

#define CTO_RXLEN  50 /* ms unit */
#define CTO_STATE 100 /* ms unit */
#define CTO_WRITE  50 /* ms unit */

#define CT_ID_RXLEN 0
#define CT_ID_STATE 1
#define CT_ID_WRITE 2

#define CT_FLAG_RUN_RXLEN 0x01
#define CT_FLAG_RUN_STATE 0x02
#define CT_FLAG_RUN_WRITE 0x04
#define CT_FLAG_TO_RXLEN  0x10
#define CT_FLAG_TO_STATE  0x20
#define CT_FLAG_TO_WRITE  0x40

/* global variables */
uint8_t  nc_tcp_state[MAX_SOCK_TCP];
uint16_t nc_tcp_rxlen[MAX_SOCK_TCP];
uint8_t  nc_read_len[MAX_SOCK_TCP];
uint8_t  nc_write_len[MAX_SOCK_TCP];
uint8_t  nc_read_buf[MAX_SOCK_TCP][SOCK_READ_CACHE_SIZE];
uint8_t  nc_write_buf[MAX_SOCK_TCP][SOCK_WRITE_CACHE_SIZE];

/* local variables */
static uint16_t ct_t1_rxlen[MAX_SOCK_TCP];
static uint16_t ct_t1_state[MAX_SOCK_TCP];
static uint16_t ct_t1_write[MAX_SOCK_TCP];
static uint8_t  ct_flags[MAX_SOCK_TCP];

static uint16_t ct_elapsed_ms(uint16_t t1_ms16)
{
	uint16_t t2_ms16;

	t2_ms16 = (uint16_t)millis();

	if(t2_ms16 >= t1_ms16)
		return t2_ms16 - t1_ms16;
	else
		return (~t1_ms16 + 1) + t2_ms16;
}

static void ct_start(uint8_t sock_id, uint8_t ct_id)
{
	uint16_t cur_ms16;

	cur_ms16 = (uint16_t)millis();

	switch(ct_id)
	{
		case CT_ID_RXLEN:
			ct_t1_rxlen[sock_id] = cur_ms16;
			ct_flags[sock_id] |= CT_FLAG_RUN_RXLEN;
			break;

		case CT_ID_STATE:
			ct_t1_state[sock_id] = cur_ms16;
			ct_flags[sock_id] |= CT_FLAG_RUN_STATE;
			break;

		case CT_ID_WRITE:
			ct_t1_write[sock_id] = cur_ms16;
			ct_flags[sock_id] |= CT_FLAG_RUN_WRITE;
			break;
	}
}

static void ct_stop(uint8_t sock_id, uint8_t ct_id)
{
	switch(ct_id)
	{
		case CT_ID_RXLEN:
			ct_flags[sock_id] &= ~CT_FLAG_RUN_RXLEN;
			break;

		case CT_ID_STATE:
			ct_flags[sock_id] &= ~CT_FLAG_RUN_STATE;
			break;

		case CT_ID_WRITE:
			ct_flags[sock_id] &= ~CT_FLAG_RUN_WRITE;
			break;
	}
}

static void ct_loop(uint8_t sock_id)
{
	if(ct_flags[sock_id] & CT_FLAG_RUN_RXLEN)
	{
		if(ct_elapsed_ms(ct_t1_rxlen[sock_id]) >= CTO_RXLEN)
		{
			ct_flags[sock_id] &= ~CT_FLAG_RUN_RXLEN;
			ct_flags[sock_id] |= CT_FLAG_TO_RXLEN;
		}
	}

	if(ct_flags[sock_id] & CT_FLAG_RUN_STATE)
	{
		if(ct_elapsed_ms(ct_t1_state[sock_id]) >= CTO_STATE)
		{
			ct_flags[sock_id] &= ~CT_FLAG_RUN_STATE;
			ct_flags[sock_id] |= CT_FLAG_TO_STATE;
		}
	}

	if(ct_flags[sock_id] & CT_FLAG_RUN_WRITE)
	{
		if(ct_elapsed_ms(ct_t1_write[sock_id]) >= CTO_WRITE)
		{
			ct_flags[sock_id] &= ~CT_FLAG_RUN_WRITE;
			ct_flags[sock_id] |= CT_FLAG_TO_WRITE;
		}
	}
}

void nc_init(uint8_t sock_id, int tcp_state)
{
	nc_tcp_state[sock_id] = tcp_state;

	if((tcp_state == TCP_CLOSED) || (tcp_state == SSL_STOP) || (tcp_state == SSH_STOP))
	{
		ct_stop(sock_id, CT_ID_RXLEN);
		ct_stop(sock_id, CT_ID_STATE);
		ct_stop(sock_id, CT_ID_WRITE);
	}
	else
	{
		ct_start(sock_id, CT_ID_RXLEN);
		ct_start(sock_id, CT_ID_STATE);
	}

	nc_read_len[sock_id]  = 0;
	nc_write_len[sock_id] = 0;
	nc_tcp_rxlen[sock_id] = 0;
}

void nc_update(uint8_t sock_id, uint8_t flags)
{
	if(flags & NC_FLAG_RENEW_RXLEN)
	{
		ct_flags[sock_id] &= ~CT_FLAG_RUN_RXLEN;
		ct_flags[sock_id] |= CT_FLAG_TO_RXLEN;
	}

	if(flags & NC_FLAG_RENEW_STATE)
	{
		ct_flags[sock_id] &= ~CT_FLAG_RUN_STATE;
		ct_flags[sock_id] |= CT_FLAG_TO_STATE;
	}

	if(flags & NC_FLAG_FLUSH_WRITE)
	{
		ct_flags[sock_id] &= ~CT_FLAG_RUN_WRITE;
		ct_flags[sock_id] |= CT_FLAG_TO_WRITE;
	}

	ct_loop(sock_id); /* update timer event */

	if(ct_flags[sock_id] & CT_FLAG_TO_RXLEN)
	{
		ct_flags[sock_id] &= ~CT_FLAG_TO_RXLEN;

		nc_tcp_rxlen[sock_id] = Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);
		ct_start(sock_id, CT_ID_RXLEN); /* restart rxlen timer */
	}

	if(ct_flags[sock_id] & CT_FLAG_TO_STATE)
	{
		ct_flags[sock_id] &= ~CT_FLAG_TO_STATE;

		nc_tcp_state[sock_id] = Phpoc.tcpIoctlReadInt(F("state"), sock_id);
		ct_start(sock_id, CT_ID_STATE); /* restart state timer */
	}

	if(ct_flags[sock_id] & CT_FLAG_TO_WRITE)
	{
		ct_flags[sock_id] &= ~CT_FLAG_TO_WRITE;

		if(nc_write_len[sock_id])
		{
#ifdef INCLUDE_LIB_V1
			if(Sppc.flags & PF_SYNC_V1)
			{
				Phpoc.command(F("tcp%u send"), sock_id);
				if(!Sppc.errno)
					Phpoc.write(nc_write_buf[sock_id], nc_write_len[sock_id]);
			}
			else
#endif
			{
				Phpoc.write(nc_write_buf[sock_id], nc_write_len[sock_id]);
				if(!Sppc.errno)
					Phpoc.command(F("tcp%u send"), sock_id);
			}

			nc_write_len[sock_id] = 0;
		}
	}

	if(!nc_read_len[sock_id] && nc_tcp_rxlen[sock_id])
	{
		int len;

		len = Phpoc.command(F("tcp%u recv %u"), sock_id, SOCK_READ_CACHE_SIZE);

		if(len > 0)
		{
			Phpoc.read(nc_read_buf[sock_id] + SOCK_READ_CACHE_SIZE - len, len);
			nc_read_len[sock_id] = len;
		}

		nc_tcp_rxlen[sock_id] = Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);

		ct_start(sock_id, CT_ID_RXLEN); /* restart rxlen timer */
	}
}

int nc_peek(uint8_t sock_id)
{
	nc_update(sock_id, 0);

	if(nc_read_len[sock_id])
	{
		int nc_offset;

		nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];
		return (int)nc_read_buf[sock_id][nc_offset];
	}
	else
		return EOF;
}

int nc_read(uint8_t sock_id)
{
	nc_update(sock_id, 0);

	if(nc_read_len[sock_id])
	{
		int nc_offset;

		nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];
		nc_read_len[sock_id]--;
		return nc_read_buf[sock_id][nc_offset];
	}
	else
		return EOF;
}

int nc_read(uint8_t sock_id, uint8_t *rbuf, size_t rlen)
{
	int copy_len, recv_len;

	nc_update(sock_id, 0);

	if(nc_read_len[sock_id])
	{
		int nc_offset;

		if(rlen > nc_read_len[sock_id])
		{
			copy_len = nc_read_len[sock_id];
			recv_len = rlen - nc_read_len[sock_id];
		}
		else
		{
			copy_len = rlen;
			recv_len = 0;
		}

		nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];
		memcpy(rbuf, nc_read_buf[sock_id] + nc_offset, copy_len);
		nc_read_len[sock_id] -= copy_len;
	}
	else
		return 0;

	if(!nc_tcp_rxlen[sock_id])
		return copy_len;

	if(recv_len)
	{
		recv_len = Phpoc.command(F("tcp%u recv %u"), sock_id, recv_len);

		if(recv_len > 0)
			Phpoc.read(rbuf + copy_len, recv_len);

		nc_tcp_rxlen[sock_id] = Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);

		ct_start(sock_id, CT_ID_RXLEN); /* restart rxlen timer */
	}

	return copy_len + recv_len;
}

int nc_read_line(uint8_t sock_id, uint8_t *rbuf, size_t rlen)
{
	int copy_len, recv_len, nc_offset;
	int copy_drop_len, recv_drop_len;
	uint8_t crlf;

	nc_update(sock_id, 0);

	if(!nc_read_len[sock_id])
		return 0;

	copy_len = 0;
	recv_len = 0;
	copy_drop_len = 0;
	recv_drop_len = 0;

	crlf = 0;

	nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];

	while(nc_offset < SOCK_READ_CACHE_SIZE)
	{
		if(!crlf)
		{
			if(nc_read_buf[sock_id][nc_offset] == 0x0d) /* CR ? */
				crlf++;
		}
		else
		{
			if(nc_read_buf[sock_id][nc_offset] == 0x0a) /* LF ? */
			{
				crlf++;
				copy_len++;
				break;
			}
			else
				crlf = 0;
		}

		nc_offset++;
		copy_len++;
	}

	if(copy_len > rlen)
	{
		copy_drop_len = copy_len - rlen;
		copy_len = rlen;
	}

	if(crlf == 2)
	{ /* CRLF */
		nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];
		
		memcpy(rbuf, nc_read_buf[sock_id] + nc_offset, copy_len);
		nc_read_len[sock_id] -= (copy_len + copy_drop_len);

		return copy_len;
	}

	if(crlf == 1)
	{ /* CR */
		uint8_t byte;

		if(Phpoc.command(F("tcp%u peek 1"), sock_id) <= 0)
			return 0;

		Phpoc.read(&byte, 1);

		if(byte == 0x0a) /* LF ? */
			recv_len = 1;
		else
			return 0;
	}
	else
	{
#ifdef INCLUDE_LIB_V1
		if(Sppc.flags & PF_SYNC_V1)
		{
			Phpoc.command(F("tcp%u ioctl get rxlen 0d0a"), sock_id);

			if(!Sppc.errno)
				recv_len = Phpoc.readInt();
			else
				recv_len = 0;
		}
		else
#endif
			recv_len = Phpoc.command(F("tcp%u ioctl get rxlen \r\n"), sock_id);

		if(!recv_len)
			return 0;
	}

	if(copy_drop_len)
	{
		recv_drop_len = recv_len;
		recv_len = 0;
	}
	else
	{
		if((copy_len + recv_len) > rlen)
		{
			recv_drop_len = (copy_len + recv_len) - rlen;
			recv_len -= recv_drop_len;
		}
	}

	nc_offset = SOCK_READ_CACHE_SIZE - nc_read_len[sock_id];
		
	memcpy(rbuf, nc_read_buf[sock_id] + nc_offset, copy_len);
	nc_read_len[sock_id] -= (copy_len + copy_drop_len);

	if(recv_len)
	{
		recv_len = Phpoc.command(F("tcp%u recv %u"), sock_id, recv_len);

		if(recv_len > 0)
			Phpoc.read(rbuf + copy_len, recv_len);
		else
			return 0;

		nc_tcp_rxlen[sock_id] = Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);
		ct_start(sock_id, CT_ID_RXLEN); /* restart rxlen timer */
	}

	if(recv_drop_len)
	{
		while(recv_drop_len)
		{
			uint8_t drop_buf[16];
			int len;

			if(recv_drop_len > 16)
				len = 16;
			else
				len = recv_drop_len;

			len = Phpoc.command(F("tcp%u recv %u"), sock_id, len);
			Phpoc.read(drop_buf, len);

			recv_drop_len -= len;
		}

		nc_tcp_rxlen[sock_id] = Phpoc.tcpIoctlReadInt(F("rxlen"), sock_id);
		ct_start(sock_id, CT_ID_RXLEN); /* restart rxlen timer */
	}

	return copy_len + recv_len;
}

int nc_write(uint8_t sock_id, const uint8_t *wbuf, size_t wlen)
{
	int wcnt;

	wcnt = 0;

	if(nc_write_len[sock_id] + wlen >= SOCK_WRITE_CACHE_SIZE)
	{
		if(nc_write_len[sock_id])
		{
			int frag;

			if((frag = SOCK_WRITE_CACHE_SIZE - nc_write_len[sock_id]))
			{
				memcpy(nc_write_buf[sock_id] + nc_write_len[sock_id], wbuf, frag);
				wbuf += frag;
				wcnt += frag;
				wlen -= frag;
			}

#ifdef INCLUDE_LIB_V1
			if(Sppc.flags & PF_SYNC_V1)
			{
				Phpoc.command(F("tcp%u send"), sock_id);
				if(!Sppc.errno)
					Phpoc.write(nc_write_buf[sock_id], SOCK_WRITE_CACHE_SIZE);
			}
			else
#endif
			{
				Phpoc.write(nc_write_buf[sock_id], SOCK_WRITE_CACHE_SIZE);
				if(!Sppc.errno)
					Phpoc.command(F("tcp%u send"), sock_id);
			}

			nc_write_len[sock_id] = 0;
		}

		if(wlen >= SOCK_WRITE_CACHE_SIZE)
		{
#ifdef INCLUDE_LIB_V1
			if(Sppc.flags & PF_SYNC_V1)
			{
				Phpoc.command(F("tcp%u send"), sock_id);
				if(!Sppc.errno)
					Phpoc.write(wbuf, wlen);
			}
			else
#endif
			{
				Phpoc.write(wbuf, wlen);
				if(!Sppc.errno)
					Phpoc.command(F("tcp%u send"), sock_id);
			}

			wcnt += wlen;
			wlen = 0;
		}

		ct_stop(sock_id, CT_ID_WRITE);
	}

	if(wlen)
	{
		if(!nc_write_len[sock_id])
			ct_start(sock_id, CT_ID_WRITE);

		memcpy(nc_write_buf[sock_id] + nc_write_len[sock_id], wbuf, wlen);
		nc_write_len[sock_id] += wlen;

		wcnt += wlen;
		wlen = 0;
	}

	nc_update(sock_id, 0);

	return wcnt;
}

#endif /* INCLUDE_NET_CACHE */
