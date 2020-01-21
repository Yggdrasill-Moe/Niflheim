# -*- coding:utf-8 -*-

import struct
import os
import sys
import io

def byte2int(byte):
	long_tuple=struct.unpack('L',byte)
	long = long_tuple[0]
	return long

def int2byte(num):
	return struct.pack('L',num)

def FormatString(string, count):
	res = "○%08d○\n%s\n●%08d●\n%s\n\n"%(count, string, count, string)
	return res

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

for f in os.listdir('Config'):
	if not f.endswith('._dat'):
		continue
	fs = open('Config/'+f,'rb')
	fs.seek(0, os.SEEK_END)
	filesize=fs.tell()
	fs.seek(0, os.SEEK_SET)
	count = byte2int(fs.read(4))
	types = [byte2int(fs.read(4)) for i in range(count)]
	str_list = []
	while fs.tell() < filesize:
		for t in types:
			if t == 2:
				fs.read(4)
			elif t == 1:
				l=dumpstr(fs)
				if len(l) != 0:
					str_list.append(l)
	if len(str_list)!=0:
		dstname = 'Config/' + f[:-5] + '.txt'
		dst = open(dstname, 'w', encoding='utf16')
		i = 0
		for string in str_list:
			dst.write(FormatString(string, i))
			i += 1
		dst.close()
	fs.close()