# -*- coding:utf-8 -*-

import struct
import os
import sys
import io

nameo = open('nameo.txt','r',encoding='utf-8-sig')
namen = open('namen.txt','r',encoding='utf-8-sig')
tbl = open('tbl.txt','r',encoding='utf16')
dicts = {}
for rows in tbl:
	row = rows.rstrip('\r\n').split('=')
	dicts[row[1]]=int(row[0],16)
name_dict = {}
for row in nameo:
	old = '【' + row.strip().split(' ')[2] + '】'
	new = '【' + namen.readline().strip().split(' ')[2] + '】'
	name_dict[old] = new

if os.path.exists('build') == False:
	os.mkdir('build')
for f in os.listdir('BR_TC'):
	if not f.endswith('.txt'):
		continue
	print(f)
	src = open('BR_TC/'+f,'r',encoding='utf-8-sig')
	data = src.readlines()
	dst = open('build/'+f,'wb')
	for rows in data:
		row = rows.replace('⇒','→').replace('♪','♂').replace('♥','！').replace('⑪','').replace('⑫','').replace('・','·').replace('ﾀ','').replace('ﾌ','').replace('ｯ','').replace('ﾞ','').replace('ﾗ','').replace('ﾝ','').replace('ｺ','')
		if row[0] == '【':
			for i in name_dict:
				if row.find(i) != -1:
					row = row.replace(i,name_dict[i])
					#print(row)
					#os.system('pause')
					break
		for ch in row:
			try:
				if ch == '\n':
					dst.write(b'\x0d\x0a')
				elif ch == '\t':
					dst.write(b'\x09')
				elif dicts[ch] > 0xFF:
					dst.write(struct.pack('>H',dicts[ch]))
				else:
					dst.write(struct.pack('B',dicts[ch]))
			except Exception as inst:
				print(ch)
				print(row)
		#dst.write(row.encode('936').decode('936'))
