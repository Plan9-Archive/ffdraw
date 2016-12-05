#include <ffmpeg/avformat.h>

void initff(char*);
void ffdraw(void);

typedef struct FFStuff {
	AVFormatContext *formatctx;
	int videoidx;
	int audioidx;
} FFStuff;
