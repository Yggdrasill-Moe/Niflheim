# -*- coding:utf-8 -*-
#用于导入文件头为PS2A的解密文件的文本。
#by Darkness-TX 2018.04.23
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

for f in os.listdir('code'):
	if not f.endswith('.dec'):
		continue
	print(f)
	fs = open('code/' + f,'rb')
	data = fs.read()
	#0x0120020f
	#0x01200201
	op_dict = {}
	op_dict_new = {}
	off = 0
	while data.find(b'\x01\x02\x20\x01',off) != -1:
		off = data.find(b'\x01\x02\x20\x01',off) + 4
		op_dict[off] = byte2int(data[off:off + 4])
		op_dict_new[off] = byte2int(data[off:off + 4])
	filesize = fs.tell()
	fs.seek(0x04, os.SEEK_SET)
	header_length = byte2int(fs.read(4))
	fs.seek(0x10, os.SEEK_SET)
	unknown1_count = byte2int(fs.read(4))
	unknown2_length = byte2int(fs.read(4))
	fs.seek(0x1C, os.SEEK_SET)
	name_index_length = byte2int(fs.read(4))
	fs.seek(0x28, os.SEEK_SET)
	uncomprlen = byte2int(fs.read(4))
	fs.seek(0, os.SEEK_SET)
	data = fs.read(unknown1_count * 4 + unknown2_length + header_length)
	txt = open('code/' + f[:-4] + '.txt','r',encoding='utf16')
	str_list = []
	offset_list = []
	len_list = []
	scr_txt = open('code/' + f[:-4] + '.scr.txt','r',encoding='utf16')
	for rows in scr_txt:
		row = rows[1:].rstrip('\r\n').split('○')
		off = int(row[1].split('|')[0],16)
		slen = int(row[1].split('|')[1],16)
		offset_list.append(off)
		len_list.append(slen)
	for rows in txt:
		if rows[0] != '●':
			continue
		row = rows[1:].rstrip('\r\n').split('●')[1]
		str_list.append(row)
	dst = open('code/' + f[:-4] + '.enc','wb')
	dst.write(data)
	#有些文件文本块前会有一些多余字节，也要输出
	if offset_list[0] != 0:
		dst.write(fs.read(offset_list[0]))
	count = 0
	for row in str_list:
		length = len(row.encode('936')) - len_list[count]
		name_index_length += length
		uncomprlen += length
		for k in op_dict:
			#吔屎啦！！！这个op_dict的又不是从文本块开始算的，是从第一句文本开始，
			#所以要先减去第一个文本前的数据大小，这个offset_list[0]大部分情况下都为0
			#2023.01.04更新：对于start.ps3，反而是从文本块开始算，尴尬了，那就单独处理下，
			#不过offset_list[0]非0的情况只遇到过intproc.ps3、omake.ps3、start.ps3，只有start.ps3有例外
			#因为start.ps3的offset_list[0]为1，intproc.ps3、omake.ps3都大于1，所以两个判断方式都行。
			#if offset_list[0] == 1:
			if f == 'start.ps3.dec':
				if op_dict[k] > offset_list[count]:
					op_dict_new[k] = op_dict_new[k] + length
			else:
				if op_dict[k] > offset_list[count] - offset_list[0]:
					op_dict_new[k] = op_dict_new[k] + length
		count += 1
	for k in op_dict_new:
		dst.seek(k, os.SEEK_SET)
		dst.write(int2byte(op_dict_new[k]))
	dst.seek(0,os.SEEK_END)
	count = 0
	for row in str_list:
		dst.write(row.encode('936'))
		if count + 1 != len(str_list):
			dst.seek(offset_list[count + 1] - offset_list[count] - len_list[count], os.SEEK_CUR)
		count += 1
	dst.write(b'\x00')
	dst.seek(0x1C, os.SEEK_SET)
	dst.write(int2byte(name_index_length))
	dst.seek(0x28, os.SEEK_SET)
	dst.write(int2byte(uncomprlen))