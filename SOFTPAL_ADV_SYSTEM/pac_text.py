# -*- coding:utf-8 -*-

import os
import struct
import io
import sys

def help():
	print('usage:pac_text [-d|-i]')
	print('\t-d\tdump text from TEXT.DAT SCRIPT.SRC')
	print('\t-i\tpack text into TEXT.DAT_NEW SCRIPT.SRC_NEW')

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
	ofp = open('SCRIPT.SRC','rb')
	dfp = open('script.txt','w',encoding='utf16')
	ofp.seek(0x6C450)
	#全文件扫描文本地址，所以肯定不止一处，需要确定一个开始扫描的地址，魔女こいにっき是这个地址，各个游戏都不同，自行判断
	src.seek(0x0C)
	count = byte2int(src.read(4))
	for i in range(0, count):
		if i >= 0x175:#文本开始的编号，同样各个游戏都不同
			off = src.tell()
			while 1:
				if byte2int(ofp.read(4)) == off:
					res = "○%08d○%X\n"%(i,ofp.tell() - 4)
					print('text num:%d script off:0x%X'%(i,ofp.tell() - 4))
					dfp.write(res)
					break
		num = byte2int(src.read(4))
		dst.write(FormatString(dumpstr(src),num))
	print('\nall %d row'%(count))

def pack():
	src = open('TEXT.DAT', 'rb')
	fin = open('text.txt', 'r',encoding='utf16')
	dst = open('TEXT.DAT_NEW', 'wb')
	fp = open('SCRIPT.SRC','rb')
	ofp = open('SCRIPT.SRC_NEW','wb')
	dfp = open('script.txt','r',encoding='utf16')
	tbl = open('tbl.txt','r',encoding='utf16')
	dicts = {}
	for rows in tbl:
		row = rows.rstrip('\r\n').split('=')
		if len(row) == 3:
			row[1] = '='
		dicts[row[1]]=int(row[0],16)
	buff = fp.read(os.path.getsize('SCRIPT.SRC'))
	ofp.write(buff)
	dict = {}
	for rows in dfp:
		row = rows[1:].rstrip('\r\n').split('○')
		num = str(int(row[0]))
		off = int(row[1],16)
		dict[num] = off
	buff = src.read(0x10)
	dst.write(buff)
	for rows in fin:
		if rows[0] != '●':
			continue
		row = rows[1:].rstrip('\r\n').split('●')
		try:
			if int(row[0]) >= 0x175:
				ofp.seek(dict[str(int(row[0]))])
				ofp.write(int2byte(dst.tell()))
				num = int2byte(int(row[0]))
				dst.write(num)
				for ch in row[1]:
					if dicts[ch] > 0xFF:
						dst.write(struct.pack('>H',dicts[ch]))
					else:
						dst.write(struct.pack('B',dicts[ch]))
			else:
				line = row[1].encode('932')
				num = int2byte(int(row[0]))
				dst.write(num)
				dst.write(line)
		except Exception as inst:
			print(row[0])
			print(inst)
			#os.system("pause")
		dst.write(struct.pack('B',0))

def main():
	print('project：Niflheim-SOFTPAL_ADV_SYSTEM')
	print('用于导入导出文本，确保TEXT.DAT与SCRIPT.SRC和本程序在同一目录下')
	print('by Destinyの火狐 2016.11.15\n')
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