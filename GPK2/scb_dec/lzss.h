#include <Windows.h>

static DWORD lzss_decompress(BYTE *uncompr, DWORD uncomprlen,BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte >= comprlen)
				break;
		}
		if (flag & 1) {
			unsigned char data;

			if (act_uncomprlen >= uncomprlen)
				break;
			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		}
		else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes >> 4) << 8;
			copy_bytes = copy_bytes & 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {
				unsigned char data;

				if (act_uncomprlen >= uncomprlen)
					break;
				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			}
		}
	}
	return act_uncomprlen;
}