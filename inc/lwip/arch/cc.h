/* 
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science. 
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
 * This file is part of the lwIP TCP/IP stack. 
 *  
 * Author: Adam Dunkels <adam@sics.se> 
 * 
 */ 
#ifndef __CC_H__ 
#define __CC_H__ 

#include <stdint.h>

/**
 * Types based on stdint.h
 */
typedef uint8_t		u8_t; 
typedef int8_t		s8_t; 
typedef uint16_t	u16_t; 
typedef int16_t		s16_t; 
typedef uint32_t	u32_t; 
typedef int32_t		s32_t; 
typedef uintptr_t	mem_ptr_t; 

/**
 * Define (sn)printf formatters for these lwIP types
 */
#define U16_F "2u"
#define S16_F "2d"
#define X16_F "2x"
#define U32_F "4u"
#define S32_F "4d"
#define X32_F "4x"
#define SZT_F "uz"

/**
 * ARM/LPC17xx is little endian only
 */
#define BYTE_ORDER LITTLE_ENDIAN

/**
 * Use LWIP error codes
 */
#define LWIP_PROVIDE_ERRNO

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/**
 * Used with IP headers only
 */
#define LWIP_CHKSUM_ALGORITHM 1

#ifdef LWIP_DEBUG
/**
 * Platform specific diagnostic output
 */
#include "debug.h"
#define LWIP_PLATFORM_DIAG(msg) debug_printf msg
#define LWIP_PLATFORM_ASSERT(flag) { debug_printf("%s at %s:%d\n",	\
						  flag, __FILE__, __LINE__); while(1); }
#else
#define LWIP_PLATFORM_DIAG(msg) { ; }
#define LWIP_PLATFORM_ASSERT(flag) { while (1); }
#endif

#endif /* __CC_H__ */ 
