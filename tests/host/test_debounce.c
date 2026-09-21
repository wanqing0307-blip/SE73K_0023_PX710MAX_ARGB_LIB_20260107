/*******************************************************************************
* 文件名称: test_debounce.c
*
* 功能说明: 矩阵去抖的回归测试。用的是 matrix_key.h 里的 key_debounce_step()，
*           和 matrix_debounce() 调的是同一份实现。
*           扫描周期 4ms，所以"连续两次一致"滤掉的是 4ms 以内的抖动。
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include "matrix_key.h"
#include "test_util.h"

//----------------------------------------------------------------------------
// 喂一串采样，返回每次的稳定输出
//
static void feed(const uint8_t *raw, uint8_t *out, int n)
{
    uint8_t raw_bak = 0, stable = 0;

    for(int i = 0; i < n; i++)
    {
        stable = key_debounce_step(raw[i], &raw_bak, stable);
        out[i] = stable;
    }
}

//----------------------------------------------------------------------------
// 干净按下：第二次采样才认，延迟一个扫描周期
//
static void test_clean_press(void)
{
    const uint8_t raw[] = { 0x00, 0x01, 0x01, 0x01, 0x00, 0x00 };
    uint8_t out[6];

    feed(raw, out, 6);
    CHECK_EQ_INT(0x00, out[0], "空闲");
    CHECK_EQ_INT(0x00, out[1], "按下第一帧还不认");
    CHECK_EQ_INT(0x01, out[2], "第二帧认下去");
    CHECK_EQ_INT(0x01, out[3], "保持");
    CHECK_EQ_INT(0x01, out[4], "松开第一帧还不认");
    CHECK_EQ_INT(0x00, out[5], "第二帧认松开");
}

//----------------------------------------------------------------------------
// 单帧毛刺：滤掉
//
static void test_single_frame_glitch(void)
{
    const uint8_t raw[] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
    uint8_t out[6];

    feed(raw, out, 6);
    for(int i = 0; i < 6; i++)
        CHECK_EQ_INT(0x00, out[i], "毛刺第 %d 帧", i);
}

//----------------------------------------------------------------------------
// 按下过程中抖动：抖完才认，且只认一次（不会出双击）
//
static void test_bouncing_press(void)
{
    const uint8_t raw[] = { 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01 };
    uint8_t out[7];
    int edges = 0;

    feed(raw, out, 7);
    for(int i = 1; i < 7; i++)
        if(out[i] != out[i-1])
            edges++;

    CHECK_EQ_INT(1, edges, "抖动只产生一次状态变化");
    CHECK_EQ_INT(0x01, out[6], "最终按下");
}

//----------------------------------------------------------------------------
// 逐位独立：一个键在抖，不应该耽误同一列上另一个键
//
static void test_per_bit(void)
{
    const uint8_t raw[] = { 0x00, 0x03, 0x01, 0x03, 0x03 };   // bit0 稳，bit1 在抖
    uint8_t out[5];

    feed(raw, out, 5);
    CHECK_EQ_INT(0x01, out[2], "稳定的 bit0 照常认下去");
    CHECK_EQ_INT(0x01, out[3], "抖动的 bit1 还没认");
    CHECK_EQ_INT(0x03, out[4], "bit1 稳定后才认");
}

//----------------------------------------------------------------------------
// 睡醒后第一帧扩展列错位：单帧异常，会被滤掉
// （ctrl_54e_inpull() 已修，这里是第二道保险）
//
static void test_wake_first_frame_garbage(void)
{
    const uint8_t raw[] = { 0x00, 0x40, 0x02, 0x02, 0x02 };   // 第一帧错位到别的位
    uint8_t out[5];

    feed(raw, out, 5);
    CHECK_EQ_INT(0x00, out[1], "错位帧不生效");
    CHECK_EQ_INT(0x00, out[2], "真实键第一帧还不认");
    CHECK_EQ_INT(0x02, out[3], "真实键第二帧认下去");
    CHECK_TRUE((out[0] | out[1] | out[2] | out[3] | out[4]) == 0x02, "错位的位从未出现");
}

int main(void)
{
    test_clean_press();
    test_single_frame_glitch();
    test_bouncing_press();
    test_per_bit();
    test_wake_first_frame_garbage();

    return test_summary("matrix debounce");
}
