all: pearldecoder pearlquick

pearldecoder: pearldecoder.c
	$(CC) $(CCFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi

pearlquick: pearlquick.c
	$(CC) $(CCFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi
	
clean:
	$(RM) *.o pearldecoder pearlquick
