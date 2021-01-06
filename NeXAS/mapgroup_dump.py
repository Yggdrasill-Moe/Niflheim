# -*- coding:utf-8 -*-
import struct,os,fnmatch,re

def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

def byte2int(byte):
    long_tuple=struct.unpack('L',byte)
    long = long_tuple[0]
    return long

def int2byte(num):
    return struct.pack('L',num)

def FormatString(string, count):
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    
    #res = "○%08d○%s○\n%s●%08d●%s●\n%s\n"%(count, name, string, count, name, string)
    
    
    res = "○%08d○\n%s\n●%08d●\n%s\n\n"%(count, string, count, string)
    
    return res

def dumpstr(src):
    bstr = b''
    c = src.read(1)
    while c != b'\x00':
        bstr += c
        c = src.read(1)
    return bstr.decode('932')

def main():
    f_lst = walk('grp')
    for fn in f_lst:
        if fn != 'grp\\mapgroup._grp':
            continue
        src = open(fn, 'rb')
        dstname = fn[:-5] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')
        filesize = os.path.getsize(fn)
        str_list = []
        while src.tell() < filesize:
            flag = byte2int(src.read(4))
            if flag == 1:
                data = src.read(4)
                src.seek(-4,os.SEEK_CUR)
                if data[-1] == 0:
                    continue
                else:
                    str_list.append(dumpstr(src))
                    str_list.append(dumpstr(src))
                    str_list.append(dumpstr(src))
        i = 0
        for string in str_list:
            dst.write(FormatString(string, i))
            i += 1
        src.close()
        dst.close()

main()