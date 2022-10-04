# -*- coding:utf-8 -*-
#用于导出文件头为PS2A的解密文件的文本。
#by Darkness-TX 2018.04.20
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
	res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
	return res

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

for f in os.listdir('code'):
	if not f.endswith('.dec'):
		continue
	print(f)
	fs = open('code/'+f,'rb')
	fs.seek(0, os.SEEK_END)
	filesize = fs.tell()
	fs.seek(0x04, os.SEEK_SET)
	header_length = byte2int(fs.read(4))
	fs.seek(0x10, os.SEEK_SET)
	unknown1_count = byte2int(fs.read(4))
	unknown2_length = byte2int(fs.read(4))
	fs.seek(unknown1_count * 4 + unknown2_length + header_length, os.SEEK_SET)
	str_list = []
	offset_list = []
	len_list = []
	while fs.tell() != filesize:
		offset = fs.tell()
		l = dumpstr(fs)
		#是否需要大于2还要再看，因为数据块地址开头并非一定是文本或者全零，有遇到读取时读到\x01\x00的情况，加个大于2排除下
		if len(l) != 0 and (fs.tell() - offset) > 2:
			str_list.append(l)
			offset_list.append(offset - unknown1_count * 4 - unknown2_length - header_length)
			len_list.append(len(l.encode('932')))
	if len(str_list) != 0:
		dstname = 'code/' + f[:-4] + '.txt'
		dst = open(dstname, 'w', encoding='utf16')
		dstname = 'code/' + f[:-4] + '.scr.txt'
		scr_dst = open(dstname, 'w', encoding='utf16')
		i = 0
		for string in str_list:
			dst.write(FormatString(string, i))
			scr_dst.write("○%08d○%08X|%08X\n"%(i, offset_list[i], len_list[i]))
			i += 1
		dst.close()
	fs.close()