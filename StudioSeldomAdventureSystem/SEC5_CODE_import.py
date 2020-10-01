# -*- coding:utf-8 -*-
#Niflheim-StudioSeldomAdventureSystem
#用于导入文本，请确认CODE文件在同一目录下
#by Darkness-TX 2020.06.28
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
jump_skip_new = [str_op_effect1_start_offset,str_op_effect1_end_offset,str_op_effect2_start_offset,str_op_effect2_end_offset,str_op_start_offset,str_op_end_offset]

org_list = []
str_list = []
script_list = []
jump_offset_old = []
jump_address_old = []
jump_offset_new = []
jump_address_new = []
jump_offset_info = []
jump_address_info = []
jump_address_dict = {}

def byte2int(byte):
	return struct.unpack('L',byte)[0]

def int2byte(byte):
	return struct.pack('L',byte)

def replace_note(line):
	# $<>$
	for j in range(0,line.count('$<'.encode('936'))):
		end = line.find('>$'.encode('936')) + len('>$'.encode('936'))
		start = line.find('$<'.encode('936'))
		block = line[start:end]
		if block.find('<['.encode('936')) != -1 and block.find('<{'.encode('936')) > block.find('<['.encode('936')):
			num = block.find('<['.encode('936')) - len('$<'.encode('936'))
		else:
			num = block.find('<{'.encode('936')) - len('$<'.encode('936'))
		# $<
		block = block.replace('$<'.encode('936'),note_op_start + int2byte(num + len(op_num_split) + 4 + 1) + op_num_split + int2byte(num))
		# >$
		block = block.replace('>$'.encode('936'),note_op_end)
		# <{}>
		for k in range(0,block.count('<{'.encode('936'))):
			dend = block.find('}>'.encode('936')) + len('}>'.encode('936'))
			dstart = block.find('<{'.encode('936'))
			dblock = block[dstart:dend]
			num = dblock.find('}>'.encode('936')) - dblock.find('<{'.encode('936')) - len('}>'.encode('936'))
			# <{
			dblock = dblock.replace('<{'.encode('936'),note_op + int2byte(num + len(op_num_split) + 4 + 1) + op_num_split + int2byte(num))
			# }>
			dblock = dblock.replace('}>'.encode('936'),b'')
			block = block[:dstart] + dblock + block[dend:]
		# <[]>
		for l in range(0,block.count('<['.encode('936'))):
			zend = block.find(']>'.encode('936')) + len(']>'.encode('936'))
			zstart = block.find('<['.encode('936'))
			zblock = block[zstart:zend]
			num = zblock.find(']>'.encode('936')) - zblock.find('<['.encode('936')) - len(']>'.encode('936'))
			# <[
			zblock = zblock.replace('<['.encode('936'),note_op_split + int2byte(num + len(op_num_split) + 4 + 1) + op_num_split + int2byte(num))
			# ]>
			zblock = zblock.replace(']>'.encode('936'),b'')
			block = block[:zstart] + zblock + block[zend:]
		line = line[:start] + block + line[end:]
	return line

def replace_line(line):
	# $$(\n)
	line = line.replace('$$(\\n)'.encode('936'),str_op_line_break)
	# ##[
	line = line.replace('##['.encode('936'),str_op_effect1_start)
	# ]##
	line = line.replace(']##'.encode('936'),str_op_effect1_end)
	# ##&[
	line = line.replace('##&['.encode('936'),str_op_effect2_start)
	# ]&##
	line = line.replace(']&##'.encode('936'),str_op_effect2_end)
	# $$&[
	line = line.replace('$$&['.encode('936'),str_op_effect3_start)
	# ]&$$
	line = line.replace(']&$$'.encode('936'),str_op_effect3_end)
	# $$[
	line = line.replace('$$['.encode('936'),str_op_effect4_start)
	# ]$$
	line = line.replace(']$$'.encode('936'),str_op_effect4_end)
	# note
	line = replace_note(line)
	return line

def change_jump(old,new,offset,offset_index,address_index):
	if old != new:
		num = new - old
		for i in range(offset_index, len(jump_offset_old)):
			if jump_offset_old[i] > offset:
				offset_index = i
				break
		for i in range(offset_index, len(jump_offset_old)):
			jump_offset_new[i] += num
		for i in range(address_index, len(jump_address_old)):
			if jump_address_old[i] > offset:
				address_index = i
				break
		for i in range(address_index, len(jump_address_old)):
			jump_address_new[i] += num
	return offset_index,address_index

def change_jump_skip(old,new,offset,offset_index):
	if old != new:
		num = new - old
		for i in range(offset_index, len(jump_skip)):
			if jump_skip[i] > offset:
				offset_index = i
				break
		for i in range(offset_index, len(jump_skip)):
			jump_skip_new[i] += num
	return offset_index

def build_opcode():
	offset_index = 0
	global str_op_effect1_start,str_op_effect1_end,str_op_effect2_start,str_op_effect2_end,str_op_start,str_op_end
	for i in range(0, len(script_list)):
		offset = int(script_list[i].split('|')[0],16)
		if offset > jump_skip[-1]:
			break
		str_num = int(script_list[i].split('|')[2])
		#选项
		if str_list[i].find('$s<') != -1:
			#line = replace_line(org_list[i].replace('$s<','').replace('>s$','').encode('932'))
			line = replace_line(str_list[i].replace('$s<','').replace('>s$','').encode('936'))
		#人名1
		elif str_list[i].find('$n1<') != -1:
			#line = replace_line(org_list[i].replace('$n1<','').replace('>n1$','').encode('932'))
			line = replace_line(str_list[i].replace('$n1<','').replace('>n1$','').encode('936'))
		#人名2
		elif str_list[i].find('$n2<') != -1:
			#line = replace_line(org_list[i].replace('$n2<','').replace('>n2$','').encode('932'))
			line = replace_line(str_list[i].replace('$n2<','').replace('>n2$','').encode('936'))
		#立绘动作人名
		elif str_list[i].find('$le<') != -1:
			#line = replace_line(org_list[i].replace('$le<','').replace('>le$','').encode('932'))
			line = replace_line(str_list[i].replace('$le<','').replace('>le$','').encode('936'))
		#人名3
		elif str_list[i].find('$n3<') != -1:
			#line = replace_line(org_list[i].replace('$n3<','').replace('>n3$','').encode('932'))
			line = replace_line(str_list[i].replace('$n3<','').replace('>n3$','').encode('936'))
		#人名4
		elif str_list[i].find('$n4<') != -1:
			#line = replace_line(org_list[i].replace('$n4<','').replace('>n4$','').encode('932'))
			line = replace_line(str_list[i].replace('$n4<','').replace('>n4$','').encode('936'))
		#人名5
		elif str_list[i].find('$n5<') != -1:
			#line = replace_line(org_list[i].replace('$n5<','').replace('>n5$','').encode('932'))
			line = replace_line(str_list[i].replace('$n5<','').replace('>n5$','').encode('936'))
		#其他人名
		elif str_list[i].find('$on<') != -1:
			#line = replace_line(org_list[i].replace('$on<','').replace('>on$','').encode('932'))
			line = replace_line(str_list[i].replace('$on<','').replace('>on$','').encode('936'))
		#控制文本框位置用人名
		elif str_list[i].find('$cn<') != -1:
			#line = replace_line(org_list[i].replace('$cn<','').replace('>cn$','').encode('932'))
			line = replace_line(str_list[i].replace('$cn<','').replace('>cn$','').encode('936'))
		#立绘效果用人名
		elif str_list[i].find('$en<') != -1:
			#line = replace_line(org_list[i].replace('$en<','').replace('>en$','').encode('932'))
			line = replace_line(str_list[i].replace('$en<','').replace('>en$','').encode('936'))
		#特殊用途人名
		elif str_list[i].find('$p<') != -1:
			#line = replace_line(org_list[i].replace('$p<','').replace('>p$','').encode('932'))
			line = replace_line(str_list[i].replace('$p<','').replace('>p$','').encode('936'))
		#章节名
		elif str_list[i].find('$c<') != -1:
			#line = replace_line(org_list[i].replace('$c<','').replace('>c$','').encode('932'))
			line = replace_line(str_list[i].replace('$c<','').replace('>c$','').encode('936'))
		else:
			#line = replace_line(org_list[i].encode('932'))
			line = replace_line(str_list[i].replace('♪','♂').encode('936'))
		offset_index = change_jump_skip(str_num,len(line),offset,offset_index)
	str_op_effect1_start_offset = jump_skip_new[0]
	str_op_effect1_end_offset = jump_skip_new[1]
	str_op_effect2_start_offset = jump_skip_new[2]
	str_op_effect2_end_offset = jump_skip_new[3]
	str_op_start_offset = jump_skip_new[4]
	str_op_end_offset = jump_skip_new[5]
	str_op_effect1_start = str_op_effect1_start[:9] + int2byte(str_op_effect1_start_offset) + str_op_effect1_start[-2:]
	str_op_effect1_end = str_op_effect1_end[:9] + int2byte(str_op_effect1_end_offset) + str_op_effect1_end[-2:]
	str_op_effect2_start = str_op_effect2_start[:9] + int2byte(str_op_effect2_start_offset) + str_op_effect2_start[-2:]
	str_op_effect2_end = str_op_effect2_end[:9] + int2byte(str_op_effect2_end_offset) + str_op_effect2_end[-2:]
	str_op_start = str_op_start[:9] + int2byte(str_op_start_offset) + str_op_start[-2:]
	str_op_end = str_op_end[:9] + int2byte(str_op_end_offset) + str_op_end[-2:]

def CODE_import():
	offset_index = 0
	address_index = 0
	jump = open('CODE_jump.txt','r',encoding='utf16')
	lines = jump.readlines()
	for i in range(0,len(lines)):
		jump_offset_old.append(int(lines[i][10:].rstrip('\r\n').split('|')[0],16))
		jump_offset_new.append(int(lines[i][10:].rstrip('\r\n').split('|')[0],16))
		jump_offset_info.append(lines[i][10:].rstrip('\r\n').split('|')[2])
		if int(lines[i][10:].rstrip('\r\n').split('|')[1],16) not in jump_address_dict:
			jump_address_dict[int(lines[i][10:].rstrip('\r\n').split('|')[1],16)] = str(i)
		else:
			jump_address_dict[int(lines[i][10:].rstrip('\r\n').split('|')[1],16)] = jump_address_dict[int(lines[i][10:].rstrip('\r\n').split('|')[1],16)] + '|' + str(i)
	for key in sorted(jump_address_dict.keys()):
		jump_address_old.append(key)
		jump_address_new.append(key)
		jump_address_info.append(jump_address_dict[key])
	#address = open('address.txt','w',encoding='utf16')
	#for key in sorted(jump_address_dict.keys()):
	#	address.write('%X|%d\n'%(key,len(jump_address_dict[key].split('|'))))
	script = open('CODE_script.txt','r',encoding='utf16')
	lines = script.readlines()
	for line in lines:
		script_list.append(line[10:].rstrip('\r\n'))
	txt = open('CODE_text.txt','r',encoding='utf16')
	lines = txt.readlines()
	for line in lines:
		if line[0] == '○':
			org_list.append(line[10:].rstrip('\r\n'))
		elif line[0] == '●':
			str_list.append(line[10:].rstrip('\r\n'))
	if len(script_list) != len(str_list):
		print('script与text行数不同！script:%d text:%d'%(len(script_list), len(str_list)))
		os.system('pause')
		exit(0)
	src = open('CODE','rb')
	data = src.read()
	dst = open('CODE.new','wb+')
	start = 0
	build_opcode()
	for i in range(0, len(script_list)):
		end = int(script_list[i].split('|')[0],16)
		dst.write(data[start:end])
		#选项
		if str_list[i].find('$s<') != -1:
			if str_list[i].count('$s<') != str_list[i].count('>s$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$s<','').replace('>s$','').encode('932'))
			line = replace_line(str_list[i].replace('$s<','').replace('>s$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_op_select) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_select + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		#人名1
		elif str_list[i].find('$n1<') != -1:
			if str_list[i].count('$n1<') != str_list[i].count('>n1$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$n1<','').replace('>n1$','').encode('932'))
			line = replace_line(str_list[i].replace('$n1<','').replace('>n1$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_op_name1) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_name1 + int2byte(len(line)) + line + buff
			#为$n2<>n2$做准备，重新记录新的$n1<>n1$块长度
			script_list[i] += '|' + '%d'%(len(line) + 4)
			dst.write(int2byte(len(line)))
			dst.write(line)
		#人名2
		elif str_list[i].find('$n2<') != -1:
			if str_list[i].count('$n2<') != str_list[i].count('>n2$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$n2<','').replace('>n2$','').encode('932'))
			line = replace_line(str_list[i].replace('$n2<','').replace('>n2$','').encode('936'))
			block_num = int(script_list[i].split('|')[1])
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + len(str_op_name2) + 4 + str_num:end + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_name2 + int2byte(len(line)) + line + buff
			dst.seek(-int(script_list[i - 1].split('|')[3]), os.SEEK_CUR)
			buff = dst.read(int(script_list[i - 1].split('|')[3]))
			dst.seek(-int(script_list[i - 1].split('|')[3]), os.SEEK_CUR)
			line = buff[4:] + line
			dst.write(int2byte(len(line)))
			dst.write(line)
		#人名3
		elif str_list[i].find('$n3<') != -1:
			if str_list[i].count('$n3<') != str_list[i].count('>n3$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$n3<','').replace('>n3$','').encode('932'))
			line = replace_line(str_list[i].replace('$n3<','').replace('>n3$','').encode('936'))
			block_num = int(script_list[i].split('|')[1])
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + len(str_op_name3)  + 4 + len(op_num_split) + 4 + str_num:end + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_name3 + int2byte(len(op_num_split) + 4 + len(line) + len(buff)) + op_num_split + int2byte(len(line)) + line + buff
			dst.write(line)
		#人名4
		elif str_list[i].find('$n4<') != -1:
			if str_list[i].count('$n4<') != str_list[i].count('>n4$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$n4<','').replace('>n4$','').encode('932'))
			line = replace_line(str_list[i].replace('$n4<','').replace('>n4$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_op_name4) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_name4 + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		#人名5
		elif str_list[i].find('$n5<') != -1:
			if str_list[i].count('$n5<') != str_list[i].count('>n5$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$n5<','').replace('>n5$','').encode('932'))
			line = replace_line(str_list[i].replace('$n5<','').replace('>n5$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_op_name5) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_name5 + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		#其他人名
		elif str_list[i].find('$on<') != -1:
			if str_list[i].count('$on<') != str_list[i].count('>on$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$on<','').replace('>on$','').encode('932'))
			line = replace_line(str_list[i].replace('$on<','').replace('>on$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + 5 + len(str_op_other_name) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = data[end + 4:end + 4 + 5] + str_op_other_name + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		#控制文本框位置用人名
		elif str_list[i].find('$cn<') != -1:
			if str_list[i].count('$cn<') != str_list[i].count('>cn$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$cn<','').replace('>cn$','').encode('932'))
			line = replace_line(str_list[i].replace('$cn<','').replace('>cn$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			num = byte2int(data[end + len(str_op_control):end + len(str_op_control) + 4])
			buff = data[end:end + len(str_op_control) + 4 + num + 1]
			dst.write(buff)
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = op_num_split + int2byte(len(line)) + line + b'\xFF'
			dst.write(int2byte(len(line)))
			dst.write(line)
		#立绘动作人名
		elif str_list[i].find('$le<') != -1:
			if str_list[i].count('$le<') != str_list[i].count('>le$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$le<','').replace('>le$','').encode('932'))
			line = replace_line(str_list[i].replace('$le<','').replace('>le$','').encode('936'))
			block_num = int(script_list[i].split('|')[1])
			str_num = int(script_list[i].split('|')[2])
			str_offset_start = data.find(str_op_ename,end)
			buff = data[end + 4:str_offset_start] + str_op_ename + int2byte(len(line)) + line
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = buff + data[end + 4 + (str_offset_start - end - 4) + len(str_op_ename) + 4 + str_num:end + 4 + block_num - 4]
			dst.write(int2byte(len(line)))
			dst.write(line)
		#立绘效果用人名
		elif str_list[i].find('$en<') != -1:
			if str_list[i].count('$en<') != str_list[i].count('>en$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$en<','').replace('>en$','').encode('932'))
			line = replace_line(str_list[i].replace('$en<','').replace('>en$','').encode('936'))
			block_num = int(script_list[i].split('|')[1])
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + len(str_op_effect)  + 4 + len(op_num_split) + 4 + str_num:end + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_op_effect + int2byte(len(op_num_split) + 4 + len(line) + len(buff)) + op_num_split + int2byte(len(line)) + line + buff
			dst.write(line)
		#特殊用途人名
		elif str_list[i].find('$p<') != -1:
			if str_list[i].count('$p<') != str_list[i].count('>p$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$p<','').replace('>p$','').encode('932'))
			line = replace_line(str_list[i].replace('$p<','').replace('>p$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_special_name) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_special_name + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		#章节名
		elif str_list[i].find('$c<') != -1:
			if str_list[i].count('$c<') != str_list[i].count('>c$'):
				print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
				os.system('pause')
				exit(0)
			#line = replace_line(org_list[i].replace('$c<','').replace('>c$','').encode('932'))
			line = replace_line(str_list[i].replace('$c<','').replace('>c$','').encode('936'))
			block_num = int(script_list[i].split('|')[1]) - 4
			str_num = int(script_list[i].split('|')[2])
			buff = data[end + 4 + len(str_chapter_name) + 4 + str_num:end + 4 + block_num]
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			line = str_chapter_name + int2byte(len(line)) + line + buff
			dst.write(int2byte(len(line)))
			dst.write(line)
		else:
			if str_list[i].find('$<') != -1:
				if str_list[i].count('$<') != str_list[i].count('>$'):
					print('编号%d行前后标识符不匹配！%s'%(i,str_list[i]))
					os.system('pause')
					exit(0)
			str_num = int(script_list[i].split('|')[2])
			#line = replace_line(org_list[i].encode('932'))
			line = replace_line(str_list[i].replace('♪','♂').encode('936'))
			#if i <= 754 or i >= 756:
			#	line = replace_line(org_list[i].encode('932'))
			#else:
			#	line = replace_line(str_list[i].replace('♪','♂').encode('936'))
			offset_index,address_index = change_jump(str_num,len(line),end,offset_index,address_index)
			dst.seek(-len(str_op_start),os.SEEK_CUR)
			dst.write(str_op_start)
			dst.write(line)
			dst.write(str_op_end)
			end += len(str_op_end)
		start = end + int(script_list[i].split('|')[1])
	dst.write(data[start:])
	jump_address_dict.clear()
	for i in range(0,len(jump_address_new)):
		for key in jump_address_info[i].split('|'):
			jump_address_dict[int(key)] = jump_address_new[i]
	for i in range(0,len(jump_offset_new)):
		dst.seek(jump_offset_new[i],os.SEEK_SET)
		dst.write(int2byte(jump_address_dict[i]))

def main():
	print('project：Niflheim-StudioSeldomAdventureSystem')
	print('用于导入文本，请确认CODE文件在同一目录下')
	print('by Darkness-TX 2020.06.28\n')
	st = time.clock()
	CODE_import()
	et = time.clock()
	print('insert all rows in %f s'%(et - st))

main()