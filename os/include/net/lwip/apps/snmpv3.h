/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/**
 * @file
 * Additional SNMPv3 functionality RFC3414 and RFC3826.
 */

/*
 * Copyright (c) 2016 Elias Oenal.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Elias Oenal <lwip@eliasoenal.com>
 */

#ifndef LWIP_HDR_APPS_SNMP_V3_H
#define LWIP_HDR_APPS_SNMP_V3_H

#include "lwip/apps/snmp_opts.h"
#include "lwip/err.h"

#if LWIP_SNMP && LWIP_SNMP_V3

#define SNMP_V3_AUTH_ALGO_INVAL  0
#define SNMP_V3_AUTH_ALGO_MD5    1
#define SNMP_V3_AUTH_ALGO_SHA    2

#define SNMP_V3_PRIV_ALGO_INVAL  0
#define SNMP_V3_PRIV_ALGO_DES    1
#define SNMP_V3_PRIV_ALGO_AES    2

#define SNMP_V3_PRIV_MODE_DECRYPT  0
#define SNMP_V3_PRIV_MODE_ENCRYPT  1

/*
 * The following callback functions must be implemented by the application.
 * There is a dummy implementation in snmpv3_dummy.c.
 */

void snmpv3_get_engine_id(const char **id, u8_t *len);
err_t snmpv3_set_engine_id(const char *id, u8_t len);

u32_t snmpv3_get_engine_boots(void);
void snmpv3_set_engine_boots(u32_t boots);

u32_t snmpv3_get_engine_time(void);
void snmpv3_reset_engine_time(void);

err_t snmpv3_get_user(const char *username, u8_t *auth_algo, u8_t *auth_key, u8_t *priv_algo, u8_t *priv_key);

/* The following functions are provided by the SNMPv3 agent */

void snmpv3_engine_id_changed(void);

void snmpv3_password_to_key_md5(const u8_t *password,	/* IN */
				u8_t passwordlen,	/* IN */
				const u8_t *engineID,	/* IN  - pointer to snmpEngineID  */
				u8_t engineLength,	/* IN  - length of snmpEngineID */
				u8_t *key);	/* OUT - pointer to caller 16-octet buffer */

void snmpv3_password_to_key_sha(const u8_t *password,	/* IN */
				u8_t passwordlen,	/* IN */
				const u8_t *engineID,	/* IN  - pointer to snmpEngineID  */
				u8_t engineLength,	/* IN  - length of snmpEngineID */
				u8_t *key);	/* OUT - pointer to caller 20-octet buffer */

#endif

#endif							/* LWIP_HDR_APPS_SNMP_V3_H */