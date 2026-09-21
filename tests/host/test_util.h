/*******************************************************************************
* 文件名称: test_util.h
*
* 功能说明: 主机测试用的最小断言
*******************************************************************************/
#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include <stdio.h>

static int test_pass_count;
static int test_fail_count;

#define CHECK_EQ_INT(want, got, fmt, ...)                                      \
    do {                                                                       \
        long _w = (long)(want), _g = (long)(got);                              \
        if (_w == _g) {                                                        \
            test_pass_count++;                                                 \
        } else {                                                               \
            test_fail_count++;                                                 \
            printf("  FAIL %s:%d  " fmt "  期望 %ld, 实际 %ld\n",              \
                   __FILE__, __LINE__, ##__VA_ARGS__, _w, _g);                 \
        }                                                                      \
    } while (0)

#define CHECK_TRUE(cond, fmt, ...)  CHECK_EQ_INT(1, (cond) ? 1 : 0, fmt, ##__VA_ARGS__)

static int test_summary(const char *name)
{
    printf("  %-22s %3d 通过, %d 失败\n", name, test_pass_count, test_fail_count);
    return test_fail_count ? 1 : 0;
}

#endif
