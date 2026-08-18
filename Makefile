SOURCES = ./src/main ./src/getWindow ./src/nstrUtils

EXE = ./bin/main

CFLAGS = -Wall

LDFLAGS = 
LDOBJ = -c

LIBS = -lgdi32 -luser32

LD = gcc

OBJECTS = $(SOURCES:%=%.o)
CFILE = $(SOURCES:%=%.c)


default: build

clean:
	-rm -f $(EXE)      # Remove the executable file
	-rm -f $(OBJECTS)  # Remove the object files


setup: $(CFILE)
	@for f in $(SOURCES); do \
		$(LD) $(LDOBJ) $$f.c -o $$f.o; \
	done

f_build:
	$(LD) $(LDFLAGS) $(CFILE) -o $(EXE) $(LIBS)

build: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(EXE) $(LIBS)
	.\$(EXE)

# results: main.c nstrUtils.c nstrUtils.h getWindow.c getWindow.h
#     gcc -i main.c nstrUtils.c getWindow.c -o results -lgdi32 -luser32