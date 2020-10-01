# -*- coding:utf-8 -*-
#Niflheim-StudioSeldomAdventureSystem
#用于导出文本，请确认CODE文件在同一目录下
#by Darkness-TX 2020.06.08
import os
import struct
import sys
import io
import time

#文本起始符
str_op_start = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\xE3\x50\x01\x00\xFF\xFF'
#文本终止符，与文本起始配套
str_op_end = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\x66\x58\x01\x00\xFF\xFF'
#选项 $s<>s$ 前4字节记录整块长度
str_op_select = b'\x20\x6C\x00\x00\x00\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#人名 $n1<>n1$ 前4字节记录整块长度，如果有$n2<>n2$，则长度包含$n2<>n2$
str_op_name1 = b'\x20\x71\x00\x00\x00\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#人名 $n2<>n2$ 长度包括在$n1<>n1$的块里
str_op_name2 = b'\x24\x13\x01\x3C\x50\x13\x03\x24\x1E\x00\x00\x00\x00'
#立绘动作人名 $le<>le$ 长度包括在上级块里
str_op_ename = b'\x24\x13\x01\x3C\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#与人名1配套 $n3<>n3$ 其中一个用处是控制多人说同一句话时对话框四散在画面上
str_op_name3 = b'\x1B\x82\x02\x00'
#与控制文本框位置用人名及立绘效果用人名配套 $n4<>n4$
str_op_name4 = b'\x20\x79\x00\x00\x00\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#与人名1配套 $n5<>n5$ 其中一个用处是控制对话框跟随人物在画面中的位置
str_op_name5 = b'\x20\x77\x00\x00\x00\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#其他人名 $on<>on$
str_op_other_name = b'\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#控制文本框位置用人名 $cn<>cn$
str_op_control = b'\x1B\x86\x02\x00'
#立绘效果用人名 $en<>en$
str_op_effect = b'\x1B\x80\x02\x00'
#特殊用途人名 $p<>p$ 前4字节记录整块长度(FastForwardOffAtSelection、MsgColorsByName、InterpolatePicture)其中MsgColorsByName为文字颜色根据人物变化功能，可在游戏设置中开启
str_special_name = b'\x2A\x02\x13\x01\x24\x3A\x1E\x00\x00\x00\x00'
#章节名 $c<>c$ 前4字节记录整块长度
str_chapter_name = b'\x20\x24\x00\x00\x00\x50\x23\x50\x13\x01\x24\x1E\x00\x00\x00\x00'
#换行 $$(\n)
str_op_line_break = b'\x1B\xF8\x01\xFF'
#分隔符
op_num_split = b'\x1E\x00\x00\x00\x00'

#放大
#效果1起始符 ##[
str_op_effect1_start = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\x8A\x09\x00\x00\xFF\xFF'
#效果1终止符 ]##
str_op_effect1_end = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\xA9\x09\x00\x00\xFF\xFF'
#放大
#效果2起始符 ##&[
str_op_effect2_start = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\xC0\x09\x00\x00\xFF\xFF'
#效果2终止符 ]&##
str_op_effect2_end = b'\x1B\x03\x00\x01\x06\x00\x00\x00\x19\xDF\x09\x00\x00\xFF\xFF'
#加粗
#效果3起始符 $$&[
str_op_effect3_start = b'\x1B\x06\x02\xFF'
#效果3终止符 ]&$$
str_op_effect3_end = b'\x1B\x07\x02\xFF'
#缩小
#效果4起始符 $$[
str_op_effect4_start = b'\x1B\x03\x02\xFF'
#效果4终止符 ]$$
str_op_effect4_end = b'\x1B\x02\x02\xFF'

#注释开头 $<
note_op_start = b'\x1B\xF9\x01\x01'
#需要分块注释的大文字 <[]>
note_op_split = b'\xFF\x01'
#注释文字 <{}>
note_op = b'\xFF\x02'
#注释结尾 >$
note_op_end = b'\xFF\xFF'

#跳转地址
jump_op_1B450000 = b'\x1B\x45\x00\x00'#类型1

jump_op_1B020001 = b'\x1B\x02\x00\x01'#类型1
jump_op_1B030001 = b'\x1B\x03\x00\x01'#类型1
jump_op_1B020000 = b'\x1B\x02\x00\x00'#类型2
jump_op_1B030000 = b'\x1B\x03\x00\x00'#类型2

jump_op_1BC00101 = b'\x1B\xC0\x01\x01'#类型1
jump_op_1BC10101 = b'\x1B\xC1\x01\x01'#类型1
jump_op_1BC20101 = b'\x1B\xC2\x01\x01'#类型1
jump_op_1BC00100 = b'\x1B\xC0\x01\x00'#类型2
jump_op_1BC00103 = b'\x1B\xC0\x01\x03'#类型2

#需要跳过的硬编码地址
str_op_effect1_start_offset = 0x98A
str_op_effect1_end_offset = 0x9A9
str_op_effect2_start_offset = 0x9C0
str_op_effect2_end_offset = 0x9DF
str_op_start_offset = 0x150E3
str_op_end_offset = 0x15866
jump_skip = [str_op_effect1_start_offset,str_op_effect1_end_offset,str_op_effect2_start_offset,str_op_effect2_end_offset,str_op_start_offset,str_op_end_offset]

byte_list = []
str_list = []
script_list = []
jump_dict = {}
byte_off = {}
check_list = []

def FormatString(string, count):
	res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
	return res

def byte2int(byte):
	return struct.unpack('L',byte)[0]

def find_start(data, offset = 0):
	return data.find(str_op_start, offset)

def find_end(data, offset = 0):
	return data.find(str_op_end, offset)

def find_select(data):
	str_offset_start = data.find(str_op_select)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_op_select)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$s<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>s$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_select, str_offset_end)

def find_name1(data):
	str_offset_start = data.find(str_op_name1)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_op_name1)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$n1<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>n1$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_name1, str_offset_end)

def find_name2(data):
	str_offset_start = data.find(str_op_name2)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start
		str_offset_start += len(str_op_name2)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$n2<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>n2$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, 'n2', num)
		str_offset_start = data.find(str_op_name2, str_offset_end)

def find_name3(data):
	str_offset_start = data.find(str_op_name3)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start
		str_offset_start += len(str_op_name3)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		str_offset_start2 = str_offset_start + 4 + len(op_num_split)
		num = byte2int(data[str_offset_start2:str_offset_start2 + 4])
		byte_list.append('$n3<'.encode('932') + data[str_offset_start2 + 4:str_offset_start2 + 4 + num] + '>n3$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_offset_start:str_offset_start + 4]) + 4 + 4, num)
		str_offset_start = data.find(str_op_name3, str_offset_end)

def find_name4(data):
	str_offset_start = data.find(str_op_name4)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_op_name4)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$n4<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>n4$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_name4, str_offset_end)

def find_name5(data):
	str_offset_start = data.find(str_op_name5)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_op_name5)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$n5<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>n5$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_name5, str_offset_end)

def find_other_name(data):
	str_offset_start = data.find(str_op_other_name)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 5 - 4
		str_offset_start += len(str_op_other_name)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		if num != 0 and str_opcode_start not in byte_off:
			byte_list.append('$on<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>on$'.encode('932'))
			byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_other_name, str_offset_end)

def find_control(data):
	str_offset_start = data.find(str_op_control)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start
		str_offset_start += len(str_op_control)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		if data[str_offset_end] != 0x01:
			print('标识符错误offset:0x%X opcode:0x%X'%(str_offset_end,data[str_offset_end]))
			os.system('pause')
			exit(0)
		str_offset_start2 = str_offset_end + 1 + 4 + len(op_num_split)
		num = byte2int(data[str_offset_start2:str_offset_start2 + 4])
		str_offset_end2 = str_offset_start2 + 4 + num
		byte_list.append('$cn<'.encode('932') + data[str_offset_start2 + 4:str_offset_end2] + '>cn$'.encode('932'))
		block_num = len(str_op_control) + 4 + byte2int(data[str_offset_start:str_offset_start + 4]) + 1 + 4 + byte2int(data[str_offset_end + 1:str_offset_end + 1 + 4])
		byte_off[str_opcode_start] = (len(byte_list) - 1, block_num, num)
		str_offset_start = data.find(str_op_control, str_offset_end)

def find_ename(data):
	str_offset_start = data.find(str_op_ename)
	while str_offset_start != -1:
		str_opcode_start = data.rfind(b'\x1B\x12\x00\x01',0,str_offset_start) + 4
		str_offset_start += len(str_op_ename)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$le<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>le$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_op_ename, str_offset_end)

def find_effect(data):
	str_offset_start = data.find(str_op_effect)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start
		str_offset_start += len(str_op_effect)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		str_offset_start2 = str_offset_start + 4 + len(op_num_split)
		num = byte2int(data[str_offset_start2:str_offset_start2 + 4])
		byte_list.append('$en<'.encode('932') + data[str_offset_start2 + 4:str_offset_start2 + 4 + num] + '>en$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_offset_start:str_offset_start + 4]) + 4 + 4, num)
		str_offset_start = data.find(str_op_effect, str_offset_end)

def find_special_name(data):
	str_offset_start = data.find(str_special_name)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_special_name)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$p<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>p$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_special_name, str_offset_end)

def find_chapter(data):
	str_offset_start = data.find(str_chapter_name)
	while str_offset_start != -1:
		str_opcode_start = str_offset_start - 4
		str_offset_start += len(str_chapter_name)
		num = byte2int(data[str_offset_start:str_offset_start + 4])
		str_offset_end = str_offset_start + 4 + num
		byte_list.append('$c<'.encode('932') + data[str_offset_start + 4:str_offset_end] + '>c$'.encode('932'))
		byte_off[str_opcode_start] = (len(byte_list) - 1, byte2int(data[str_opcode_start:str_opcode_start + 4]) + 4, num)
		str_offset_start = data.find(str_chapter_name, str_offset_end)

def replace_note(line):
	note_offset_start = line.find(note_op_start)
	while note_offset_start != -1:
		note_offset_end = line.find(note_op_end)
		if note_offset_end == -1:
			print('存在注释起始符，但未找到注释终止符，此行文本可能存在错误！ note_offset_start：0x%X\n%s'%(note_offset_start,str(line)))
			os.system('pause')
			exit(0)
		note = line[note_offset_start + len(note_op_start) + 4 + 9:note_offset_end]
		note_offset_end += len(note_op_end)
		note_offset_split = note.find(note_op_split)
		#需要分块注释的大文字 @[]@
		while note_offset_split != -1:
			num = byte2int(note[note_offset_split + len(note_op_split) + 9:note_offset_split + len(note_op_split) + 9 + 4])
			note = note[:note_offset_split] + '<['.encode('932') + note[note_offset_split + len(note_op_split) + 9 + 4:note_offset_split + len(note_op_split) + 9 + 4 + num] + ']>'.encode('932') + note[note_offset_split + len(note_op_split) + 9 + 4 + num:]
			note_offset_split = note.find(note_op_split)
		note_offset = note.find(note_op)
		#注释文字 @{}@
		while note_offset != -1:
			num = byte2int(note[note_offset + len(note_op) + 9:note_offset + len(note_op) + 9 + 4])
			note = note[:note_offset] + '<{'.encode('932') + note[note_offset + len(note_op) + 9 + 4:note_offset + len(note_op) + 9 + 4 + num] + '}>'.encode('932') + note[note_offset + len(note_op) + 9 + 4 + num:]
			note_offset = note.find(note_op)
		line = line[:note_offset_start] + '$<'.encode('932') + note + '>$'.encode('932') + line[note_offset_end:]
		note_offset_start = line.find(note_op_start)
	return line

def replace_line(line):
	# $$(\n)
	line = line.replace(str_op_line_break,'$$(\\n)'.encode('932'))
	# ##[
	line = line.replace(str_op_effect1_start,'##['.encode('932'))
	# ]##
	line = line.replace(str_op_effect1_end,']##'.encode('932'))
	# ##&[
	line = line.replace(str_op_effect2_start,'##&['.encode('932'))
	# ]&##
	line = line.replace(str_op_effect2_end,']&##'.encode('932'))
	# $$&[
	line = line.replace(str_op_effect3_start,'$$&['.encode('932'))
	# ]&$$
	line = line.replace(str_op_effect3_end,']&$$'.encode('932'))
	# $$[
	line = line.replace(str_op_effect4_start,'$$['.encode('932'))
	# ]$$
	line = line.replace(str_op_effect4_end,']$$'.encode('932'))
	# note
	line = replace_note(line)
	return line

def find_text(data):
	str_offset_start = find_start(data)
	if str_offset_start == -1:
		print('未能找到文本起始符！')
		os.system('pause')
		exit(0)
	while str_offset_start != -1:
		str_offset_start += len(str_op_start)
		str_offset_end = find_end(data, str_offset_start)
		if str_offset_end == -1:
			print('存在文本起始符，但未找到文本终止符，此文件可能存在错误！ str_offset_start：0x%X'%(str_offset_start))
			os.system('pause')
			exit(0)
		byte_list.append(data[str_offset_start:str_offset_end])
		byte_off[str_offset_start] = (len(byte_list) - 1, str_offset_end - str_offset_start, str_offset_end - str_offset_start)
		str_offset_end += len(str_op_end)
		str_offset_start = find_start(data, str_offset_end)

def find_jump(data):
	#1B450000
	jump_offset_start = data.find(jump_op_1B450000)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1B450000):jump_offset_start + len(jump_op_1B450000) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1B450000) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1B450000) + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + len(jump_op_1B450000) + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1B450000'%(jump_offset_start + len(jump_op_1B450000) + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + len(jump_op_1B450000) + 4 + 1] = '%X|1B450000'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1B450000, jump_offset_end)
	#1B020001
	jump_offset_start = data.find(jump_op_1B020001)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1B020001):jump_offset_start + len(jump_op_1B020001) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1B020001) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1B020001) + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + len(jump_op_1B020001) + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1B020001'%(jump_offset_start + len(jump_op_1B020001) + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + len(jump_op_1B020001) + 4 + 1] = '%X|1B020001'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1B020001, jump_offset_end)
	#1B030001
	jump_offset_start = data.find(jump_op_1B030001)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1B030001):jump_offset_start + len(jump_op_1B030001) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1B030001) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1B030001) + 4:jump_offset_end]
		if buff[0] == 0x19:
			#排除掉需要跳过的硬编码地址
			if byte2int(buff[1:-1]) not in jump_skip:
				if jump_offset_start + len(jump_op_1B030001) + 4 + 1 in jump_dict:
					print('发现重复的跳转地址！%X|%X|1B030001'%(jump_offset_start + len(jump_op_1B030001) + 4 + 1, byte2int(buff[1:-1])))
					os.system('pause')
					exit(0)
				jump_dict[jump_offset_start + len(jump_op_1B030001) + 4 + 1] = '%X|1B030001'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1B030001, jump_offset_end)
	#1B020000
	jump_offset_start = data.find(jump_op_1B020000)
	while jump_offset_start != -1:
		jump_op_start = jump_offset_start
		num = byte2int(data[jump_offset_start + len(jump_op_1B020000):jump_offset_start + len(jump_op_1B020000) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1B020000) + 4 + num
		while data[jump_offset_end] != 1:
			jump_offset_start = jump_offset_end + 1
			num = byte2int(data[jump_offset_start:jump_offset_start + 4])
			jump_offset_end = jump_offset_start + 4 + num
		jump_offset_start = jump_offset_end + 1
		num = byte2int(data[jump_offset_start:jump_offset_start + 4])
		jump_offset_end = jump_offset_start + 4 + num
		buff = data[jump_offset_start + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1B020000'%(jump_offset_start + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + 4 + 1] = '%X|1B020000'%(byte2int(buff[1:-1]))
		else:
			print('未知的opcode！opcode:0x%X off_start:0x%X'%(data[jump_offset_end],jump_op_start))
			os.system('pause')
			exit(0)
		jump_offset_start = data.find(jump_op_1B020000, jump_offset_end)
	#1B030000
	jump_offset_start = data.find(jump_op_1B030000)
	while jump_offset_start != -1:
		jump_op_start = jump_offset_start
		num = byte2int(data[jump_offset_start + len(jump_op_1B030000):jump_offset_start + len(jump_op_1B030000) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1B030000) + 4 + num
		while data[jump_offset_end] != 1:
			jump_offset_start = jump_offset_end + 1
			num = byte2int(data[jump_offset_start:jump_offset_start + 4])
			jump_offset_end = jump_offset_start + 4 + num
		jump_offset_start = jump_offset_end + 1
		num = byte2int(data[jump_offset_start:jump_offset_start + 4])
		jump_offset_end = jump_offset_start + 4 + num
		buff = data[jump_offset_start + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1B030000'%(jump_offset_start + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + 4 + 1] = '%X|1B030000'%(byte2int(buff[1:-1]))
		else:
			print('未知的opcode！opcode:0x%X off_start:0x%X'%(data[jump_offset_start + 4],jump_op_start))
			os.system('pause')
			exit(0)
		jump_offset_start = data.find(jump_op_1B030000, jump_offset_end)
	#1BC00101
	jump_offset_start = data.find(jump_op_1BC00101)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1BC00101):jump_offset_start + len(jump_op_1BC00101) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1BC00101) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1BC00101) + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + len(jump_op_1BC00101) + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1BC00101'%(jump_offset_start + len(jump_op_1BC00101) + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + len(jump_op_1BC00101) + 4 + 1] = '%X|1BC00101'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1BC00101, jump_offset_end)
	#1BC10101
	jump_offset_start = data.find(jump_op_1BC10101)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1BC10101):jump_offset_start + len(jump_op_1BC10101) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1BC10101) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1BC10101) + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + len(jump_op_1BC10101) + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1BC10101'%(jump_offset_start + len(jump_op_1BC10101) + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + len(jump_op_1BC10101) + 4 + 1] = '%X|1BC10101'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1BC10101, jump_offset_end)
	#1BC20101
	jump_offset_start = data.find(jump_op_1BC20101)
	while jump_offset_start != -1:
		num = byte2int(data[jump_offset_start + len(jump_op_1BC20101):jump_offset_start + len(jump_op_1BC20101) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1BC20101) + 4 + num
		buff = data[jump_offset_start + len(jump_op_1BC20101) + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + len(jump_op_1BC20101) + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1BC20101'%(jump_offset_start + len(jump_op_1BC20101) + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + len(jump_op_1BC20101) + 4 + 1] = '%X|1BC20101'%(byte2int(buff[1:-1]))
		jump_offset_start = data.find(jump_op_1BC20101, jump_offset_end)
	#1BC00100
	jump_offset_start = data.find(jump_op_1BC00100)
	while jump_offset_start != -1:
		jump_op_start = jump_offset_start
		num = byte2int(data[jump_offset_start + len(jump_op_1BC00100):jump_offset_start + len(jump_op_1BC00100) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1BC00100) + 4 + num
		while data[jump_offset_end] != 1:
			jump_offset_start = jump_offset_end + 1
			num = byte2int(data[jump_offset_start:jump_offset_start + 4])
			jump_offset_end = jump_offset_start + 4 + num
		jump_offset_start = jump_offset_end + 1
		num = byte2int(data[jump_offset_start:jump_offset_start + 4])
		jump_offset_end = jump_offset_start + 4 + num
		buff = data[jump_offset_start + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1BC00100'%(jump_offset_start + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + 4 + 1] = '%X|1BC00100'%(byte2int(buff[1:-1]))
		else:
			print('未知的opcode！opcode:0x%X off_start:0x%X'%(data[jump_offset_start + 4],jump_op_start))
			os.system('pause')
			exit(0)
		jump_offset_start = data.find(jump_op_1BC00100, jump_offset_end)
	#1BC00103
	jump_offset_start = data.find(jump_op_1BC00103)
	while jump_offset_start != -1:
		jump_op_start = jump_offset_start
		num = byte2int(data[jump_offset_start + len(jump_op_1BC00103):jump_offset_start + len(jump_op_1BC00103) + 4])
		jump_offset_end = jump_offset_start + len(jump_op_1BC00103) + 4 + num
		while data[jump_offset_end] != 1:
			jump_offset_start = jump_offset_end + 1
			num = byte2int(data[jump_offset_start:jump_offset_start + 4])
			jump_offset_end = jump_offset_start + 4 + num
		jump_offset_start = jump_offset_end + 1
		num = byte2int(data[jump_offset_start:jump_offset_start + 4])
		jump_offset_end = jump_offset_start + 4 + num
		buff = data[jump_offset_start + 4:jump_offset_end]
		if buff[0] == 0x19:
			if jump_offset_start + 4 + 1 in jump_dict:
				print('发现重复的跳转地址！%X|%X|1BC00101'%(jump_offset_start + 4 + 1, byte2int(buff[1:-1])))
				os.system('pause')
				exit(0)
			jump_dict[jump_offset_start + 4 + 1] = '%X|1BC00101'%(byte2int(buff[1:-1]))
		else:
			print('未知的opcode！opcode:0x%X off_start:0x%X'%(data[jump_offset_start + 4],jump_op_start))
			os.system('pause')
			exit(0)
		jump_offset_start = data.find(jump_op_1BC00103, jump_offset_end)

def check_jump(data):
	jump_op = b'\x06\x00\x00\x00\x19'
	jump_offset_start = data.find(jump_op)
	while jump_offset_start != -1:
		jump_offset_start += 5
		jump_offset_end = jump_offset_start + 4
		if jump_offset_start not in jump_dict and byte2int(data[jump_offset_start:jump_offset_end]) not in jump_skip:
			print('未发现的jump_opcode！off_start:0x%X value:0x%X'%(jump_offset_start, byte2int(data[jump_offset_start:jump_offset_end])))
		jump_offset_start = data.find(jump_op, jump_offset_end)

def find_jump_plus(data):
	filesize = len(data)
	#0x19为跳转符
	jump_offset_start = data.find(b'\x19')
	while jump_offset_start != -1:
		jump_offset_start += 1
		jump_offset_end = jump_offset_start + 4
		jump_offset = byte2int(data[jump_offset_start:jump_offset_end])
		if jump_offset < filesize and data[jump_offset] == 27 and jump_offset not in jump_skip:#0x1B
			jump_dict[jump_offset_start] = '%X|19'%(jump_offset)
		jump_offset_start = data.find(b'\x19', jump_offset_end)

def CODE_dump():
	src = open('CODE','rb')
	data = src.read()
	find_text(data)
	find_select(data)
	find_name1(data)
	find_name2(data)
	find_name3(data)
	find_name4(data)
	find_name5(data)
	find_control(data)
	find_ename(data)
	find_effect(data)
	find_special_name(data)
	find_chapter(data)
	find_other_name(data)
	#find_jump(data)
	#check_jump(data)
	find_jump_plus(data)
	for i in sorted(byte_off.keys()):
		str_list.append(replace_line(byte_list[byte_off[i][0]]).decode('932'))
		#$n2<>n2$的长度包含在了$n1<>n1$里，所以先记录下来，然后再重新计算$n1<>n1$和$n2<>n2$分别的长度
		if byte_off[i][1] == 'n2':
			#计算$n2<>n2$的长度，$n2<>n2$必然跟在$n1<>n1$后面，所以取script_list最后一位即可
			script_list.append('%X|%d|%d'%(i, int(script_list[-1].split('|')[1]) - (i - int(script_list[-1].split('|')[0], 16)), byte_off[i][2]))
			#重新计算$n1<>n1$的长度
			script_list[-2] = script_list[-2].split('|')[0] + '|' + '%d'%(int(script_list[-1].split('|')[0], 16) - int(script_list[-2].split('|')[0], 16)) + '|' + script_list[-2].split('|')[2]
		else:
			script_list.append('%X|%d|%d'%(i, byte_off[i][1], byte_off[i][2]))
	dst = open('CODE_text.txt','w',encoding='utf16')
	i = 0
	for line in str_list:
		dst.write(FormatString(line,i))
		i += 1
	script = open('CODE_script.txt','w',encoding='utf16')
	i = 0
	for line in script_list:
		script.write("○%08d○%s\n"%(i, line))
		i += 1
	jump = open('CODE_jump.txt','w',encoding='utf16')
	i = 0
	for key in sorted(jump_dict.keys()):
		line = '%X|%s'%(key,jump_dict[key])
		jump.write("○%08d○%s\n"%(i, line))
		i += 1

def main():
	print('project：Niflheim-StudioSeldomAdventureSystem')
	print('用于导出文本，请确认CODE文件在同一目录下')
	print('by Darkness-TX 2020.06.08\n')
	CODE_dump()

main()