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

#ifndef Sppc_h
#define Sppc_h

#include <Arduino.h>

/* PHPoC flags */
#define PF_INIT_SPI  0x0001
#define PF_SHIELD    0x0002 /* PHPoC shield installed */
#define PF_SYNC_V1   0x0004 /* protocol V1 enabled */
#define PF_SYNC_V2   0x0008 /* protocol V2 enabled */
#define PF_IP4       0x0010 /* IPv4 enabled */
#define PF_IP6       0x0020 /* IPv6 enabled */
#define PF_SPC       0x0040 /* SPC connector detected */
#define PF_EXPANSION 0x0080 /* Smart Expansion initialized */
#define PF_LOG_SPI   0x0100
#define PF_LOG_NET   0x0200
#define PF_LOG_APP   0x0400

/* SPPC protocol flags */
#define S2M_FLAG_SYNC 0x8000 /* SPI SYNC ok */
#define S2M_FLAG_RXB  0x4000 /* data is in rx buffer */
#define S2M_FLAG_TXB  0x2000 /* data is in tx buffer */
#define S2M_FLAG_CRC  0x1000
#define S2M_FLAG_BID  0x0800
#define M2S_FLAG_CRC  0x1000
#define M2S_FLAG_BID  0x0800

/* SPPC protocol commands */
#define CMD_STATUS    0x0000
#define CMD_TXLEN_B0  0x2000
#define CMD_RXFREE_B0 0x4000
#define CMD_READ_B0   0x6000
#define CMD_WRITE_B0  0x8000
#define CMD_TXLEN_B1  (0x2000 | M2S_FLAG_BID)
#define CMD_RXFREE_B1 (0x4000 | M2S_FLAG_BID)
#define CMD_READ_B1   (0x6000 | M2S_FLAG_BID)
#define CMD_WRITE_B1  (0x8000 | M2S_FLAG_BID)

/* SPPC protocol buffer ID */
#define BID_CMD  0
#define BID_DATA 1

/* PHPoC shield state (not used in source code) */
#define PS_NO_SHIELD      0 /* PHPoC Shield not found */
#define PS_NET_STOP       1 /* ethernet enabled, but unplugged */
#define PS_NET_SCAN       2 /* wifi enabled, but doesn't associated */
#define PS_NET_ADDRESS    3 /* acquiring network address (ip address) */
#define PS_NET_CONNECTED  4 /* ethernet or wifi ready */

/* SPPC error code */
#define EPERM            1  /* Operation not permitted */
#define ENOENT           2  /* No such file or directory */
#define EINTR            4  /* Interrupted system call */
#define ENXIO            6  /* No such device or address */
#define E2BIG            7  /* Arg list too long */
#define EAGAIN          11  /* Try again */
#define ENOMEM          12  /* Out of memory */
#define EBUSY           16  /* Device or resource busy */
#define ENODEV          19  /* No such device */
#define EINVAL          22  /* Invalid argument */
#define EMFILE          24  /* Too many open files */
#define ENOSPC          28  /* No space left on device */
#define EPIPE           32  /* Broken pipe */
#define ENAMETOOLONG    36  /* File name too long */
#define ENOSYS          38  /* Function not implemented */

#define ETIME           62  /* Timer expired */
#define EPROTO          71  /* Protocol error */

#define EADDRINUSE      98  /* Address already in use */
#define EADDRNOTAVAIL   99  /* Cannot assign requested address */
#define ENETDOWN       100  /* Network is down */
#define ENETUNREACH    101  /* Network is unreachable */
#define ECONNRESET     104  /* Connection reset by peer */
#define ETIMEDOUT      110  /* Connection timed out */
#define ECONNREFUSED   111  /* Connection refused */
#define EHOSTUNREACH   113  /* No route to host */

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

#define SSH_AUTH_SIZE   16 /* buffer size for ssh username/password */
#define VSP_COUNT_LIMIT 64 /* string + NULL(0x00) */

#define READSTR_BUF_SIZE 64 /* should be bigger than IPv6 address string */

#ifndef EOF
#define EOF (-1)
#endif

class SppcClass
{
	private:
		/* SPI priviate member functions */
		uint16_t spi_request(uint16_t req);
		uint16_t spi_cmd_status(void);
		uint16_t spi_cmd_sync(void);
		uint16_t spi_resync(void);
		int spi_cmd_txlen(int bid);
		int spi_cmd_rxfree(int bid);
		int spi_wait(int bid, int len, int ms);
		int spi_cmd_read(uint16_t cmd, uint8_t *rbuf, size_t rlen);
		int spi_cmd_write(uint16_t cmd, const uint8_t *wbuf, size_t wlen, boolean pgm);

	private:
		static char readstr_buf[READSTR_BUF_SIZE];
		uint16_t sppc_request(const char *wbuf, int wlen);
		int sppc_write_data(const uint8_t *wbuf, int wlen, boolean pgm);

	public:
		static uint16_t flags;
		static uint16_t pkg_ver_id;
		static uint16_t errno;
		uint16_t command(const __FlashStringHelper *format, ...);
		uint16_t command(const char *format, ...);
		int write(const __FlashStringHelper *wstr);
		int write(const char *wstr);
		int write(const uint8_t *wbuf, size_t wlen);
		int read(uint8_t *rbuf, size_t rlen);
		char *readString(void);
		void logFlush(uint8_t id);
		void logPrint(uint8_t id);
		int beginIP4();
		int beginIP6();
		int begin(uint16_t init_flags = 0x0000);

		friend class PhpocClass;
		friend class PhpocClient;
		friend class PhpocServer;
};

/* SppcVsprintf.cpp */
extern int sppc_vsprintf(char *str, const __FlashStringHelper *format, va_list args);
extern int sppc_vsprintf(char *str, const char *format, va_list args);
extern int sppc_sprintf(char *str, const __FlashStringHelper *format, ...);
extern int sppc_sprintf(char *str, const char *format, ...);

/* Sppc.cpp */
extern void sppc_printf(const __FlashStringHelper *format, ...);
extern SppcClass Sppc;

#endif
