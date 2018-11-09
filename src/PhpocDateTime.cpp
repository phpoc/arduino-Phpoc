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

int PhpocDateTime::rtc_date(void)
{
	int len;

	if(Phpoc.command(F("rtc0 ioctl get date")) > 0)
	{
		if((len = Phpoc.read((uint8_t *)date_buf, DATE_BUF_SIZE - 1)) > 0)
		{
			date_buf[len] = 0x00;
			return len;
		}
	}

	return 0;
}

char *PhpocDateTime::date(const char *format)
{
	int len;

	if(format && format[0])
	{
#ifdef INCLUDE_LIB_V1
		if(Sppc.flags & PF_SYNC_V1)
		{
			if(Phpoc.command(F("sys date format")) >= 0)
				Phpoc.write(format);
		}
		else
#endif
		{
			Phpoc.write(format);
			Phpoc.command(F("sys date format"));
		}
	}

	if(Phpoc.command(F("sys date")) > 0)
	{
		if((len = Phpoc.read((uint8_t *)date_buf, DATE_BUF_SIZE - 1)) > 0)
			date_buf[len] = 0x00;
		else
			date_buf[0] = 0x00;
	}
	else
		date_buf[0] = 0x00;

	return date_buf;
}

char *PhpocDateTime::date(const __FlashStringHelper *format)
{
	if(format && pgm_read_byte(format))
	{
#ifdef INCLUDE_LIB_V1
		if(Sppc.flags & PF_SYNC_V1)
		{
			if(Phpoc.command(F("sys date format")) >= 0)
				Phpoc.write(format);
		}
		else
#endif
		{
			Phpoc.write(format);
			Phpoc.command(F("sys date format"));
		}
	}

	return date();
}

uint8_t PhpocDateTime::hour()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get hour")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[10] = 0x00;
		return atoi(date_buf + 8);
	}
}

uint8_t PhpocDateTime::minute()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get minute")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[12] = 0x00;
		return atoi(date_buf + 10);
	}
}

uint8_t PhpocDateTime::second()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get second")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[14] = 0x00;
		return atoi(date_buf + 12);
	}
}

uint8_t PhpocDateTime::day()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get day")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[8] = 0x00;
		return atoi(date_buf + 6);
	}
}

uint8_t PhpocDateTime::dayofWeek()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get wday")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		int wday;

		wday = Phpoc.command(F("rtc0 ioctl get wday"));

		if(wday < 0)
			return 0;

		if(wday == 0)
			wday = 7;

		return wday;
	}
}

uint8_t PhpocDateTime::month()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get month")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[6] = 0x00;
		return atoi(date_buf + 4);
	}
}

uint16_t PhpocDateTime::year()
{
#ifdef INCLUDE_LIB_V1
	if(Sppc.flags & PF_SYNC_V1)
	{
		if(Phpoc.command(F("sys rtc get year")) > 0)
			return Phpoc.readInt();
		else
			return 0;
	}
	else
#endif
	{
		if(rtc_date() < 14)
			return 0;

		date_buf[4] = 0x00;
		return atoi(date_buf + 0);
	}
}

