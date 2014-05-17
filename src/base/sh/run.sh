#!/usr/bin/env sh

./sh/disable_bbb_leds.sh
./a.out -op_name listen > /dev/null 2>&1 &
