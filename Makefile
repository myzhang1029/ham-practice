# Enable SDL:
# $ make ENABLE_SDL=1 CFLAGS="-Ofast -Wall -Wextra $(pkg-config --cflags SDL2_image)" LDFLAGS="$(pkg-config --libs SDL2_image)"

.PHONY: clean

dotest: dotest.o pcg.o
	$(CC) -o $@ $^ $(LDFLAGS)

pcg.o: pcg.c pcg.h
	$(CC) -c $(CFLAGS) -o $@ $<

dotest.o: dotest.c images_gperf.h images.h questions.h pcg.h
	$(CC) -c $(CFLAGS) -DENABLE_SDL=$(ENABLE_SDL) -o $@ $<

images_gperf.h: images.gperf
	gperf --null-strings -ntCIK filename -L ANSI-C -F ", NULL, 0" --output-file=$@ $<

images.h.interm: images
	ls $< | xargs -I{} xxd -i $</{} > $@

# Convert to const
images.h: images.h.interm
	sed 's/^unsigned char/const unsigned char/;s/^unsigned int/const unsigned int/' $< > $@

images.lst: images
	ls $< > $@

images.gperf: images.lst
	echo "struct image_info_t { const char * const filename; const void * const content; const size_t length;};" > $@
	echo "%%" >> $@
	python3 -c "open('$@', 'a').write(''.join(f'{(y:=x.strip())},{(j:=(\"images_\"+y.replace(\"-\",\"_\").replace(\".\",\"_\")))},{j}_len\n' for x in open('$<')))"

questions.h: questions.json gen_data_header.py
	./gen_data_header.py $< $@

clean:
	rm -f images_gperf.h images.h images.h.interm images.lst images.gperf questions.h dotest *.o
