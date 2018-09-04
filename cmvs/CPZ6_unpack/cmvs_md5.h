typedef struct {
	unsigned int buffer[16];
	unsigned int state[4];
} cmvs_md5_ctx;

void cmvs_md5(unsigned int data[], cmvs_md5_ctx *ctx);