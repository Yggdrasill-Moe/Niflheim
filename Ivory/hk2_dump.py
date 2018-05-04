# -*- coding:utf-8 -*-
#用于导出文件头为fAGS的文本。
#by Destinyの火狐 2018.04.28
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
		c = src.read(2)
	return cstr

for f in os.listdir('SCRIPT'):
	if not f.endswith('.hk2'):
		continue
	fs = open('SCRIPT/'+f,'rb')
	fs.seek(0x0C, os.SEEK_SET)
	pos = byte2int(fs.read(4))
	fs.seek(pos - 4, os.SEEK_CUR)
	pos = byte2int(fs.read(4))
	fs.seek(pos - 4, os.SEEK_CUR)
	filesize = byte2int(fs.read(4))
	fs.seek(4, os.SEEK_CUR)
	code_length = byte2int(fs.read(4)) - 0x1C
	fs.seek(8, os.SEEK_CUR)
	code_count = byte2int(fs.read(4))
	print('name:%s code_length:0x%X code_count:%d'%(f,code_length,code_count))
	cod = open('SCRIPT/'+f[:-4]+'.COD','rb')
	tex = open('SCRIPT/'+f[:-4]+'.TEX','rb')
	tex.seek(0,os.SEEK_END)
	#一般code_count最小为1，这个时候代表没文本，也可以判断TEX的大小是否为0
	if tex.tell() == 0:
		print('warning！ TEX filesize is zero')
		fs.close()
		cod.close()
		tex.close()
		continue
	cod.seek(code_length,os.SEEK_SET)
	off_list = []
	for i in range(0,code_count):
		off_list.append(byte2int(cod.read(4)))
	str_list = []
	count = 0
	for i in off_list:
		cod.seek(i,os.SEEK_SET)
		opcode = short2int(cod.read(2))
		op_size = short2int(cod.read(2))
		if opcode == 0x01:
			count += 1
			cod.seek(4,os.SEEK_CUR)
			offset = byte2int(cod.read(4))
			tex.seek(offset,os.SEEK_SET)
			l = dumpstr(tex)
			if len(l) != 0:
				str_list.append(l)
			else:
				print('error! opcode:0x%X offset:0x%X cod_offset:0x%X'%(opcode,offset,cod.tell() - 4))
				os.system('pause')
		elif opcode == 0x02:
			count += 1
			cod.seek(4,os.SEEK_CUR)
			name_off = byte2int(cod.read(4))
			offset = byte2int(cod.read(4))
			tex.seek(name_off,os.SEEK_SET)
			name = dumpstr(tex)
			if len(name) == 0:
				print('error! opcode:0x%X offset:0x%X name_off:0x%X cod_offset:0x%X'%(opcode,offset,name_off,cod.tell() - 4))
				os.system('pause')
			tex.seek(offset,os.SEEK_SET)
			l = dumpstr(tex)
			if len(l) == 0:
				print('error! opcode:0x%X offset:0x%X cod_offset:0x%X'%(opcode,offset,cod.tell() - 4))
				os.system('pause')
			#str_list.append(name + '|「' + l)
			str_list.append(name)
			str_list.append('「' + l)
		elif opcode == 0x2F:
			count += 1
			offset = byte2int(cod.read(4))
			tex.seek(offset,os.SEEK_SET)
			l = dumpstr(tex)
			if len(l) != 0:
				str_list.append(l)
			else:
				print('error! opcode:0x%X offset:0x%X cod_offset:0x%X'%(opcode,offset,cod.tell() - 4))
				os.system('pause')
		elif opcode == 0x09:
			count += 1
			cod.seek(8,os.SEEK_CUR)
			choice = byte2int(cod.read(4))
			choice_list = []
			for i in range(0,choice):
				cod.seek(4,os.SEEK_CUR)
				choice_list.append(byte2int(cod.read(4)))
				cod.seek(4,os.SEEK_CUR)
			for offset in choice_list:
				tex.seek(offset,os.SEEK_SET)
				l = dumpstr(tex)
				if len(l) != 0:
					str_list.append(l)
				else:
					print('error! opcode:0x%X offset:0x%X cod_offset:0x%X'%(opcode,offset,cod.tell() - 4))
					os.system('pause')
		elif opcode == 0x35 or opcode == 0x1D:
			count += 1
			tex.seek(0,os.SEEK_SET)
			l = dumpstr(tex)
			if len(l) != 0:
				str_list.append(l)
			else:
				print('error! opcode:0x%X offset:0x%X cod_offset:0x%X'%(opcode,offset,cod.tell() - 4))
				os.system('pause')
		#特殊情况，跳过
		elif opcode == 0x14 or opcode == 0x45:
			count += 1
		else:
			print('warning! opcode:0x%X cod_offset:0x%X'%(opcode,cod.tell() - 4))
			os.system('pause')
	if count != code_count:
		print('warning！ count:%d code_count:%d'%(count,code_count))
		os.system('pause')
	if len(str_list) != 0:
		if os.path.exists('text') == False:
			os.mkdir('text')
		i = 0
		dst = open('text/'+f[:-4]+'.txt','w',encoding='utf16')
		for string in str_list:
			dst.write(FormatString(string, i))
			i += 1
		dst.close()
	fs.close()
	cod.close()
	tex.close()