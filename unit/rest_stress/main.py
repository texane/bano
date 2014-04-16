#!/usr/bin/env python

import httplib
import urllib
import string

kv = {}
kv['naddr'] = '0x5c5f8548'
kv['key'] = 0x0000
kv['val'] = 0
kv['op'] = 'set'
kv['is_ack'] = 1
kv['fmt'] = 'plain'

conn = httplib.HTTPConnection("dedibox:1180")

npass = 0
while True:
 if kv['val'] == 1: kv['val'] = 0
 else: kv['val'] = 1
 params = urllib.urlencode(kv)
 print(str(params))
 conn.request("GET", "/", params)
 resp = conn.getresponse()
 compl_data = eval(resp.read())
 compl_err = compl_data[0]
 compl_val = compl_data[1]
 if compl_err != 0: print("err=" + str(compl_err) + " at " + str(npass))
 npass = npass + 1

conn.close()
