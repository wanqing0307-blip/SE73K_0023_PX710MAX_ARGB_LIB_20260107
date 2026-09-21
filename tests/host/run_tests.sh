#!/bin/sh
# 主机侧回归测试。需要一个 PC 上的 gcc（MinGW / Linux 均可），和 Keil 无关。
#   sh tests/host/run_tests.sh
set -e

HERE=$(dirname "$0")
ROOT="$HERE/../.."
OUT="$HERE/_out"
CC=${CC:-gcc}
CFLAGS="-std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-comment -O1 -g"
INC="-I$HERE -I$HERE/mock -I$ROOT/projects/mouse_demo/periph -I$ROOT/projects/mouse_demo/main -I$ROOT/projects/mouse_demo/led -I$ROOT/projects/mouse_demo/flash"

mkdir -p "$OUT"
rc=0

echo "主机回归测试"

$CC $CFLAGS $INC "$HERE/test_wheel.c"    -o "$OUT/test_wheel"    && "$OUT/test_wheel"    || rc=1
$CC $CFLAGS $INC "$HERE/test_debounce.c" -o "$OUT/test_debounce" && "$OUT/test_debounce" || rc=1
$CC $CFLAGS $INC "$HERE/test_lvd.c" "$ROOT/projects/mouse_demo/periph/lvd.c" \
    -o "$OUT/test_lvd" && "$OUT/test_lvd" || rc=1

if [ $rc -eq 0 ]; then
    echo "全部通过"
else
    echo "有用例失败"
fi
exit $rc
