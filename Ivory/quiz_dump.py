# -*- coding:utf-8 -*-
#用于导出文件头为fHKQ的文本。
#by Destinyの火狐 2018.05.02
import struct
import os
import sys
import io

def byte2int(byte):
	long_tuple=struct.unpack('L',byte)
	long = long_tuple[0]
	return long

def short2int(byte):
	long_tuple=struct.unpack('H',byte)
	long = long_tuple[0]
	return long

def int2byte(num):
	return struct.pack('L',num)

def FormatString(string, count):
	res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
	return res

def dumpstr(src):
	cstr = ''
	c = src.read(2)
	while c != b'\x00\x00':
		if c[0] == 0:
			cstr += c.decode('utf-16be')
		else:
			cstr += c.decode('932')
		if c == b'\xC2\xCE':
			return cstr
		c = src.read(2)
	return cstr

for f in os.listdir('SCRIPT'):
	if not f.endswith('.dat'):
		continue
	fs = open('SCRIPT/'+f,'rb')
	fs.seek(0x0C, os.SEEK_SET)
	pos = byte2int(fs.read(4))
	fs.seek(pos + 0x0C, os.SEEK_SET)
	code_length = byte2int(fs.read(4))
	code_length = code_length - byte2int(fs.read(4))
	fs.seek(4,os.SEEK_CUR)
	group_count = byte2int(fs.read(4))
	fs.close()
	print('name:%s code_length:0x%X group_count:0x%X'%(f,code_length,group_count))
	qzt = open('SCRIPT/'+f[:-4]+'.QZT','rb')
	tex = open('SCRIPT/'+f[:-4]+'.TEX','rb')
	num_list = []
	count_list = []
	off_list = []
	str_list = []
	for i in range(0,group_count):
		num_list.append(byte2int(qzt.read(4)))
		count_list.append(byte2int(qzt.read(4)))
		off_list.append(byte2int(qzt.read(4)) + i * 0x0C)
	for i in range(0,group_count):
		qzt.seek(off_list[i],os.SEEK_SET)
		for j in range(0,count_list[i]):
			num = byte2int(qzt.read(4))
			text_offset = byte2int(qzt.read(4))
			answer_count = byte2int(qzt.read(4))
			print('num:0x%X text_offset:0x%X answer_count:0x%X'%(num,text_offset,answer_count))
			tex.seek(text_offset,os.SEEK_SET)
			l = dumpstr(tex)
			if len(l) != 0:
				str_list.append(l)
			else:
				print('error!')
				os.system('pause')
			for k in range(0,answer_count):
				right = byte2int(qzt.read(4))
				answer_offset = byte2int(qzt.read(4))
				print('\tanswer_offset:0x%X right:%d'%(answer_offset,right))
				tex.seek(answer_offset,os.SEEK_SET)
				l = dumpstr(tex)
				if len(l) != 0:
					str_list.append(l)
				else:
					print('error!')
					os.system('pause')
	qzt.close()
	tex.close()
	if len(str_list) != 0:
		if os.path.exists('text') == False:
			os.mkdir('text')
		i = 0
		dst = open('text/'+f[:-4]+'.txt','w',encoding='utf16')
		for string in str_list:
			dst.write(FormatString(string.rstrip(), i))
			i += 1
		dst.close()