# Niflheim-NeXAS
用于处理NeXAS引擎，针对BALDR HEART
## [Base on]
giga_text_dump.py:https://github.com/Inori/FuckGalEngine/blob/master/GIGA/giga_text_out.py

dat_dump.py:https://github.com/regomne/chinesize/blob/master/NeXAs/exTxtFromDat.py

pac_unpack、pac_pack:crass源码
## [Note]
#### [字库]
将fnt_make_bold_ft.exe、fnt_make_ft.exe、fnt_make.ini、libpng16.dll、tbl_chs.txt和三个makefnt的bat放在同一个文件夹，新建fnt、UpdateCHS文件夹。

将fnt_build.exe、fnt_dec.exe、libpng16.dll、zlib1.dll和全部原始fnt文件放入fnt文件夹里，双击makefnt_all.bat即可。

如果不想封包，可以去掉makefnt_all.bat的最后一行“pac_pack.exe UpdateCHS”，不然自行添加pac_pack.exe相关exe和dll。

字体用的思源黑体和宋体（tutorial用的宋体），也可以自行更改fnt_make.ini用自己喜欢的字体，我个人觉得思源的字体效果不完美......。

tutorial如果想还原原字体效果，需要在fnt_make_bold_ft.exe生成png后再用ps录制一层描边再批处理，效果为RGB（255,255,255），透明度50%，1像素。

字体对应.txt为找到的fnt在游戏中使用到的地方，并非所有fnt都会使用，NeXAS_fnt_Shift-JIS.txt为所有fnt的编码对应，tbl_chs.txt用于生成中文字库使用的码表，可自行更改，本项目中使用的tbl.txt和tbl_chs.txt都为针对BALDR HEART制作。

最好都用freetype2版的程序（有ft标志），非freetype2的程序后续并没有开发，因为用Windows API生成的效果巨 屎 无 比，所以抛弃了，仅供参考。
#### [游戏主程序]
用的Themida壳，脱壳就顺带也破解了.....，根据dump的方法不同，可能需要自行修复一下IAT表，一般会少了D3DXAssembleShader、D3DXQuaternionMultiply，exe中有非常多文本，想完美汉化不得不脱壳。
#### [文本]
bin中除了导出程序导出的外还会有点文本，一般就一两句话会用到，测试的时候就知道了。

dat为系统文本，mek为机体文本，grp有一些文本，看会不会用到，除了bin和dat外，mek和grp都需要具体文件具体分析。

但是都没有文本长度标识，所有其实可以暴力导出然后按原地址塞回去，再进行超长或缩短后的处理。

tbl.txt为BALDR HEART的汉化码表，更改文本会影响到游戏读取图片文件的文件名，所以使用在GetGlyphOutline中进行编码转换的方式实现汉化，即可保证汉化和原版存档共存，又能保持更改文本对文件读取影响不大，也不会出现乱码。
## [New]
ver 1.0

可解决BALDR HEART汉化