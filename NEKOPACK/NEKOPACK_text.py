# -*- coding:utf-8 -*-
#project：Niflheim-NEKOPACK
#用于导出NEKOPACK引擎的txt文件的纯文本
#by Destinyの火狐 2021.06.02
import os
import struct
import io
import sys

def help():
	print('usage:NEKOPACK_text [-d|-i] txt_folder')
	print('\t-d\tdump text from txt_folder')
	print('\t-i\tpack text from txt_folder and txt_folder_dump')

def walk(adr):
	mylist = []
	for root,dirs,files in os.walk(adr):
		for name in files:
			if name[-4:] != '.txt':
				continue
			adrlist=os.path.join(root, name)
			mylist.append(adrlist)
	return mylist

def FormatString(string, count):
	res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
	return res

def dump():
	dirname = sys.argv[2]
	f_lst = walk(dirname)
	if len(f_lst) != 0:
		if os.path.exists(dirname + '_dump') == False:
			os.makedirs(dirname + '_dump')
	for fn in f_lst:
		print(fn)
		src = open(fn,'r',encoding='932')
		str_list = []
		for row in src.readlines():
			row = row.rstrip('\r\n')
			if len(row) != 0:
				if len(row.replace('\t','').replace(' ','')) != 0 and row.replace('\t','').replace(' ','')[0] not in ['@','\t',';','.','?','#']:
					if row[0] == ':':
						str_list.append(row.split(':')[1])
					else:
						str_list.append(row)
		if len(str_list) != 0:
			dst = open(fn.replace(dirname,dirname + '_dump'),'w',encoding='utf16')
			i = 0
			for string in str_list:
				dst.write(FormatString(string, i))
				i += 1

def pack():
	dirname = sys.argv[2]
	f_lst = walk(dirname)
	if len(f_lst) != 0:
		if os.path.exists(dirname + '_dump') == False:
			print('无法找到%s目录！'%(dirname + '_dump'))
			os.system('pause')
			exit()
		if os.path.exists(dirname + '_new') == False:
			os.makedirs(dirname + '_new')
		for fn in f_lst:
			if os.path.exists(fn.replace(dirname,dirname + '_dump')) == False:
				continue
			print(fn)
			src = open(fn,'r',encoding='932') 
			txt = open(fn.replace(dirname,dirname + '_dump'),'r',encoding='utf16')
			str_list = []
			for row in txt.readlines():
				if row[0] != '●':
					continue
				str_list.append(row[10:].rstrip('\r\n'))
			dst = open(fn.replace(dirname,dirname + '_new'),'wb')
			i = 0
			for row in src.readlines():
				if len(row.rstrip('\r\n')) != 0:
					if len(row.replace('\t','').replace(' ','').rstrip('\r\n')) != 0 and row.replace('\t','').replace(' ','')[0] not in ['@','\t',';','.','?','#']:
						if row[0] == ':':
							temp = row.split(':')
							temp[1] = str_list[i].replace('・','·').replace('♪','～')
							dst.write(':'.join(temp).encode('936'))
							i += 1
						else:
							dst.write((str_list[i].replace('・','·').replace('♪','～') + '\n').encode('936'))
							i += 1
					else:
						dst.write(row.encode('932'))
				else:
					dst.write(row.encode('932'))

def main():
	print('project：Niflheim-NEKOPACK')
	print('用于导出NEKOPACK引擎的txt文件的纯文本')
	print('by Destinyの火狐 2021.06.02\n')
	if len(sys.argv) != 3:
		help()
	else:
		if sys.argv[1] == '-d':
			dump()
		elif sys.argv[1] == '-i':
			pack()
		else:
			help()
main()