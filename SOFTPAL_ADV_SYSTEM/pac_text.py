# -*- coding:utf-8 -*-

import os
import struct
import io
import sys

def help():
	print('usage:pac_text [-d|-i]')
	print('\t-d\tdump text from TEXT.DAT')
	print('\t-i\tpack text into TEXT.DAT_NEW')

def byte2int(byte):
	long_tuple=struct.unpack('L',byte)
	long = long_tuple[0]
	return long

def int2byte(num):
	return struct.pack('L',num)

def FormatString(string, count):
	res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
	return res

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

def dump():
	src = open('TEXT.DAT', 'rb')
	dst = open('text.txt', 'w', encoding='utf16')
	src.seek(0x0C)
	count = byte2int(src.read(4))
	for i in range(0, count):
		num = byte2int(src.read(4))
		dst.write(FormatString(dumpstr(src),num))
	print('all %d row'%(count))

def pack():
	src = open('TEXT.DAT', 'rb')
	fin = open('text.txt', 'r',encoding='utf16')
	dst = open('TEXT.DAT_NEW', 'wb')
	buff = src.read(0x10)
	dst.write(buff)
	for rows in fin:
		if rows[0] != '●':
			continue
		row = rows[1:].rstrip('\r\n').split('●')
		num = int2byte(int(row[0]))
		line = row[1].encode('932')
		dst.write(num)
		dst.write(line)
		dst.write(struct.pack('B',0))



def main():
	print('project：Niflheim-SOFTPAL_ADV_SYSTEM')
	print('用于导入导出文本(TEXT.DAT)')
	print('by Destinyの火狐 2016.11.08\n')
	if len(sys.argv) != 2:
		help()
	else:
		if sys.argv[1] == '-d':
			dump()
		elif sys.argv[1] == '-i':
			pack()
		else:
			help()
main()