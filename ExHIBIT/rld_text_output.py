# -*- coding:utf-8 -*-
#用于导出rld文件的文本
#by Darkness-TX
#2017.11.17

import struct
import os
import sys
import io

def byte2int(byte):
	long_tuple = struct.unpack('L',byte)
	long = long_tuple[0]
	return long

def int2byte(num):
	return struct.pack('L',num)

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

def FormatString(string, count):
	res = "○%08d○\n%s\n●%08d●\n%s\n\n"%(count, string, count, string)
	return res

def ReadHeader(src):
	tag = ''
	magic = src.read(4)
	ver = byte2int(src.read(4))
	offset = byte2int(src.read(4))
	count = byte2int(src.read(4))
	flag = byte2int(src.read(4))
	if flag == 1:
		tag = dumpstr(src)
	return magic, ver, offset, count, flag, tag

def Opcode_Analysis(src):
	buff = byte2int(src.read(4))
	op = buff & 0xFFFF
	init_count = (buff & 0xFF0000) >> 16
	str_count = (buff >> 24) & 0xF
	unk = buff >> 28
	return op, init_count, str_count, unk

def Get_Name_Table(dump_str):
	names = {}
	for l in dump_str:
		group = l.split(',')
		names[int(group[0])] = group[3]
	return names

def rld_output(fname, name_table=[]):
	print(fname)
	src = open(os.path.join('rld', fname), 'rb')
	magic, ver, offset, count, flag, tag = ReadHeader(src)
	if magic != b'\00DLR':
		print(fname + "不是支持的类型")
	else:
		src.seek(offset + 4, os.SEEK_SET)
		dst = open(os.path.join('rld', fname[:-4] + '.txt'), 'w', encoding='utf16')
		l = 0
		dump_str = []
		for i in range(0, count):
			opcode, init_count, str_count, unk = Opcode_Analysis(src)
			all_init = []
			all_str =[]
			for j in range(0, init_count):
				val = src.read(4)
				all_init.append(byte2int(val))
			for k in range(0, str_count):
				row = dumpstr(src)
				all_str.append(row)
			if opcode == 28:
				if all_init[0] in name_table:
					dst.write(FormatString(name_table[all_init[0]], l))
					dump_str.append(name_table[all_init[0]])
					l += 1
				for string in all_str:
					if string != '*' and string != '$noname$' and len(string) != 0 and string.count(',') < 2:
						dst.write(FormatString(string, l))
						dump_str.append(string)
						l += 1
			elif opcode == 21:
				for string in all_str:
					if string != '*' and string != '$noname$' and len(string) != 0 and string.count(',') < 2:
						dst.write(FormatString(string, l))
						dump_str.append(string)
						l += 1
			elif opcode == 48:
				dst.write(FormatString(all_str[0], l))
				dump_str.append(all_str[0])
				l += 1
			elif opcode == 191:
				if len(all_str[0]) != len(all_str[0].encode('932')):
					dst.write(FormatString(all_str[0], l))
					dump_str.append(all_str[0])
					l += 1
		return dump_str

def main():
	dump_str = rld_output('defChara.bin')
	name_table = Get_Name_Table(dump_str)
	for f in os.listdir('rld'):
		if not f.endswith('.bin') or f == 'defChara.bin':
			continue
		rld_output(f, name_table)
main()