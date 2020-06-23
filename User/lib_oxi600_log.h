#ifndef __LIB_OXI600_LOG_H__
#define __LIB_OXI600_LOG_H__

/*
*	DBG api define , differnt platform has differnt implemented
*/
#if 1//(OXI_CONFIG_PLATFORM == PLATFORM_QSEE && OXI_CONFIG_BUILD == BUILD_TEE)
#if 1
#include <string.h>
#include <stdio.h>                        
#include "qsee_log.h"    
#define DBG(fmt,...)         qsee_printf("INFO: [%s] " fmt, __FUNCTION__,  ##__VA_ARGS__)   
#define DBG_OPTM(fmt,...)		qsee_printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#elif LIBUSB_OPEN_FD
#include <android/log.h>

#define LOG_TAG                        "_OXIFP_LISTDEV_"
#define DBG(...)            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define INFO(...)            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif
#else 	//(stm32 pc)

#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
#define DBG_OPTM(fmt,...)		printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif


#endif