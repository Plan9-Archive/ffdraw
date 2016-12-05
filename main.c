#include <stdio.h>
#include "draw.h"
#include "memdraw.h"
#include "ff.h"

int main(int argc, char **argv) {
	initff(argv[1]);
	initdraw(0,0,"ffdraw");
	ffdraw();
}
