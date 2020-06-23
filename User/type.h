////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2011, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:types.h
//		Description	:re-define language C basic data type for porting conveniently
//						over platforms
//		Changelog	:
//					2012-02-21	Create basic data type re-definition for uniform
//						platform by Robert
///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup MVUtils
 * @{
 * @defgroup types types
 * @{
 */


#ifndef		__TYPES_H__
#define		__TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#ifndef NULL	
#define	NULL			((void*)0)
#endif

#define	FALSE			(0)
#define	TRUE			(1)

typedef	void(*FPCALLBACK)(void);

typedef unsigned char   		bool;
typedef bool(*TerminateFunc)(void);

typedef unsigned char	      	u8;
typedef unsigned int          	u32;
typedef unsigned short			u16;
typedef unsigned char   		bool;
#ifndef int8_t
//#if !__int8_t_defined
typedef signed char				int8_t;
#endif

#ifndef uint8_t
//#if !__int8_t_defined
typedef unsigned char			uint8_t;
#endif

#ifndef int16_t
//#if !__int16_t_defined
typedef signed short			int16_t;
#endif

#ifndef uint16_t
//#if !__int16_t_defined
typedef unsigned short			uint16_t;
#endif

#ifndef int32_t
//#if !__int32_t_defined
typedef signed int				int32_t;
#endif

#ifndef uint32_t
//#if !__int32_t_defined
typedef unsigned int			uint32_t;
#endif

typedef signed char				int8;
typedef unsigned char			uint8;
typedef signed short			int16;
typedef unsigned short			uint16;
typedef signed int				int32;
typedef unsigned int			uint32;



#ifdef __cplusplus
}
#endif//__cplusplus

#ifndef   __WEAK
  #define __WEAK                                 __attribute__((weak))
#endif

#ifndef   __NO_DEBUG_MODE__
	
	#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
	#define DBG_OPTM(fmt,...)		printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
	#define CHECK_RET_RETURN(tip, check_value, ret_code)	if(check_value!= 0){ DBG_OPTM("%s ERR %d\r\n",tip,check_value);return ret_code;}

#else
	
	#define	DBG(format, ...)		
	#define DBG_OPTM(fmt,...)		
	#define CHECK_RET_RETURN(tip, check_value, ret_code)	if(check_value!= 0){ DBG_OPTM("%s ERR %d\r\n",tip,check_value);return ret_code;}

#endif

#endif	//__TYPE_H__



