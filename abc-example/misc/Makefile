CC := ../../build/abc/abc
CFLAGS := -O3 -I ../include
DEPFLAGS = -MD -MF $(dep.dir)/$(<F).d \
           -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

LDFLAGS += -L../lib -labc

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
asm.abc := $(src.abc:%.abc=%.s)
obj.abc := $(src.abc:%.abc=%.o)
ll.abc := $(src.abc:%.abc=%.ll)
target := $(src.abc:%.abc=%)

dep := $(src.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target) $(obj.abc)

%.s : %.abc
	$(CC) -S $(CFLAGS) $<

%.ll : %.abc
	$(CC) --emit-llvm $(CFLAGS) $<

%.o : %.abc
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

%.o : %.s
	$(AS) -o $@ $<

% : %.s
% : %.o
% : %.o $(lib)
	$(LINK.o) $^ -o $@

# % : %.abc $(lib)
# 	$(LINK.c)  $^ -o $@

$(dep.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm.abc) $(obj.abc) $(ll.abc) $(lib) $(cpp.abc) $(dep)

$(dep):
-include $(dep)

