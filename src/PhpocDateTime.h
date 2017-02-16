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

#ifndef PhpocDateTime_h
#define PhpocDateTime_h

#define DATE_BUF_SIZE 32

class PhpocDateTime
{
	private:
		char date_buf[DATE_BUF_SIZE];

	public:
		char *date(const char *format = NULL);
		char *date(const __FlashStringHelper *format);
		uint8_t hour();      // 0 ~ 23
		uint8_t minute();    // 0 ~ 59
		uint8_t second();    // 0 ~ 59
		/* day, week, month, year number is ISO-8601 compliant */
		uint8_t day();       // 1 ~ 31
		uint8_t dayofWeek(); // 1(Monday) ~ 7(Sunday)
		uint8_t month();     // 1(January) ~ 12
		uint16_t year();     // 2000 ~ 2099
};

#endif
