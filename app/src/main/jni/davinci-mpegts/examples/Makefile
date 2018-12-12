CCOMP = clang
CFLAGS = -I ../
CLIBS = ../cgts_demux.c ../cgts_mux.c ../cgts_structs.c ../cgts_nal_adts_parse.c ../cgts_util.c

T01:
	$(CCOMP) example_remux.c   $(CLIBS) $(CFLAGS) -o cgts_remux
	$(CCOMP) example_analyse.c $(CLIBS) $(CFLAGS) -o cgts_analyse
	$(CCOMP) example_find_keyframe.c $(CLIBS) $(CFLAGS) -o cgts_find_keyframe
clean:
	rm -rfv output.ts cgts_remux cgts_analyse cgts_find_keyframe
analyse:
	./cgts_analyse ../test_samples/h264_aac.ts
remux:
	./cgts_remux ../test_samples/h264_aac.ts output.ts
find:
	./cgts_find_keyframe ../test_samples/h265_aac.ts
