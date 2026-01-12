/**
 * \file platform.h
 *
 * \brief mbed TLS Platform abstraction layer
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#ifndef MBEDTLS_PLATFORM_H
#define MBEDTLS_PLATFORM_H

#include "ke_mem.h"
#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"

#else
#include MBEDTLS_CONFIG_FILE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in config.h or define them on the compiler command line.
 * \{
 */
#define ke_calloc(num, size)   ke_malloc(((num) * (size)), KE_MEM_KE_MSG) 

#define mbedtls_free       ke_free
#define mbedtls_calloc     ke_calloc
#define mbedtls_printf     uart2_printf

/*
 * The function pointers for snprintf
 *
 * The snprintf implementation should conform to C99:
 * - it *must* always correctly zero-terminate the buffer
 *   (except when n == 0, then it must leave the buffer untouched)
 * - however it is acceptable to return -1 instead of the required length when
 *   the destination buffer is too short.
 */


/*
 * The function pointers for exit
 */

#ifdef __cplusplus
}
#endif

#endif /* platform.h */
