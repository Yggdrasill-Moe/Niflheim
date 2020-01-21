# -*- coding:utf-8 -*-

import struct
import os
import sys
import io

def walk(adr):
	mylist=[]
	for root,dirs,files in os.walk(adr):
		for name in files:
			if name[-5:] != '._dat':
				continue
			adrlist=os.path.join(root, name)
			mylist.append(adrlist)
	return mylist

def byte2int(byte):
	long_tuple=struct.unpack('L',byte)
	long = long_tuple[0]
	return long

def dumpstr(src):
	bstr = b''
	c = src.read(1)
	while c != b'\x00':
		bstr += c
		c = src.read(1)
	return bstr.decode('932')

def int2byte(num):
	return struct.pack('L',num)

def main():
	f_lst = walk('Config')
	tbl = open('tbl.txt','r',encoding='utf16')
	dicts = {}
	for rows in tbl:
		row = rows.rstrip('\r\n').split('=')
		if len(row) == 3:
			row[1] = '='
		dicts[row[1]]=int(row[0],16)
	for fn in f_lst:
		fs = open(fn, 'rb')
		dstname = fn[:-5] + '.txt'
		txt = open(dstname, 'r', encoding='utf16')
		filesize=os.path.getsize(fn)
		dst = open(fn[:-5]+'.dat','wb')
		count = byte2int(fs.read(4))
		types=[byte2int(fs.read(4)) for i in range(count)]
		dst.write(int2byte(count))
		for t in types:
			dst.write(int2byte(t))
		str_list = []
		for rows in txt:
			if rows[0] != '●':
				continue
			row = txt.readline().rstrip('\r\n').replace('\\n','\n')
			str_list.append(row)
		i = 0
		while fs.tell() < filesize:
			for t in types:
				if t == 2:
					dst.write(fs.read(4))
				elif t == 1:
					l = dumpstr(fs)
					if len(l) != 0:
						try:
							for ch in str_list[i]:
								if ch == '\n':
									dst.write(b'\x0A')
								elif dicts[ch] > 0xFF:
									dst.write(struct.pack('>H',dicts[ch]))
								else:
									dst.write(struct.pack('B',dicts[ch]))
						except Exception as inst:
							print(str_list[i])
						dst.write(struct.pack('B',0))
						i += 1
					else:
						dst.write(struct.pack('B',0))
		fs.close()
		dst.close()
		txt.close()
main()