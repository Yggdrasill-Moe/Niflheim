# Niflheim-cmvs
用于处理cmvs引擎
## [Base on]
CPZ7_unpack、CPZ6_unpack、pb32png:参考garbro源码

ps3_decoder:参考crass源码

## [Note]
#### [文本]
先ps3_decoder.exe解密后cmvs_textdump.py导出，导入直接用cmvs_textimport.py，最后ps3_encoder.exe压缩回去，以防万一以后可能还要祭出Longinus1.4，就不把ps3_encoder.exe的功能集成到cmvs_textimport.py了。

~~因为Longinus1.4的cmvs文本hook点是解压后的，所以文本方面就没有再压缩，但是如果想要改的是data\pack\start.ps3，则需要单独再使用ps3_encoder.exe压缩回去。~~
#### [图片]
多数为pb3文件，有部分分支为类jpeg算法压缩，所以是有损压缩，虽然转换可以强制锁定到某些非类jpeg的算法分支，但是写伪压缩没啥必要，因为：
```
from ChronoClock
int __thiscall sub_42F2C0(void *this, int *a2, char *data, SIZE_T uBytes, int a5, const CHAR *a6)
{
  int flag;
  int result;
  
  flag = 0;
  result = 0;
  if ( data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47 )// PNG
	flag = 3;
  if ( data[0] == 0x42 && data[1] == 0x4D )// BMP
	flag = 2;
  if ( data[0] == 0x50 && data[1] == 0x42 && data[2] == 0x33 && data[3] == 0x42 )// PB3
	flag = 1;
  if ( data[0] == 0x4D && data[1] == 0x53 && data[2] == 0x4B && data[3] == 0x30 )// MSK
	flag = 4;
  switch ( flag )
  {
	case 1:
	  result = ReadPB3((int)this, a2, data, uBytes, a5, a6);
	  break;
	case 2:
	  result = ReadBMP(a2, data, uBytes);
	  break;
	case 3:
	  result = ReadPNG(a2, (int)data, uBytes);
	  break;
	case 4:
	  result = ReadMSK(a2, data, uBytes);
	  break;
	default:
	  return result;
  }
  return result;
}
```
所以直接转成png改后缀为pb3就行，大小上比原始pb3要小，就是要牺牲点读取速度了，而真正的png2pb3等以后有兴趣了再写。

~~偷了个懒直接用的Longinus1.4，直接可以读png2pb3转换的，其实只是抽出了rgba信息，挂羊头卖狗肉~~
#### [补丁]
直接封包，如果制作增量补丁就用CPZ6_make.exe，但是start.ps3也要增加新增量封包的信息，使用cmvs_start_patch.py。

~~暂未写完封包程序，只能用Longinus1.4的封包功能~~
## [New]
ver 1.2

增加CPZ7封包方面功能，基于【アオイトリ】制作

ver 1.1

增加CPZ6封包方面功能，基于【クロノクロック】制作

ver 1.0

完成，基于【アオイトリ】制作