/* borrows from:
 * FFplay : Simple Media Player based on the ffmpeg libraries
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ff.h"
#include "draw.h"

FFStuff stuff;

void
initff(char *file){
	int i;

	av_register_all();

	// Open video file
	if(av_open_input_file(&stuff.formatctx, file, NULL, 0, NULL)!=0){
		fprintf(stderr, "cannot open file\n");
		exit(-2); // Couldn't open file
	}

	// Dump information about file onto standard error
	dump_format(stuff.formatctx, 0, file, 0);

	// Find the first video stream
	for(i=0; i<stuff.formatctx->nb_streams; i++){
		if(stuff.formatctx->streams[i]->codec.codec_type == CODEC_TYPE_VIDEO) {
			stuff.videoidx = i;
			break;
		}
	}
	if(i==stuff.formatctx->nb_streams){
		fprintf(stderr, "didn't find a video stream\n");
		exit(-3);
	}

	// Find the first audio stream
	for(i=0; i<stuff.formatctx->nb_streams; i++){
		if(stuff.formatctx->streams[i]->codec.codec_type == CODEC_TYPE_AUDIO) {
			stuff.audioidx = i;
			break;
		}
	}
	if(i==stuff.formatctx->nb_streams){
		fprintf(stderr, "didn't find an audio stream\n");
		exit(-3);
	}

	
}

void
ffdraw(void){
	AVPacket pkt1, *pkt = &pkt1;

	while(av_read_frame(stuff.formatctx, pkt) >= 0){
	}
}
