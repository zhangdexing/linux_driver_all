#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# python allow_selinux.py log_path output_path
import sys,os

def save2file(line):
        print line
#buf4
        i=line.find('{')
        j=line.find('}')
        buf4=line[i+2:j-1]
#buf1
        i=line.find('scontext')
	if i==-1:
		return
	buf1=line[i:].split(':')[2]
#buf2
	i=line.find('tcontext')
	if i==-1:
		return
	buf2=line[i:].split(':')[2]
#buf3
	i=line.find('tclass')
	if i==-1:
		return
	buf3=line[i:].split('=')[1]
	buf3=buf3.split(' ')[0]

	savebuf='allow %s %s:%s {%s};\n'%(buf1,buf2,buf3,buf4)
	print savebuf
	wfd.writelines(savebuf)

fd = open(sys.argv[1],'r')
wfd = open(sys.argv[2],"w+")
for line in fd.readlines():
	flag=line.find('avc: denied')
	if flag != -1:
                save2file(line)
wfd.close()
fd.close()

