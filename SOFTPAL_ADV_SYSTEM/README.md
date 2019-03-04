# Niflheim-SOFTPAL_ADV_SYSTEM
用于处理SOFTPAL_ADV_SYSTEM引擎
## [Base on]
fuckpac、pac_text.py:https://github.com/Inori/FuckGalEngine/tree/master/PAC

pgd2png、png2pgd:crass源码
## [New]
ver 0.95

fuckfont新增freetype2生成

建议使用此模式（-fi）生成，原始模式（-i）模式无法准确计算生成字模的长宽

ver 0.9

增加制作封包功能但有些字节不明白其意义，

所以这些部分全补零，不建议使用这功能

ver 0.7

支持处理字库，配合Longinus可实现自制字库与API的显示方式切换

添加日文码表，不过E000开始后似乎有错但不影响汉化，

因为用不上那么多字

ver 0.2

支持文本脚本简单导入导出，可实现超长汉化

ver 0.1

支持魔女こいにっき

图片仅支持32或24位PGD3类型

文本脚本、字库未解决