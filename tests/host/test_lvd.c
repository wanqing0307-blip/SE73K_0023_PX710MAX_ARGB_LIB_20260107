/*******************************************************************************
* 文件名称: test_lvd.c
*
* 功能说明: 电量检测的回归测试。直接编译工程里的 lvd.c，POF 比较器用桩模拟，
*           测的是固件那一份 bat_check()。
*
*           重点：bat_power_delay 原来是累计计数，读数一次正常也不清零，
*           灯效负载造成的瞬时跌落会累积，电量只降不升。
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "om_driver.h"
#include "main.h"
#include "lvd.h"
#include "test_util.h"

//----------------------------------------------------------------------------
// 硬件桩
//
MOCK_POF_Type     mock_pof;
MOCK_RF_Type      mock_rf;
MOCK_GPIO_Type    mock_gpio0;
MOCK_PMU_Type     mock_pmu;
MOCK_ENCODER_Type mock_encoder;
MOCK_RTC_Type     mock_rtc;

static uint32_t sim_bat_mv = 4000;              // 模拟电池电压
static uint32_t sim_rtc;
static int      deep_sleep_calls;

uint32_t drv_rtc_read(MOCK_RTC_Type *rtc)   { (void)rtc; return sim_rtc; }
void drv_pmu_set_low_power_mode(pmu_lpm_t l) { (void)l; deep_sleep_calls++; }

// POF 比较器：门限在 POFCON[8:4]，A3 档位 0.900V 起、25mV 一档。
// 分压 2M/1M，所以脚上电压是电池的 1/3。
// 固件写完门限后会 delayus() 等建立，就在这里更新比较结果。
void delayus(unsigned int time)
{
    (void)time;
    if(mock_pof.POFCON & 0x08)                  // POF 使能
    {
        uint32_t idx    = (mock_pof.POFCON >> 4) & 0x1F;
        uint32_t thr_mv = 900 + 25 * idx;
        if((sim_bat_mv / 3) >= thr_mv)
            mock_pof.POFCON |= 0x01;            // pof_warn=1: 高于门限
        else
            mock_pof.POFCON &= ~0x01u;
    }
}

//----------------------------------------------------------------------------
// 被 lvd.c 引用、但定义在别处的符号
//
void logo_off(void) {}
void mouse_periph_wakeup_enable(uint8_t mask) { (void)mask; }

bool                bsave_status;
bool                bsave_cfg_dpi;
bool                bled_bright_en;
uint8_t             led_cur_mode;
uint8_t             led_light_lev;
uint8_t             hint_led_status;
uint8_t             kb_led_status;
volatile uint32_t   sys_tick_count;
uint32_t            rtc_cur_value;
uint32_t            rtc_init_value;
uint32_t            rtc_temp_value;

//----------------------------------------------------------------------------
// lvd.c 里有、但头文件没声明的
//
extern uint8_t  lvd_value;
extern uint16_t bat_power_delay;
extern bool     bcharging_bak;
extern void     bat_check(void);
extern uint8_t  bat_level_cal(uint8_t bat_level);

//----------------------------------------------------------------------------
// 辅助
//
#define iBAT_STEP_LIMIT     (5*64)              // lvd.c 里的门限

static void set_charging(bool on)
{
    if(on) mock_gpio0.DATA |=  (1u << (3*8+4)); // USB_5V = P34
    else   mock_gpio0.DATA &= ~(1u << (3*8+4));
}

// 档位 -> 刚好落在该档位的电池电压（分压 1/3，A3 档位 0.900V 起、25mV 一档）
static uint32_t mv_for_code(uint8_t code)
{
    return 3 * (900 + 25 * (code - 1)) + 30;
}

static void bat_reset(uint32_t mv, uint8_t lvd_start)
{
    sim_bat_mv  = mv;
    bat_status  = 0;                            // 清掉 blvd_power_on，直接走正常分支
    lvd_value   = lvd_start;
    bat_value   = 50;
    bat_power_delay = 0;
    bcharging_bak   = false;
    set_charging(false);
    mock_rf.LINE = iOM6239_A3_0;                // 走 A3 档位表
}

//----------------------------------------------------------------------------
// 1) 门限搜索：给定电压能读回对应档位
//
static void test_voltage_ladder(void)
{
    struct { uint32_t mv; uint8_t lvd_min, lvd_max; } tv[] = {
        { 4200, 0x14, 0x1B },                   // 满电
        { 3600, 0x0C, 0x10 },
        { 3200, 0x06, 0x09 },                   // 接近关机门限
    };

    for(unsigned k = 0; k < sizeof(tv)/sizeof(tv[0]); k++)
    {
        sim_bat_mv = tv[k].mv;
        uint8_t v = bat_voltage_check();
        CHECK_TRUE(v >= tv[k].lvd_min && v <= tv[k].lvd_max,
                   "%umV 读回档位 %u", tv[k].mv, v);
    }
}

//----------------------------------------------------------------------------
// 2) 持续偏低：累计到门限后降一档（正常放电）
//
static void test_sustained_drop_steps_down(void)
{
    bat_reset(mv_for_code(0x0E), 0x12);         // 实际电压比 lvd_value 低几档
    uint8_t start = lvd_value;

    for(int i = 0; i < iBAT_STEP_LIMIT + 2; i++)
        bat_check();

    CHECK_EQ_INT(start - 1, lvd_value, "持续偏低应降一档");
}

//----------------------------------------------------------------------------
// 3) 间歇性跌落：不应该降档（本次修复的点）
//    模拟灯效负载：一半时间电压被拉低，一半时间正常
//
static void test_intermittent_dip_does_not_step(void)
{
    bat_reset(mv_for_code(0x12), 0x12);
    uint8_t start = lvd_value;
    uint32_t dip_mv    = mv_for_code(0x0E);     // 灯效负载把电压拉低几档
    uint32_t normal_mv = mv_for_code(0x12);     // 空载时回到当前档位

    for(int i = 0; i < 4 * iBAT_STEP_LIMIT; i++)
    {
        sim_bat_mv = (i & 1) ? dip_mv : normal_mv;  // 隔次跌落
        bat_check();
    }

    CHECK_EQ_INT(start, lvd_value, "间歇跌落不应降档（累计计数的老问题）");
}

//----------------------------------------------------------------------------
// 4) 充放电切换：计数清零，不把充电时攒的数带到放电
//
static void test_charge_toggle_resets_counter(void)
{
    bat_reset(mv_for_code(0x12), 0x12);

    set_charging(true);                         // 充电中，读数偏高 -> 计数累加
    sim_bat_mv = mv_for_code(0x16);
    for(int i = 0; i < iBAT_STEP_LIMIT - 2; i++)
        bat_check();
    CHECK_TRUE(bat_power_delay > 0, "充电时计数已累加");

    set_charging(false);                        // 切回放电
    sim_bat_mv = mv_for_code(0x0E);
    bat_check();
    CHECK_TRUE(bat_power_delay <= 1, "切换后计数清零");
}

//----------------------------------------------------------------------------
// 5) 百分比映射的边界
//
static void test_level_mapping(void)
{
    mock_rf.LINE = iOM6239_A3_0;
    CHECK_EQ_INT(0,   bat_level_cal(iLVD_0_VALUE_A3),     "0%% 门限");
    CHECK_EQ_INT(100, bat_level_cal(iLVD_100_VALUE_A3),   "100%% 门限");
    CHECK_TRUE(bat_level_cal(iLVD_0_VALUE_A3 + 1) > 0,    "刚过 0%% 门限");
}

int main(void)
{
    test_voltage_ladder();
    test_sustained_drop_steps_down();
    test_intermittent_dip_does_not_step();
    test_charge_toggle_resets_counter();
    test_level_mapping();

    CHECK_EQ_INT(0, deep_sleep_calls, "测试过程中不应触发关机");

    return test_summary("lvd / battery");
}
