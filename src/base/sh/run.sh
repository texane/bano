#!/usr/bin/env sh

if [ -z "$BANO_SMS_KEY" ]; then
 echo "BANO_SMS_KEY not set" ;
 exit ;
fi

if [ ! -x ./sh/disable_bbb_leds.sh ]; then
 chmod 755 ./sh/disable_bbb_leds.sh ;
fi

./sh/disable_bbb_leds.sh ;
./a.out -op_name listen > /dev/null 2>&1 & ;
