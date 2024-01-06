ABC := ../bin/xtest_parser

LDFLAGS := -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib \
	   -lSystem

# Uncomment to directly link with ld on MacOS

## LD := ld
## LINK.o = $(LD) $(ASFLAGS) $(LDFLAGS) $(TARGET_MACH)

src.c := $(wildcard *.c)
obj.o := $(src.c:%.c=%.o)
lib := $(obj.o)

src.abc := $(wildcard *.abc)
asm.abc := $(src.abc:%.abc=%s)
target := $(src.abc:%.abc=%)


.DEFAULT_GOAL := all

.PHONY: all
all: $(target)

%.s : %.abc
	$(ABC) -S $<

% : %.o
% : %.o $(lib)
	$(LINK.o) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm)

