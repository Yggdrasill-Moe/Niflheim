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

def ReadPart1(src):
	str_list = []
	for i in range(5):
		l = dumpstr(src)
		str_list.append(l)
	return str_list

def WritePart1(dst,str_list,dicts,i):
	for k in range(5):
		try:
			for ch in str_list[i]:
				if dicts[ch] > 0xFF:
					dst.write(struct.pack('>H',dicts[ch]))
				else:
					dst.write(struct.pack('B',dicts[ch]))
		except Exception as inst:
			print(str_list[i])
		dst.write(struct.pack('B',0))
		i += 1
	return i

def ReadPart2_fix(src,count):
	bin_list_all = []
	bin_list = []
	for i in range(count):
		buff = byte2int(src.read(4))
		bin_list.append(int2byte(buff))
		if buff == 0:
			print(src,hex(src.tell()))
			continue
		bin_list_all.append(b''.join(bin_list))
		bin_list = []
		for k in range(2):
			l = dumpstr(src)
		bin_list.append(src.read(5))
		bin_list.append(src.read(104))
	return bin_list_all

def WritePart2(dst,str_list,bin_list_all,dicts,count,i):
	for k in range(count):
		dst.write(bin_list_all[k])
		for j in range(2):
			try:
				for ch in str_list[i]:
					if dicts[ch] > 0xFF:
						dst.write(struct.pack('>H',dicts[ch]))
					else:
						dst.write(struct.pack('B',dicts[ch]))
			except Exception as inst:
				print(str_list[i],ch)
			dst.write(struct.pack('B',0))
			i += 1
	return i

def Sou_ReadPart2_fix(src,count):
	bin_list_all = []
	bin_list = []
	for i in range(count):
		buff = byte2int(src.read(4))
		bin_list.append(int2byte(buff))
		if buff == 0:
			buff = byte2int(src.read(4))
			bin_list.append(int2byte(buff))
			while buff == 0xFFFFFFFF:
				bin_list.append(src.read(24))
				buff = byte2int(src.read(4))
				bin_list.append(int2byte(buff))
			if buff != 0:
				src.seek(-4,os.SEEK_CUR)
				bin_list.pop()
			else:
				if src.tell() == 0xb0f or src.tell() == 0xc2a or src.tell() == 0x1da3 or src.tell() == 0x3628 or src.tell() == 0x3722 or src.tell() == 0x8833:
					bin_list.append(src.read(0x50))
				elif src.tell() == 0x8009:
					bin_list.append(src.read(0x4))
				elif src.tell() == 0x8778:
					bin_list.append(src.read(0x14))
				elif src.tell() == 0xd23:
					bin_list.append(src.read(0x34))
				elif src.tell() == 0x664a:
					bin_list.append(src.read(0x4c))
				elif src.tell() == 0x1c7b or src.tell() == 0x8926:
					bin_list.append(src.read(0x88))
				elif src.tell() == 0x4ed1:
					bin_list.append(src.read(0xac))
				else:
					print(hex(src.tell()))
					break
		bin_list_all.append(b''.join(bin_list))
		bin_list = []
		for k in range(2):
			l = dumpstr(src)
		bin_list.append(src.read(5))
		bin_list.append(src.read(104))
	return bin_list_all

def Sou_WritePart2(dst,str_list,bin_list_all,dicts,count,i):
	for k in range(count):
		dst.write(bin_list_all[k])
		for j in range(2):
			try:
				for ch in str_list[i]:
					if dicts[ch] > 0xFF:
						dst.write(struct.pack('>H',dicts[ch]))
					else:
						dst.write(struct.pack('B',dicts[ch]))
			except Exception as inst:
				print(str_list[i],ch)
			dst.write(struct.pack('B',0))
			i += 1
	return i

def ReadPart3_fix(src):
	buff = byte2int(src.read(4))
	for i in range(2):
		l = dumpstr(src)
	return buff

def WritePart3(dst,buff,str_list,dicts,i):
	dst.write(int2byte(buff))
	for k in range(2):
		try:
			for ch in str_list[i]:
				if dicts[ch] > 0xFF:
					dst.write(struct.pack('>H',dicts[ch]))
				else:
					dst.write(struct.pack('B',dicts[ch]))
		except Exception as inst:
			print(str_list[i])
		dst.write(struct.pack('B',0))
		i += 1
	return i

def ReadPart4_fix(src):
	bin_list = []
	for s in range(2):
		count = byte2int(src.read(4))
		bin_list.append(count)
		for i in range(count):
			l = dumpstr(src)
			l = dumpstr(src)
	return bin_list

def WritePart4(dst,str_list,bin_list,dicts,i):
	for s in range(2):
		count = bin_list[s]
		dst.write(int2byte(count))
		for j in range(count):
			for k in range(2):
				try:
					for ch in str_list[i]:
						if dicts[ch] > 0xFF:
							dst.write(struct.pack('>H',dicts[ch]))
						else:
							dst.write(struct.pack('B',dicts[ch]))
				except Exception as inst:
					print(str_list[i])
				dst.write(struct.pack('B',0))
				i += 1
	return i

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

def Writebin(src,dst,i):
	buff = src.tell()
	src.seek(i, os.SEEK_SET)
	offset = byte2int(src.read(4))
	src.seek(buff)
	while src.tell() != offset:
		dst.write(src.read(1))
	buff = dst.tell()
	dst.seek(i, os.SEEK_SET)
	dst.write(int2byte(buff))
	dst.seek(0, os.SEEK_END)

def main():
	tbl = open('tbl.txt','r',encoding='utf16')
	dicts = {}
	for rows in tbl:
		row = rows.rstrip('\r\n').split('=')
		if len(row) == 3:
			row[1] = '='
		dicts[row[1]]=int(row[0],16)
	for f in os.listdir('mek'):
		if not f.endswith('._mek'):
			continue
		src = open('mek/' + f,'rb')
		txt = open('mek/' + f[:-5] + '.txt', 'r', encoding='utf16')
		str_list = []
		for rows in txt:
			if rows[0] != '●':
				continue
			row = txt.readline().rstrip('\r\n')
			str_list.append(row)
		src.seek(0, os.SEEK_END)
		filesize = src.tell()
		src.seek(0, os.SEEK_SET)
		str_offset = byte2int(src.read(4))
		dst = open('mek/' + f[:-5] + '.mek','wb')
		src.seek(0, os.SEEK_SET)
		dst.write(src.read(str_offset))
		i = 0
		ReadPart1(src)
		i = WritePart1(dst,str_list,dicts,i)
		Writebin(src,dst,4)
		Writebin(src,dst,8)
		src.seek(8, os.SEEK_SET)
		str_offset = byte2int(src.read(4))
		src.seek(str_offset)
		count = byte2int(src.read(4))
		if 'mek/' + f[:-5] + '.txt' != 'mek/sou.txt':
			bin_list_all = ReadPart2_fix(src,count)
			dst.write(int2byte(count))
			i = WritePart2(dst,str_list,bin_list_all,dicts,count,i)
		else:
			bin_list_all = Sou_ReadPart2_fix(src,226)
			dst.write(int2byte(count))
			i = Sou_WritePart2(dst,str_list,bin_list_all,dicts,226,i)
		src.seek(-5,os.SEEK_CUR)
		src.seek(-104,os.SEEK_CUR)
		Writebin(src,dst,12)
		src.seek(12, os.SEEK_SET)
		str_offset = byte2int(src.read(4))
		src.seek(str_offset)
		buff = ReadPart3_fix(src)
		i = WritePart3(dst,buff,str_list,dicts,i)
		Writebin(src,dst,16)
		src.seek(16, os.SEEK_SET)
		str_offset = byte2int(src.read(4))
		src.seek(str_offset)
		buff = byte2int(src.read(4))
		dst.write(int2byte(buff))
		if buff != 0xFFFFFFFF:
			bin_list = ReadPart4_fix(src)
			i = WritePart4(dst,str_list,bin_list,dicts,i)
		Writebin(src,dst,20)
		src.seek(20, os.SEEK_SET)
		offset = byte2int(src.read(4))
		src.seek(offset)
		data = src.read(filesize - src.tell())
		dst.write(data)
main()