# -*- coding:utf-8 -*-
#用于将增量封包信息写入start.ps3，只启动图片和文本类别。
#by Darkness-TX 2023.01.04
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

def main():
	fs = open('start.ps3.enc','rb')
	data = fs.read()
	#0x0120020f
	#0x01200201
	op_text_off = []
	text_off = []
	change_num = []
	op_jump_off = []
	jump_off = []
	off = 0
	while data.find(b'\x01\x02\x20\x01',off) != -1:
		off = data.find(b'\x01\x02\x20\x01',off) + 4
		op_text_off.append(off - 4)
		text_off.append(byte2int(data[off:off + 4]))
	off = 0	
	while data.find(b'\x0F\x02\x01\x04',off) != -1:
		off = data.find(b'\x0F\x02\x01\x04',off) + 4
		op_jump_off.append(off - 4)
		jump_off.append(byte2int(data[off:off + 4]))
	off = 0
	while data.find(b'\x99\x20\x00\x04',off) != -1:
		off = data.find(b'\x99\x20\x00\x04',off) + 4
		op_jump_off.append(off - 4)
		jump_off.append(byte2int(data[off:off + 4]))
	off = 0
	while data.find(b'\x00\x20\x00\x04',off) != -1:
		off = data.find(b'\x00\x20\x00\x04',off) + 4
		op_jump_off.append(off - 4)
		jump_off.append(byte2int(data[off:off + 4]))
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
	text_num = 0
	pic_num = 0
	for i in range(0, len(op_text_off)):
		#文本
		if data[op_text_off[i] + 8 + 16] == 0x10 and data[op_text_off[i] + 8 + 16 + 1] == 0x20:
			change_num.append(i)
			if byte2int(data[op_text_off[i] + 8 + 8:op_text_off[i] + 8 + 12]) == 0:
				text_num = i
		#图片
		elif data[op_text_off[i] + 8 + 16] == 0x11 and data[op_text_off[i] + 8 + 16 + 1] == 0x20:
			change_num.append(i)
			if byte2int(data[op_text_off[i] + 8 + 8:op_text_off[i] + 8 + 12]) == 0:
				pic_num = i
	#文本会在图片前面，如果不是那下面的字节流重组都要调转处理顺序
	if pic_num < text_num:
		print(text_num,pic_num)
		os.system('pause')
		exit(0)
	fs.seek(0, os.SEEK_SET)
	data = fs.read(unknown1_count * 4 + unknown2_length + header_length)
	dst = open('start.ps3.patch','wb+')
	opcode_length = 0x1A
	unknown2_length += opcode_length * 2
	uncomprlen += opcode_length * 2
	text_stream = b'\x01\x02\x20\x01\x00\x00\x00\x00\x0F\x02\x30\x04\x00\x02\x00\x01\x00\x00\x00\x00\x0F\x02\x30\x04\x10\x20'
	pic_stream = b'\x01\x02\x20\x01\x00\x00\x00\x00\x0F\x02\x30\x04\x00\x02\x00\x01\x00\x00\x00\x00\x0F\x02\x30\x04\x11\x20'
	data = data[:op_text_off[text_num]] + text_stream + data[op_text_off[text_num]:]
	data = data[:op_text_off[pic_num] + opcode_length] + pic_stream + data[op_text_off[pic_num] + opcode_length:]
	dst.write(data)
	dst.seek(0x14, os.SEEK_SET)
	dst.write(int2byte(unknown2_length))
	for num in change_num:
		if num < pic_num:
			dst.seek(op_text_off[num] + 0x10 + opcode_length,os.SEEK_SET)
			block_num = byte2int(dst.read(4))
			block_num += 1
			dst.seek(-4,os.SEEK_CUR)
			dst.write(int2byte(block_num))
		else:
			dst.seek(op_text_off[num] + 0x10 + opcode_length * 2,os.SEEK_SET)
			block_num = byte2int(dst.read(4))
			block_num += 1
			dst.seek(-4,os.SEEK_CUR)
			dst.write(int2byte(block_num))
	for i in range(text_num, len(op_text_off)):
		if i < pic_num:
			op_text_off[i] += opcode_length
		else:
			op_text_off[i] += opcode_length * 2
	data = fs.read()
	dst.seek(0, os.SEEK_END)
	patch_path = 'data\\pack\\' + sys.argv[1]
	patch_length = len(patch_path.encode('936')) + 1
	data = data[:text_off[text_num]] + patch_path.encode('936') + b'\x00' + data[text_off[text_num]:]
	data = data[:text_off[pic_num] + patch_length] + patch_path.encode('936') + b'\x00' + data[text_off[pic_num] + patch_length:]
	dst.write(data)
	name_index_length += patch_length * 2
	dst.seek(0x1C, os.SEEK_SET)
	dst.write(int2byte(name_index_length))
	uncomprlen += patch_length * 2
	dst.seek(0x28, os.SEEK_SET)
	dst.write(int2byte(uncomprlen))
	#写文本信息
	for i in range(text_num, len(text_off)):
		if i < pic_num:
			text_off[i] += patch_length
		else:
			text_off[i] += patch_length * 2
	for i in range(0, len(op_text_off)):
		dst.seek(op_text_off[i] + 4, os.SEEK_SET)
		dst.write(int2byte(text_off[i]))
	#写jump信息
	for i in range(0, len(op_jump_off)):
		if op_jump_off[i] > op_text_off[text_num] - opcode_length:
			op_jump_off[i] += opcode_length
	for i in range(0, len(op_jump_off)):
		if op_jump_off[i] > op_text_off[pic_num] - opcode_length * 2:
			op_jump_off[i] += opcode_length
	for i in range(0, len(jump_off)):
		if jump_off[i] > op_text_off[text_num] - opcode_length:
			jump_off[i] += opcode_length
	for i in range(0, len(jump_off)):
		if jump_off[i] > op_text_off[pic_num] - opcode_length * 2:
			jump_off[i] += opcode_length
	for i in range(0, len(op_jump_off)):
		dst.seek(op_jump_off[i] + 4, os.SEEK_SET)
		dst.write(int2byte(jump_off[i]))
	#写新增信息
	dst.seek(op_text_off[text_num] - opcode_length + 4, os.SEEK_SET)
	dst.write(int2byte(text_off[text_num] - patch_length))
	dst.seek(op_text_off[pic_num] - opcode_length + 4, os.SEEK_SET)
	dst.write(int2byte(text_off[pic_num] - patch_length))

main()