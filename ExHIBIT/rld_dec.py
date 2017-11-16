# -*- coding:utf-8 -*-
#用于解密rld文件，def.rld用一组密钥，其他文件用另一组
#by Darkness-TX
#2017.11.16

import struct
import os
import sys
import io

def ReadKey(fname):
	keyfile = open(fname, 'rb')
	key_list = []
	#256组
	for i in range(0, 0x100):
		key_list.append(struct.unpack('L', keyfile.read(4))[0])
	return key_list

def main():
	other_key_list = ReadKey('key.bin')
	def_key_list = ReadKey('key_def.bin')
	other_xor_key = 0x16E854ED
	def_xor_key = 0xAE85A916
	for f in os.listdir('rld'):
		if not f.endswith('.rld'):
			continue
		print(f)
		src = open('rld/' + f,'rb')
		src.seek(0, os.SEEK_END)
		filesize = src.tell()
		src.seek(0, os.SEEK_SET)
		dst = open('rld/' + f[:-4] + '.bin','wb')
		dst.write(src.read(0x10))
		j = ((filesize - 0x10) >> 2) & 0xffff
		#最大只解密去掉文件头的前0xFFC0字节，后面的都没加密，原样输出即可
		if j > 0x3ff0:
			j = 0x3ff0
		if f[:-4] == 'def':
			key_list = def_key_list
			xor_key = def_xor_key
		else:
			key_list = other_key_list
			xor_key = other_xor_key
		for k in range(0, j):
			en_temp = struct.unpack('L', src.read(4))
			key_count = k
			key_count = key_count & 0xff
			temp_key = key_list[key_count] ^ xor_key
			de_temp = en_temp[0] ^ temp_key	
			dst.write(struct.pack('L', de_temp))
		#解密后的其他部分原样输出，除了上面注释的原因外，不可能每个文件的大小都能整除4的嘛，经测试最后那不足4字节的数据没加密
		dst.write(src.read(filesize - src.tell()))

main()