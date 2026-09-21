/*******************************************************************************
* 文件名称: test_wheel.c
*
* 功能说明: 编码器计数转换的回归测试。
*           手册 19.1：ENCODER_CNT 是 4 位有符号数，且"读一次硬件就清零"。
*           原来中断里读了两次，正转丢步、反转读成 -16。
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include "wheel.h"
#include "test_util.h"

//----------------------------------------------------------------------------
// 模拟"读一次就清零"的 CNT 寄存器
//
static uint32_t enc_cnt;

static uint32_t cnt_read(void)
{
    uint32_t v = enc_cnt;
    enc_cnt = 0;                                // 手册：读操作清除 wheel_data
    return v;
}

//----------------------------------------------------------------------------
// 转换本身：4 位有符号数
//
static void test_cnt_to_data(void)
{
    static const int8_t expect[16] = { 0, 1, 2, 3, 4, 5, 6, 7,
                                      -8, -7, -6, -5, -4, -3, -2, -1 };
    for(uint32_t c = 0; c < 16; c++)
        CHECK_EQ_INT(expect[c], (int8_t)wheel_cnt_to_data(c), "CNT=%u", c);
}

//----------------------------------------------------------------------------
// 新写法：只读一次
//
static void test_single_read(void)
{
    enc_cnt = 1;                                // 正转一格
    CHECK_EQ_INT(1, (int8_t)wheel_cnt_to_data(cnt_read()), "正转一格");

    enc_cnt = 0x0F;                             // 反转一格
    CHECK_EQ_INT(-1, (int8_t)wheel_cnt_to_data(cnt_read()), "反转一格");
}

//----------------------------------------------------------------------------
// 旧写法：读两次。留着是为了说明这个 bug 会怎么表现
//
static uint8_t old_double_read(void)
{
    uint8_t wheel_data;

    if(cnt_read() & 0x08)                       // 第一次读，已把 CNT 清零
        wheel_data = 0xf0 | cnt_read();         // 第二次读回来是 0
    else
        wheel_data = cnt_read() & 0x0f;

    return wheel_data;
}

static void test_old_pattern_was_broken(void)
{
    enc_cnt = 1;
    CHECK_EQ_INT(0, (int8_t)old_double_read(), "旧写法：正转一格丢失");

    enc_cnt = 0x0F;
    CHECK_EQ_INT(-16, (int8_t)old_double_read(), "旧写法：反转一格变成 -16");
}

int main(void)
{
    test_cnt_to_data();
    test_single_read();
    test_old_pattern_was_broken();

    return test_summary("wheel / encoder");
}
