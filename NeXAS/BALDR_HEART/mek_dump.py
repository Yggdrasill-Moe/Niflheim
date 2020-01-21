# -*- coding:utf-8 -*-
# update for BALDR HEART EXE 2017.09.14
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

def ReadPart1(src):
	str_list = []
	for i in range(5):
		l=dumpstr(src)
		str_list.append(l)
	return str_list

def ReadPart2(src,count):
	str_list = []
	for i in range(count):
		buff = byte2int(src.read(4))
		if buff == 0:
			print(src,hex(src.tell()))
			continue
		for k in range(2):
			l=dumpstr(src)
			str_list.append(l)
		src.seek(5,os.SEEK_CUR)
		src.seek(104,os.SEEK_CUR)
	return str_list

def Sou_ReadPart2(src,count):
	str_list = []
	for i in range(count):
		buff = byte2int(src.read(4))
		if buff == 0:
			buff = byte2int(src.read(4))
			while buff == 0xFFFFFFFF:
				src.seek(24,os.SEEK_CUR)
				buff = byte2int(src.read(4))
			if buff != 0:
				src.seek(-4,os.SEEK_CUR)
			else:
				if src.tell() == 0xb0f or src.tell() == 0xc2a or src.tell() == 0x1da3 or src.tell() == 0x3628 or src.tell() == 0x3722 or src.tell() == 0x8833 or src.tell() == 0x1656 or src.tell() == 0x221c or src.tell() == 0x3e5f or src.tell() == 0x3f59:
					src.seek(0x50,os.SEEK_CUR)
				elif src.tell() == 0x8009:
					src.seek(0x4,os.SEEK_CUR)
				elif src.tell() == 0x8778:
					src.seek(0x14,os.SEEK_CUR)
				elif src.tell() == 0xd23:
					src.seek(0x34,os.SEEK_CUR)
				elif src.tell() == 0x664a or src.tell() == 0x764b:
					src.seek(0x4c,os.SEEK_CUR)
				elif src.tell() == 0x1c7b or src.tell() == 0x8926 or src.tell() == 0x1749 or src.tell() == 0x20f4:
					src.seek(0x88,os.SEEK_CUR)
				elif src.tell() == 0x4ed1 or src.tell() == 0x5ed2:
					src.seek(0xac,os.SEEK_CUR)
				elif src.tell() == 0x900a:
				#	src.seek(0x4,os.SEEK_CUR)
					break
				else:
					print(hex(src.tell()))
					os.system('pause')
					break
		for k in range(2):
			l = dumpstr(src)
			str_list.append(l)
		src.seek(5,os.SEEK_CUR)
		src.seek(104,os.SEEK_CUR)
	return str_list

def ReadPart3(src):
	str_list = []
	buff = byte2int(src.read(4))
	for i in range(2):
		l=dumpstr(src)
		str_list.append(l)
	return str_list

def ReadPart4(src):
	str_list = []
	for s in range(2):
		count = byte2int(src.read(4))
		for i in range(count):
			l=dumpstr(src)
			str_list.append(l)
			l=dumpstr(src)
			str_list.append(l)
	return str_list

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

for f in os.listdir('Dat'):
	if not f.endswith('._mek'):
		continue
	src = open('Dat/'+f,'rb')
	src.seek(0, os.SEEK_END)
	filesize = src.tell()
	src.seek(0, os.SEEK_SET)
	str_offset = byte2int(src.read(4))
	src.seek(str_offset)
	all_str = []
	all_str = ReadPart1(src)
	dstname = 'Dat/' + f[:-5] + '.txt'
	dst = open(dstname, 'w', encoding='utf16')
	i = 0
	for string in all_str:
		dst.write(FormatString(string, i))
		i += 1
	src.seek(8, os.SEEK_SET)
	str_offset = byte2int(src.read(4))
	src.seek(str_offset)
	count = byte2int(src.read(4))
	if dstname != 'Dat/sou.txt':
		all_str = ReadPart2(src,count)
	else:
		all_str = Sou_ReadPart2(src,226)
	for string in all_str:
		dst.write(FormatString(string, i))
		i += 1
	src.seek(12, os.SEEK_SET)
	str_offset = byte2int(src.read(4))
	src.seek(str_offset)
	all_str = ReadPart3(src)
	for string in all_str:
		dst.write(FormatString(string, i))
		i += 1
	src.seek(16, os.SEEK_SET)
	str_offset = byte2int(src.read(4))
	src.seek(str_offset)
	buff = byte2int(src.read(4))
	if buff != 0xFFFFFFFF:
		all_str = ReadPart4(src)
		for string in all_str:
			dst.write(FormatString(string, i))
			i += 1
	src.close()
	dst.close()