#!/usr/bin/env python

import httplib
import urllib
import string
import time
import random

kv = {}
kv['naddr'] = '0x5c5f8548'
kv['key'] = 0x0000
kv['val'] = 0
kv['op'] = 'set'
kv['is_ack'] = 1
kv['fmt'] = 'plain'

conn = httplib.HTTPConnection("192.168.0.11:80")

random.seed(None)

npass = 0
while True:
 if kv['val'] == 1: kv['val'] = 0
 else: kv['val'] = 1
 params = urllib.urlencode(kv)
 conn.request("GET", "/?", params)
 resp = conn.getresponse()
 compl_data = eval(resp.read())
 compl_err = compl_data[0]
 compl_val = compl_data[1]
 if compl_err: print("err=" + str(compl_err) + " at " + str(npass))
 if npass % 10 == 0: print("ok at npass = " + str(npass))
 npass = npass + 1
 ms = 250 + random.randint(0, 200)
 time.sleep(float(ms) / 1000.0)

conn.close()
