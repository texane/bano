#!/usr/bin/env sh

[ -x ./sh/disable_bbb_leds.sh ] || chmod 755 ./sh/disable_bbb_leds.sh
./sh/disable_bbb_leds.sh
./a.out -op_name listen > /dev/null 2>&1 &
