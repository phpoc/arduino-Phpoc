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
		Phpoc.command(F("smtp server %s %u"), host, port);
	else
		Phpoc.command(F("smtp server"));
}

void PhpocEmail::setOutgoingServer(const __FlashStringHelper *host, uint16_t port)
{
	if(host && pgm_read_byte(host))
		Phpoc.command(F("smtp server %S %u"), host, port);
	else
		Phpoc.command(F("smtp server"));
}

void PhpocEmail::setOutgoingLogin(const char *username, const char *password)
{
	if(username && username[0] && password && password[0])
		Phpoc.command(F("smtp login %s %s"), username, password);
}

void PhpocEmail::setOutgoingLogin(const __FlashStringHelper *username, const __FlashStringHelper *password)
{
	if(username && pgm_read_byte(username) && password && pgm_read_byte(password))
		Phpoc.command(F("smtp login %S %S"), username, password);
}

void PhpocEmail::setFrom(const char *email, const char *name)
{
	if(Phpoc.command(F("smtp from")) >= 0)
	{
		if(email && email[0])
			Phpoc.write(email);
		else
			return;

		if(name && name[0])
			Phpoc.write(name);
		else
			Phpoc.write(email);
	}
}

void PhpocEmail::setFrom(const __FlashStringHelper *email, const __FlashStringHelper *name)
{
	if(Phpoc.command(F("smtp from")) >= 0)
	{
		if(email && pgm_read_byte(email))
			Phpoc.write(email);
		else
			return;

		if(name && pgm_read_byte(name))
			Phpoc.write(name);
		else
			Phpoc.write(email);
	}
}

void PhpocEmail::setTo(const char *email, const char *name)
{
	if(Phpoc.command(F("smtp to")) >= 0)
	{
		if(email && email[0])
			Phpoc.write(email);
		else
			return;

		if(name && name[0])
			Phpoc.write(name);
		else
			Phpoc.write(email);
	}
}

void PhpocEmail::setTo(const __FlashStringHelper *email, const __FlashStringHelper *name)
{
	if(Phpoc.command(F("smtp to")) >= 0)
	{
		if(email && pgm_read_byte(email))
			Phpoc.write(email);
		else
			return;

		if(name && pgm_read_byte(name))
			Phpoc.write(name);
		else
			Phpoc.write(email);
	}
}

void PhpocEmail::setSubject(const char *subject)
{
	if(subject && subject[0])
	{
		if(Phpoc.command(F("smtp subject")) >= 0)
			Phpoc.write(subject);
	}
}

void PhpocEmail::setSubject(const __FlashStringHelper *subject)
{
	if(subject && pgm_read_byte(subject))
	{
		if(Phpoc.command(F("smtp subject")) >= 0)
			Phpoc.write(subject);
	}
}

void PhpocEmail::beginMessage()
{
	Phpoc.command(F("smtp data begin"));
	write_cache_len = 0;
}

void PhpocEmail::endMessage()
{
	if(write_cache_len)
	{
		if(Phpoc.command(F("smtp data")) >= 0)
			Phpoc.write(write_cache_buf, write_cache_len);
		write_cache_len = 0;
	}
}

size_t PhpocEmail::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t PhpocEmail::write(const uint8_t *buf, size_t size)
{
	if(write_cache_len + size >= WRITE_CACHE_SIZE)
	{
		if(write_cache_len)
		{
			if(Phpoc.command(F("smtp data")) >= 0)
				Phpoc.write(write_cache_buf, write_cache_len);
			write_cache_len = 0;
		}

		if(size)
		{
			if(Phpoc.command(F("smtp data")) >= 0)
				Phpoc.write(buf, size);
		}

		return size;
	}
	else
	{
		memcpy(write_cache_buf + write_cache_len, buf, size);
		write_cache_len += size;
		return size;
	}
}

uint8_t PhpocEmail::send()
{
	int len, status;

#ifdef PF_LOG_APP
	if(Phpoc.flags & PF_LOG_APP)
		Phpoc.logFlush(1);
#endif

	if(Phpoc.flags & PF_IP6)
	{
		if(Phpoc.command(F("smtp send ip6")) < 0)
			return 0;
	}
	else
	{
		if(Phpoc.command(F("smtp send")) < 0)
			return 0;
	}

	while(1)
	{
#ifdef PF_LOG_APP
		if(Phpoc.flags & PF_LOG_APP)
			Phpoc.logPrint(1);
#endif

		if((len = Phpoc.command(F("smtp status"))) < 0)
			return 0;

		if(len)
		{
			status = Phpoc.readInt();
			break;
		}

		delay(10);
	}

#ifdef PF_LOG_APP
	delay(100);
	if(Phpoc.flags & PF_LOG_APP)
		Phpoc.logPrint(1);
#endif

	if((status / 100) == 2)
		return 1; /* mail server respond 2xx */
	else
		return 0;
}

