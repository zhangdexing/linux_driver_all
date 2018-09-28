#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# python allow_selinux.py output_path log_path
import sys,os,time

def save2file(line):
	#print(line)
#buf4
        i=line.find('{')
        j=line.find('}')
	if (i==-1 or j==-1):
		print("except4 occur!!")
		print(line)
		return
        buf4=line[i+2:j-1]
#buf1
        i=line.find('scontext=')
	if i==-1:
		return
	try:
		buf1=line[i:].split(':')[2]
	except :
		print("except1 occur!!")
		print(line)
		return
	buf1=buf1.replace('\r','')
	buf1=buf1.replace('\n','')

#buf2
	i=line.find('tcontext=')
	if i==-1:
		return
	try:
		buf2=line[i:].split(':')[2]
	except :
		print("except2 occur!!")
		print(line)
		return
	buf2=buf2.replace('\r','')
	buf2=buf2.replace('\n','')

#buf3
	i=line.find('tclass=')
	if i==-1:
		return
	try:
		buf3=line[i:].split('=')[1]
		buf3=buf3.split(' ')[0]
	except :
		print("except3 occur!!")
		print(line)
		return
	buf3=buf3.replace('\r','')
	buf3=buf3.replace('\n','')

	savebuf='allow %s %s:%s {%s};\n'%(buf1,buf2,buf3,buf4)

	rfd.seek(0)
	for rline in open(sys.argv[1]):
		rline = rfd.readline()
		if rline.find(savebuf) != -1:
			#print('find same line :' + savebuf)
			return

	#print('write to file :' +savebuf)
	wfd.writelines(savebuf)
	wfd.flush()

ISOTIMEFORMAT='%Y-%m-%d %X'
cur_time = time.strftime( ISOTIMEFORMAT, time.localtime() )
print("start")
fd = open(sys.argv[2],'r')
wfd = open(sys.argv[1],'a')
rfd = open(sys.argv[1],'r')
wfd.writelines('\n# flyaudio allow selinux ' + cur_time + ' ' + sys.argv[2] + '\n')
wfd.flush()
for line in open(sys.argv[2]):  
    line = fd.readline()
    if line.find('avc:') != -1:
	if line.find('denied') != -1:
		if line.find('scontext=') != -1:
			if line.find('tcontext=') != -1:
				if line.find('tclass=') != -1:
					save2file(line)
rfd.close()
wfd.close()
fd.close()
print("end")

