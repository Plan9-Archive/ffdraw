CFLAGS=-g
LDFLAGS=-lavformat -lavcodec -lz -lm -ldts
OBJS=\
	main.o\
	draw.o\
	memdraw.o\
	cmap.o\
	ff.o\

ffdraw: $(OBJS)
	$(CC) -o ffdraw $(OBJS) $(LDFLAGS)

cmap.o: mkcmap_run
	$(CC) -c cmap.c

mkcmap_run: mkcmap
	./mkcmap > cmap.c

mkcmap: mkcmap.o
	$(CC) -o mkcmap mkcmap.o draw.o

clean:
	rm -f *.o ffdraw mkcmap
