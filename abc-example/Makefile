ABC := ../bin/abc_compiler
ABCFLAGS := -O3

# Uncomment to directly link with ld on MacOS

## LDFLAGS := -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib \
## 	   -lSystem
## LD := ld
## LINK.o = $(LD) $(ASFLAGS) $(LDFLAGS) $(TARGET_MACH)

src.c := $(wildcard *.c)
obj.o := $(src.c:%.c=%.o)
lib := $(obj.o)

src.abc := $(wildcard *.abc)
asm.abc := $(src.abc:%.abc=%.s)
bc.abc := $(src.abc:%.abc=%.bc)
target := $(src.abc:%.abc=%)


.DEFAULT_GOAL := all

.PHONY: all
all: $(target)


.PHONY: all
bc: $(bc.abc)

.PHONY: all
asm: $(asm.abc)

%.s : %.abc
	$(ABC) -S $(ABCFLAGS) $<

%.bc : %.abc
	$(ABC) -B $<

% : %.s
% : %.o
% : %.o $(lib)
	$(LINK.o) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm.abc) $(bc.abc) $(lib)

