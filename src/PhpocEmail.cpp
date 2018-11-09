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

#include <Phpoc.h>

void PhpocEmail::setOutgoingServer(const char *host, uint16_t port)
{
	if(host && host[0])
		Phpoc.command(F("php smtp server %s %u"), host, port);
	else
		Phpoc.command(F("php smtp server"));
}

void PhpocEmail::setOutgoingServer(const __FlashStringHelper *host, uint16_t port)
{
	if(host && pgm_read_byte(host))
		Phpoc.command(F("php smtp server %S %u"), host, port);
	else
		Phpoc.command(F("php smtp server"));
}

void PhpocEmail::setOutgoingLogin(const char *username, const char *password)
{
	if(username && username[0] && password && password[0])
		Phpoc.command(F("php smtp login %s %s"), username, password);
}

void PhpocEmail::setOutgoingLogin(const __FlashStringHelper *username, const __FlashStringHelper *password)
{
	if(username && pgm_read_byte(username) && password && pgm_read_byte(password))
		Phpoc.command(F("php smtp login %S %S"), username, password);
}

void PhpocEmail::setFrom(const char *email, const char *name)
{
	if(!email || !email[0])
		return;

	if(!name || !name[0])
		name = email;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp from"));
		Phpoc.write(email);
		Phpoc.write(name);
	}
	else
#endif
	{
		Phpoc.write(name);
		Phpoc.command(F("php smtp from %s"), email);
	}
}

void PhpocEmail::setFrom(const __FlashStringHelper *email, const __FlashStringHelper *name)
{
	if(!email || !pgm_read_byte(email))
		return;

	if(!name || !pgm_read_byte(name))
		name = email;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp from"));
		Phpoc.write(email);
		Phpoc.write(name);
	}
	else
#endif
	{
		Phpoc.write(name);
		Phpoc.command(F("php smtp from %S"), email);
	}
}

void PhpocEmail::setTo(const char *email, const char *name)
{
	if(!email || !email[0])
		return;

	if(!name || !name[0])
		name = email;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp to"));
		Phpoc.write(email);
		Phpoc.write(name);
	}
	else
#endif
	{
		Phpoc.write(name);
		Phpoc.command(F("php smtp to %s"), email);
	}
}

void PhpocEmail::setTo(const __FlashStringHelper *email, const __FlashStringHelper *name)
{
	if(!email || !pgm_read_byte(email))
		return;

	if(!name || !pgm_read_byte(name))
		name = email;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp to"));
		Phpoc.write(email);
		Phpoc.write(name);
	}
	else
#endif
	{
		Phpoc.write(name);
		Phpoc.command(F("php smtp to %S"), email);
	}
}

void PhpocEmail::setSubject(const char *subject)
{
	if(!subject || !subject[0])
		return;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp subject"));
		Phpoc.write(subject);
	}
	else
#endif
	{
		Phpoc.write(subject);
		Phpoc.command(F("php smtp subject"));
	}
}

void PhpocEmail::setSubject(const __FlashStringHelper *subject)
{
	if(!subject || !pgm_read_byte(subject))
		return;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp subject"));
		Phpoc.write(subject);
	}
	else
#endif
	{
		Phpoc.write(subject);
		Phpoc.command(F("php smtp subject"));
	}
}

void PhpocEmail::beginMessage()
{
	Phpoc.command(F("php smtp data begin"));
	write_cache_len = 0;
}

void PhpocEmail::endMessage()
{
	if(!write_cache_len)
		return;

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		Phpoc.command(F("php smtp data"));
		Phpoc.write(write_cache_buf, write_cache_len);
	}
	else
#endif
	{
		Phpoc.write(write_cache_buf, write_cache_len);
		Phpoc.command(F("php smtp data"));
	}

	write_cache_len = 0;
}

size_t PhpocEmail::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t PhpocEmail::write(const uint8_t *wbuf, size_t wlen)
{
	if(write_cache_len + wlen >= EMAIL_WRITE_CACHE_SIZE)
	{
#ifdef INCLUDE_LIB_V1
		if(Sppc.flags & PF_SYNC_V1)
		{
			if(write_cache_len)
			{
				Phpoc.command(F("php smtp data"));
				Phpoc.write(write_cache_buf, write_cache_len);
				write_cache_len = 0;
			}

			if(wlen)
			{
				Phpoc.command(F("php smtp data"));
				Phpoc.write(wbuf, wlen);
			}
		}
		else
#endif
		{
			if(write_cache_len)
			{
				Phpoc.write(write_cache_buf, write_cache_len);
				Phpoc.command(F("php smtp data"));
				write_cache_len = 0;
			}

			if(wlen)
			{
				Phpoc.write(wbuf, wlen);
				Phpoc.command(F("php smtp data"));
			}
		}

		return wlen;
	}
	else
	{
		memcpy(write_cache_buf + write_cache_len, wbuf, wlen);
		write_cache_len += wlen;
		return wlen;
	}
}

uint8_t PhpocEmail::send()
{
	int len, status;

#ifdef PF_LOG_APP
	if(Sppc.flags & PF_LOG_APP)
		Phpoc.logFlush(1);
#endif

#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
		Phpoc.command(F("tcp0 ioctl close")); /* close tcp connection */
	else
#endif
		/* we must close tcp pid here to enable opening /mmap/tcp0 in esmtp library by task0. */
		Phpoc.command(F("tcp0 close")); /* close tcp pid */

	if(Sppc.flags & PF_IP6)
		Phpoc.command(F("php smtp send ip6"));
	else
		Phpoc.command(F("php smtp send"));

	while(1)
	{
#ifdef PF_LOG_APP
		if(Sppc.flags & PF_LOG_APP)
			Phpoc.logPrint(1);
#endif

#ifdef INCLUDE_LIB_V1
		if(Sppc.flags & PF_SYNC_V1)
		{
			len = Phpoc.command(F("php smtp status"));

			if(Sppc.errno)
				return 0;

			if(len)
			{
				status = Phpoc.readInt();
				break;
			}
		}
		else
#endif
		{
			status = Phpoc.command(F("php smtp status"));

			if(Sppc.errno)
				return 0;

			if(status)
				break;
		}

		delay(10);
	}

#ifdef PF_LOG_APP
	delay(100);
	if(Sppc.flags & PF_LOG_APP)
		Phpoc.logPrint(1);
#endif

	if((status >= 200) && (status <= 299))
		return 1; /* mail server respond 2xx */
	else
		return 0;
}

