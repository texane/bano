#!/usr/bin/env sh

while true; do
 wget 'http://freebox:1180/?naddr=0x5c5f8548&key=0x0000&val=0&op=set&is_ack=1' -O /tmp/stress.o > /dev/null 2>&1;
 < /tmp/stress.o grep 'status: error' && echo 'error' ;
 wget 'http://freebox:1180/?naddr=0x5c5f8548&key=0x0000&val=1&op=set&is_ack=1' -O /tmp/stress.o > /dev/null 2>&1;
 < /tmp/stress.o grep 'status: error' && echo 'error' ;
done
