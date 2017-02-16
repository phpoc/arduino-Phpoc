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
 *
 * simplified & PROGMEM enabled vsprintf
 * - %s : data memory string
 * - %S : program memory string
 */

#include <Phpoc.h>

#define STATE_NC 0 /* No Conversion */
#define STATE_SC 1 /* Start Conversion */
#define STATE_c  2
#define STATE_d  3
#define STATE_s  4
#define STATE_u  5
#define STATE_x  6

#define FLAG_UPPER      0x01 /* upper case hexa-decimal, E, F, G */
#define FLAG_LEFT       0x02 /* left justified (-) */
#define FLAG_SIGN       0x04 /* sign specifier (+) */
#define FLAG_SIGN_MINUS 0x08 /* minus sign */
#define FLAG_LONG       0x10 /* long type modifier (32bit) */
#define FLAG_LONG_LONG  0x20 /* long long type modifier (64bit) */
#define FLAG_PGM_STR    0x40 /* program memory string */

static char *vsp_ptr;
static uint8_t vsp_count;

static void vsp_out_char(char ch)
{
	if(vsp_ptr)
	{
		if((vsp_count + 1) < VSP_COUNT_LIMIT)
		{
			*vsp_ptr++ = ch;
			vsp_count++;
		}
	}
	/*
	else
	{
		vsp_write(&ch, 1);
		vsp_count++;
	}
	*/
}

static void vsp_out_u16(uint16_t u16, uint8_t base, uint8_t flags)
{
	char digit_buf[7]; /* 1(sign) + 5(digit) + 1(0x00) */
	int digit, div;
	char *ptr;

	memset(digit_buf, '0', 6);
	digit_buf[6] = 0x00;
	ptr = digit_buf + 5;

	for(digit = 4; digit >=0; digit--, ptr--)
	{
		if(u16)
		{
			if(digit < 4)
				u16 /= base;
			div = u16 % base;

			if(div < 10)
				*ptr = '0' + div;
			else
			{
				if(flags & FLAG_UPPER)
					*ptr = ('A' - 10) + div;
				else
					*ptr = ('a' - 10) + div;
			}
		}
		else
			break;
	}

	ptr = digit_buf;
	while(*ptr == '0')
		ptr++;

	if(!*ptr)
		ptr--;
	else
	{
		if(flags & FLAG_SIGN_MINUS)
		{
			ptr--;
			*ptr = '-';
		}
	}

	digit = strlen(ptr);

	if(vsp_ptr)
	{
		if((vsp_count + digit) < VSP_COUNT_LIMIT)
		{
			strcpy(vsp_ptr, ptr);
			vsp_ptr += digit;
			vsp_count += digit;
		}
	}
	/*
	else
	{
		vsp_write(ptr, digit);
		vsp_count += digit;
	}
	*/
}

static void vsp_out_str(char *str, uint8_t flags)
{
	char ch;

	if(str == NULL)
		return;

	while(1)
	{
		if(flags & FLAG_PGM_STR)
			ch = pgm_read_byte(str++);
		else
			ch = *str++;
		if(!ch)
			return;

		if(vsp_ptr)
		{
			if((vsp_count + 1) >= VSP_COUNT_LIMIT)
				return;
			*vsp_ptr++ = ch;
			vsp_count++;
		}
		/*
		else
		{
			vsp_write(&ch, 1);
			vsp_count++;
		}
		*/
	}
}

static int __vsprintf(const char *format, va_list args, boolean pgm)
{
	uint8_t state, flags;
	uint16_t arg_u16;
	char ch;

	state = STATE_NC;
	flags = 0x00;

	while(1)
	{
		if(pgm)
			ch = pgm_read_byte(format++);
		else
			ch = *format++;

		if(!ch)
			break;

		if(state == STATE_NC)
		{
			if(ch == '%')
				state = STATE_SC;
			else
				vsp_out_char(ch);
			continue;
		}

		if(state == STATE_SC)
		{
			switch(ch)
			{
				case '%':
					vsp_out_char(ch);
					state = STATE_NC;
					continue;
				case 'c':
					state = STATE_c;
					break;
				case 'd':
					state = STATE_d;
					break;
				case 'S':
					flags |= FLAG_PGM_STR;
					state = STATE_s;
					break;
				case 's':
					state = STATE_s;
					break;
				case 'u':
					state = STATE_u;
					break;
				case 'X':
					flags |= FLAG_UPPER;
				case 'x':
					state = STATE_x;
					break;
				default:
					vsp_out_char(ch);
					goto _end_of_conv;
			}
		}

		switch(state)
		{
			case STATE_c:
				vsp_out_char(va_arg(args, int));
				break;
			case STATE_d:
				arg_u16 = va_arg(args, uint16_t);
				if(arg_u16 & 0x8000)
				{
					arg_u16 = ~arg_u16 + 1;
					flags |= FLAG_SIGN_MINUS;
				}
				vsp_out_u16(arg_u16, 10, flags);
				break;
			case STATE_s:
				arg_u16 = (uint16_t)va_arg(args, char *);
				vsp_out_str((char *)arg_u16, flags);
				break;
			case STATE_u:
				arg_u16 = va_arg(args, uint16_t);
				vsp_out_u16(arg_u16, 10, flags);
				break;
			case STATE_x:
				arg_u16 = va_arg(args, uint16_t);
				vsp_out_u16(arg_u16, 16, flags);
				break;
		}

_end_of_conv:
		state = STATE_NC;
		flags = 0x00;
	}

	if(vsp_ptr)
		*vsp_ptr = 0x00;

	return vsp_count;
}

int PhpocClass::vsprintf(char *str, const __FlashStringHelper *format, va_list args)
{
	vsp_ptr = str;
	vsp_count = 0;
	return ::__vsprintf((const char *)format, args, true);
}

int PhpocClass::vsprintf(char *str, const char *format, va_list args)
{
	vsp_ptr = str;
	vsp_count = 0;
	return ::__vsprintf(format, args, false);
}

int PhpocClass::sprintf(char *str, const __FlashStringHelper *format, ...)
{
	va_list args;
	int len;

	vsp_ptr = str;
	vsp_count = 0;

	va_start(args, format);
	len = ::__vsprintf((const char *)format, args, true);
	va_end(args);

	return len;
}

int PhpocClass::sprintf(char *str, const char *format, ...)
{
	va_list args;
	int len;

	vsp_ptr = str;
	vsp_count = 0;

	va_start(args, format);
	len = ::__vsprintf(format, args, false);
	va_end(args);

	return len;
}

int phpoc_sprintf(char *str, const __FlashStringHelper *format, ...)
{
	va_list args;
	int len;

	vsp_ptr = str;
	vsp_count = 0;

	va_start(args, format);
	len = ::__vsprintf((const char *)format, args, true);
	va_end(args);

	return len;
}

/*
int PhpocClass::printf(const __FlashStringHelper *format, ...)
{
	va_list args;
	int len;

	vsp_ptr = NULL;

	va_start(args, format);
	len = ::__vsprintf((const char *)format, args, true);
	va_end(args);

	return len;
}

int PhpocClass::printf(const char *format, ...)
{
	va_list args;
	int len;

	vsp_ptr = NULL;

	va_start(args, format);
	len = ::__vsprintf(format, args, false);
	va_end(args);

	return len;
}
*/
