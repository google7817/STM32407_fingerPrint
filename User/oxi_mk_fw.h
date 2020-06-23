#ifndef __OXI_MK_FW_H
#include "oxi_type.h"
#include "oxi_error.h"
#include "lib_oxi600.h"
#define LIBUSB_OPEN_FD  1

#if (OXI_CONFIG_PLATFORM == PLATFORM_QSEE && OXI_CONFIG_BUILD == BUILD_TEE)

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

#else

#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
#define DBG_OPTM(fmt,...)		printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif
/*typedef enum sensorType 
{
	SENSORMK110	 = 0,
	SENSORMK210	,
	SENSORMK310,
	SENSORMK410, 
	SENSORMK510,
	SENSORMK610,
	SENSORMK710S, 
	SENSORMK710L,
	//SENSORMK720S,	//	delete
	//SENSORMK720L,	//	delete
	SENSORMK720_80,	//   add
	SENSORMK720_100,//	add
	SENSORMK810_80,	//   modify
	//SENSORMK720_80,		delete
	SENSORMK320_100,//	modify
	SENSORMK810_80_04,	//MK810 has two type , MK810_80 and MK810_80_04,it's different
	SENSORCMOS,
} sensorType_t;*/

oxi_error_t oxi_fw_drv_init(void *phDevice);
oxi_error_t oxi_fw_sleep_roic(void *phDevice, void* CptPara,uint32_t timeout);
oxi_error_t oxi_fw_set_pmu_para(void *phDevice, oxiBlobData_t *pmuParaBlob);
oxi_error_t oxi_fw_clear_frame(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob, uint32_t timeout);
oxi_error_t oxi_fw_set_capture_para(void *phDevice, oxiBlobData_t *cptParaBlob);
oxi_error_t oxi_fw_read_flash(void *phDevice, oxiUint8_t partId ,oxiBlobData_t *imgBlob);
oxi_error_t oxi_fw_write_flash(void *phDevice, oxiUint8_t partId ,oxiBlobData_t *imgBlob);
oxi_error_t oxi_fw_set_power_mode(void *phDevice, oxiUint8_t pwrMode);
oxi_error_t oxi_fw_soft_reset(void *phDevice);
oxi_error_t oxi_fw_capture_fast_image(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob,oxiUint32_t timeout);
oxi_error_t oxi_fw_capture_full_partial_image(void *phDevice, void* CptPara,oxiUint32_t timeout);
oxi_error_t oxi_fw_capture_full_image(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob ,oxiUint32_t timeout);
oxi_error_t oxi_fw_capture_full_image_ShtAndLong(void *phDevice, void* CptPara, oxiBlobData_t *shtImgBlob ,oxiBlobData_t *longImgBlob,oxiUint32_t timeout);
oxi_error_t  oxi_fw_get_image_data(void *phDevice, uint16_t int_type, oxiBlobData_t *imgBlob,void* CptPara,uint32_t timeout);
oxi_error_t oxi_fw_get_lib_ver(void *phDevice);
oxi_error_t oxi_fw_deinit(void *phDevice);
oxi_error_t oxi_fw_flash_read_id(void *phDevice,uint16_t *flashId);
oxi_error_t oxi_fw_flash_read_sn(void *phDevice, char *pBuffer,uint32_t readAddr,uint16_t numByteToRead);
#endif
