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

#include <Arduino.h>
#include <IP6Address.h>

char IP6Address::ip6str_buf[40];

IP6Address::IP6Address()
{
	memset(addr_hex16, 0x00, 16);
}

IP6Address::IP6Address(const char *addr)
{
	fromString(addr);
}

static uint16_t atox_u16(const char *nptr, char **endptr)
{
	uint16_t u16;

	u16 = 0;

	while(isxdigit(*nptr))
	{
		u16 <<= 4;
		if(isdigit(*nptr))
			u16 += (*nptr - '0');
		else
			u16 += (toupper(*nptr) - 'A' + 10);
		nptr++;
	}

	if(endptr)
		*endptr = (char *)nptr;

	return u16;
}

static bool is_ip6_char(int ch)
{
	if(ch == ':')
		return true;

	if(isxdigit(ch))
		return true;

	return false;
}

static int check_ip6_string(const char *str, int *comp_zeros)
{
	int colons, zero_groups;
	int left_addrs, right_addrs;
	const char *ptr;

	colons = 0;
	zero_groups = 0;
	left_addrs = 0;
	right_addrs = 0;

	ptr = str;

	while(is_ip6_char(*ptr))
	{
		if(*ptr == ':')
		{
			if(ptr[1] == ':')
			{
				if(ptr[2] == ':')
					return 0;

				zero_groups++; /* number of '::' - can appear once */
				if(zero_groups > 1)
					return 0;

				colons += 2;
				ptr++;
			}
			else
			{
				if((ptr == str) || !is_ip6_char(ptr[1]))
					return 0; /* ':2:3..7:8' and '1:2..7:8:' are not allowed */
				colons++;
			}

			if(colons >= 8)
			{
				if(colons == 8)
				{
					if(!zero_groups || (left_addrs && right_addrs))
						return 0;
					/* '::2:3:4:5:6:7:8' and '1:2:3:4:5:6:7::' are allowed */
				}
				else
					return 0;
			}
		}
		else
		{
			if(!isxdigit(*ptr))
				return 0;

			if(zero_groups)
			{
				if(ptr[-1] == ':')
					right_addrs++;
			}
			else
			{
				if((ptr[1] == ':') || !is_ip6_char(ptr[1]))
					left_addrs++;
			}
		}

		ptr++;
	}

	if(zero_groups)
	{
		if(colons < 2)
			return 0;
	}
	else
	{
		if(colons < 7)
			return 0;
	}

	*comp_zeros = 8 - left_addrs - right_addrs;

	return ptr - str;
}

bool IP6Address::fromString(const char *str)
{
	uint16_t *ptr16, hex16;
	int len, comp_zeros;
	IP6Address ip6addr;

	if(!(len = check_ip6_string(str, &comp_zeros)))
		return false;

	memset(addr_hex16, 0x00, 16);
	ptr16 = addr_hex16;

	while(is_ip6_char(*str))
	{
		if(*str == ':')
		{
			if(str[1] == ':')
			{
				ptr16 += comp_zeros; /* skip compressed zeros */
				str += 2;
				continue;
			}
			str++;
			continue;
		}

		hex16 = atox_u16(str, (char **)&str);
		*ptr16 = (hex16 >> 8) | (hex16 << 8);
		ptr16++;
	}

	return true;
}

char *IP6Address::toString(void)
{
	extern int phpoc_sprintf(char *str, const __FlashStringHelper *format, ...);
	const uint16_t *ptr16;
	int omit, len;
	char *str;

	ptr16 = addr_hex16;
	str = ip6str_buf;

	if(!memcmp(ptr16, "\x00\x00\x00\x00\x00\x00\x00\x00", 8))
	{
		const uint8_t *addr8;

		addr8 = (uint8_t *)addr_hex16;

		if(!ptr16[4] && (ptr16[5] == 0xffff))
		{
			str += phpoc_sprintf(str, F("::ffff:"));
			addr8 += 12;
			str += phpoc_sprintf(str, F("%u.%u.%u.%u"), addr8[0], addr8[1], addr8[2], addr8[3]);
			return ip6str_buf;
		}
	}

	len = 16;
	omit = 0;

	for( ; len; len -= 2)
	{
		if(len > 2)
		{
			if(!*ptr16)
			{
				if(!omit)
				{
					if(len == 16)
						str += phpoc_sprintf(str, F("::"));
					else
						str += phpoc_sprintf(str, F(":"));
					omit = 1;;
				}
				if(omit == 2)
					str += phpoc_sprintf(str, F("0:"));
			}
			else
			{
				if(omit == 1)
					omit = 2;
				str += phpoc_sprintf(str, F("%x:"), (*ptr16 >> 8) | (*ptr16 << 8));
			}
			ptr16++;
		}
		else
		{
			if(*ptr16)
				str += phpoc_sprintf(str, F("%x"), (*ptr16 >> 8) | (*ptr16 << 8));
			else
				str += phpoc_sprintf(str, F("0"));
			ptr16++;
		}
	}

	return ip6str_buf;
}

bool IP6Address::operator==(const IP6Address &ip6addr)
{
	if(!memcmp(addr_hex16, ip6addr.addr_hex16, 16))
		return true;
	else
		return false;
}

bool IP6Address::operator!=(const IP6Address &ip6addr)
{
	if(!memcmp(addr_hex16, ip6addr.addr_hex16, 16))
		return false;
	else
		return true;
}

static char *bypass_const_toString(IP6Address ip6addr)
{
	return ip6addr.toString();
}

size_t IP6Address::printTo(Print &p) const
{
	return p.print(bypass_const_toString(*this));
}

