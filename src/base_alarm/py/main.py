#!/usr/bin/env python

import sys
import smtplib
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText
from email.MIMEImage import MIMEImage
 
def sendemail(msg, login, password, smtpserver='smtp.gmail.com:587'):
 server = smtplib.SMTP(smtpserver)
 server.starttls()
 server.login(login,password)
 server.sendmail(msg['From'], msg['To'], msg.as_string())
 server.quit()
 return

def get_filename(filename, ext):
 toks = filename.rsplit('.', 1)
 return toks[0] + '.' + ext

def convert_file(filename, ext):
 import Image
 new_filename = get_filename(filename, ext)
 try: im = Image.open(filename)
 except: return None
 im.save(new_filename)
 return new_filename

if len(sys.argv) > 1: im_names = sys.argv[1:]
else: im_names = [ '/tmp/foo.bmp' ]

new_im_names = []
for im_name in im_names:
 new_im_name = convert_file(im_name, 'png')
 if new_im_name != None:
  new_im_names.append(new_im_name)

msg = MIMEMultipart()
msg['From'] = 'base@home'
msg['To'] = 'fabien.lementec@gmail.com'
msg['Subject'] = 'base alert'
msg.attach(MIMEText('text content'))

for new_im_name in new_im_names:
 msg.attach(MIMEImage(file(new_im_name).read()))

# TODO: set account info
sendemail(msg, 'login', 'password')
