#include <Windows.h>

static DWORD gyu_new_uncompress(BYTE *uncompr, BYTE *compr)
{
	BYTE *org_uncompr = uncompr;
	BYTE flag = 0;
	BYTE flag_bits = 1;

	compr += 4;
	*uncompr++ = *compr++;
	while (1) {
		--flag_bits;
		if (!flag_bits) {
			flag = *compr++;
			flag_bits = 8;
		}

		int b = flag & 0x80;
		flag <<= 1;

		if (b)
			*uncompr++ = *compr++;
		else {
			--flag_bits;
			if (!flag_bits) {
				flag = *compr++;
				flag_bits = 8;
			}

			DWORD count, offset;

			b = flag & 0x80;
			flag <<= 1;
			if (!b) {
				--flag_bits;
				if (!flag_bits) {
					flag = *compr++;
					flag_bits = 8;
				}

				count = (!!(flag & 0x80)) * 2;
				flag <<= 1;
				--flag_bits;
				if (!flag_bits) {
					flag = *compr++;
					flag_bits = 8;
				}
				count += (!!(flag & 0x80)) + 1;
				offset = *compr++ - 256;
				flag <<= 1;
			}
			else {
				offset = compr[1] | (compr[0] << 8);
				compr += 2;
				count = offset & 7;
				offset = (offset >> 3) - 8192;
				if (count)
					++count;
				else {
					count = *compr++;
					if (!count)
						break;
				}
			}

			++count;
			BYTE *src = uncompr + offset;
			for (DWORD i = 0; i < count; ++i)
				*uncompr++ = *src++;
		}
	}

	return uncompr - org_uncompr;
}