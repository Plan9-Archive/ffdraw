#include <stdio.h>
#include "draw.h"
#include "ff.h"

int main(int argc, char **argv) {
	initdraw(0,0,"ffdraw");
	initff(argv[1]);
	ffdraw();
}
