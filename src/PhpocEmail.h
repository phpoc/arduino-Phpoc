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

#ifndef PhpocEmail_h
#define PhpocEmail_h

#ifndef WRITE_CACHE_SIZE
#define WRITE_CACHE_SIZE 32
#endif

class PhpocEmail : public Print
{
	private:
		uint8_t write_cache_len;
		uint8_t write_cache_buf[WRITE_CACHE_SIZE];

	public:
		void setOutgoingServer(const char *host, uint16_t port = 587);
		void setOutgoingServer(const __FlashStringHelper *host, uint16_t port = 587);
		void setOutgoingLogin(const char *username, const char *password);
		void setOutgoingLogin(const __FlashStringHelper *username, const __FlashStringHelper *password);
		void setFrom(const char *email, const char *name = NULL);
		void setFrom(const __FlashStringHelper *email, const __FlashStringHelper *name = NULL);
		void setTo(const char *email, const char *name = NULL);
		void setTo(const __FlashStringHelper *email, const __FlashStringHelper *name = NULL);
		void setSubject(const char *subject);
		void setSubject(const __FlashStringHelper *subject);
		void beginMessage();
		void endMessage();
		virtual size_t write(uint8_t byte);
		virtual size_t write(const uint8_t *buf, size_t size);
		uint8_t send();
		using Print::write;
};

#endif
