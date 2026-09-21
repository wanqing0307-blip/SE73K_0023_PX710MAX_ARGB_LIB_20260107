/*******************************************************************************
* 文件名称: om_driver.h (host mock)
*
* 功能说明: 主机侧回归测试用的最小硬件桩。把工程源码直接编到 PC 上跑，
*           测的是固件里那一份代码本身，不是抄一份副本。
*           只实现被测文件用到的寄存器和接口，不追求和芯片一致。
*******************************************************************************/
#ifndef __OM_DRIVER_H_HOST_MOCK
#define __OM_DRIVER_H_HOST_MOCK

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define __RAM_CODE
#define __ALIGNED(x)
#define __WEAK
#define __NOP()

#define OM_CRITICAL_BEGIN()     do { } while(0)
#define OM_CRITICAL_END()       do { } while(0)

//----------------------------------------------------------------------------
// 寄存器
//
typedef struct { volatile uint32_t POFCON; volatile uint32_t POFMSK; } MOCK_POF_Type;
typedef struct { volatile uint32_t LINE; } MOCK_RF_Type;
typedef struct {
    volatile uint32_t DATA, DATAOUT, DATAOUTSET, DATAOUTCLR;
    volatile uint32_t OUTENSET, OUTENCLR;
    volatile uint32_t INTENSET, INTENCLR, INTSTATUS;
} MOCK_GPIO_Type;
typedef struct {
    volatile uint32_t PAD_PU_CTRL, PAD_PU_CTRL_1, PAD_PD_CTRL, PAD_BUF_EN;
    volatile uint32_t SW_RSVD1, SW_RSVD2, SW_RSVD7;
} MOCK_PMU_Type;
typedef struct { volatile uint32_t CNT, VALID, EN; } MOCK_ENCODER_Type;
typedef struct { volatile uint32_t CNT, CNT_VALID, CR, ALARM0; } MOCK_RTC_Type;

extern MOCK_POF_Type     mock_pof;
extern MOCK_RF_Type      mock_rf;
extern MOCK_GPIO_Type    mock_gpio0;
extern MOCK_PMU_Type     mock_pmu;
extern MOCK_ENCODER_Type mock_encoder;
extern MOCK_RTC_Type     mock_rtc;

#define OM_POF      (&mock_pof)
#define OM_RF       (&mock_rf)
#define OM_GPIO0    (&mock_gpio0)
#define OM_PMU      (&mock_pmu)
#define OM_ENCODER  (&mock_encoder)
#define OM_RTC      (&mock_rtc)

#define PMU_SW_RSVD1_POWER_OFF          (1u<<16)
#define ENCODER_VALID_CNT_MASK          (1u<<0)

typedef enum {
    PMU_LPM_ACTIVE = 0,
    PMU_LPM_IDLE,
    PMU_LPM_STOP1,
    PMU_LPM_DEEP_SLEEP,
} pmu_lpm_t;

//----------------------------------------------------------------------------
// 驱动接口
//
extern uint32_t drv_rtc_read(MOCK_RTC_Type *rtc);
extern void     drv_pmu_set_low_power_mode(pmu_lpm_t lpm);
extern void     delayus(unsigned int time);

#endif
