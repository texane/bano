#!/usr/bin/env sh

if [ -z "$BANO_SMS_KEY" ]; then
 echo "BANO_SMS_KEY not set" ;
 exit ;
fi

chmod 755 ./sh/disable_bbb_leds.sh ;
./sh/disable_bbb_leds.sh ;

./a.out -op_name listen > /dev/null 2>&1 &
