# Niflheim-cmvs
用于处理cmvs引擎
## [Base on]
CPZ7_unpack、CPZ6_unpack、pb32png:参考garbro源码
ps3_decoder:参考crass源码

## [Note]
#### [文本]
先ps3_decoder.exe解密后cmvs_textdump.py导出，导入直接用cmvs_textimport.py，因为Longinus1.4的cmvs文本hook点是解压后的，所以文本方面就没有再压缩，但是如果想要改的是data\pack\start.ps3，则需要单独再使用ps3_encoder.exe压缩回去。
#### [图片]
偷了个懒直接用的Longinus1.4，直接可以读png2pb3转换的，其实只是抽出了rgba信息，挂羊头卖狗肉
#### [补丁]
暂未写完封包程序，只能用Longinus1.4的封包功能
## [New]
ver 1.0

完成，基于【アオイトリ】制作