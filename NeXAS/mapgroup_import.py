# -*- coding:utf-8 -*-

import struct
import os
import fnmatch

def walk(adr):
	mylist=[]
	for root,dirs,files in os.walk(adr):
		for name in files:
			if name[-5:] != '._grp':
				continue
			adrlist=os.path.join(root, name)
			mylist.append(adrlist)
	return mylist

def byte2int(byte):
	long_tuple=struct.unpack('L',byte)
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

def main():
	f_lst = walk('grp')
	tbl = open('tbl.txt','r',encoding='utf16')
	dicts = {}
	for rows in tbl:
		row = rows.rstrip('\r\n').split('=')
		if len(row) == 3:
			row[1] = '='
		dicts[row[1]]=int(row[0],16)
	for fn in f_lst:
		if fn != 'grp\\mapgroup._grp':
			continue
		src = open(fn, 'rb')
		dstname = fn[:-5] + '.txt'
		txt = open(dstname, 'r', encoding='utf16')
		filesize = os.path.getsize(fn)
		dst = open(fn[:-5]+'.grp','wb')
		str_list = []
		for rows in txt:
			if rows[0] != '●':
				continue
			row = txt.readline().rstrip('\r\n').replace('\\n','\n')
			str_list.append(row)
		i = 0
		while src.tell() < filesize:
			flag = byte2int(src.read(4))
			dst.write(int2byte(flag))
			if flag == 1:
				data = src.read(4)
				src.seek(-4,os.SEEK_CUR)
				if data[-1] == 0:
					continue
				else:
					for k in range(0,3):
						dumpstr(src)
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

		src.close()
		dst.close()
		txt.close()
main()