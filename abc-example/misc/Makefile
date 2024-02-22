ABC := ../../bin/abc
ABCFLAGS := -O3 -I ../include
CPP := $(CC) -E -x c -traditional -Wno-endif-labels -I../include
CPPFLAGS += -MT $(@:%.abc_p=%.o) -MMD -MP -MF $(dep.dir)/$(<F).d

# Uncomment to directly link with ld on MacOS

## LDFLAGS := -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib \
## 	   -lSystem
## LD := ld
## LINK.o = $(LD) $(ASFLAGS) $(LDFLAGS) $(TARGET_MACH)

src.c := $(wildcard *.c)
obj.o := $(src.c:%.c=%.o)
lib := $(obj.o)
dep.dir := dep

src.abc := $(wildcard *.abc)
cpp.abc := $(src.abc:%.abc=%.abc_p)
asm.abc := $(src.abc:%.abc=%.s)
obj.abc := $(src.abc:%.abc=%.o)
bc.abc := $(src.abc:%.abc=%.bc)
target := $(src.abc:%.abc=%)

dep := $(src.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target) $(obj.abc)

# %.abc_p : %.abc | $(dep.dir)
%.abc_p : %.abc
	$(CPP) $(CPPFLAGS) -o $@ $< 

#%.s : %.abc_p
%.s : %.abc
	$(ABC) -S $(ABCFLAGS) $<

%.bc : %.abc_p
	$(ABC) -B $<

%.o : %.s
	$(AS) -o $@ $<

% : %.s
% : %.o
% : %.o $(lib)
	$(LINK.o) $^ -o $@

$(dep.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm.abc) $(obj.abc) $(bc.abc) $(lib) $(cpp.abc) $(dep)

$(dep):
-include $(dep)

