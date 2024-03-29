ABC := ../../build/abc/abc
ABCFLAGS := -I ../include
CPP := $(CC) -E -x c -traditional -Wno-endif-labels -I../include
CPPFLAGS += \
	    -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o)\
	    -MMD -MP -MF $(dep.dir)/$(<F).d

# Uncomment to directly link with ld on MacOS

## LDFLAGS := -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib \
## 	   -lSystem
## LD := ld
## LINK.o = $(LD) $(ASFLAGS) $(LDFLAGS) $(TARGET_MACH)

src.c := $(wildcard *.c)
obj.o := $(src.c:%.c=%.o)
lib := $(obj.o)

dep.dir := dep
cpp.dir := cpp
asm.dir := asm
obj.dir := obj

xsrc.abc := $(wildcard xtest_*.abc)
src.abc := $(filter-out $(xsrc.abc),$(wildcard *.abc))

cpp.abc := $(src.abc:%.abc=$(cpp.dir)/%.abc_cpp)
asm.abc := $(src.abc:%.abc=$(asm.dir)/%.s)
obj.abc := $(src.abc:%.abc=$(obj.dir)/%.o)
xobj := $(xsrc.abc:$(src.dir)/%.cpp=$(obj.dir)/%.o)
bc.abc := $(src.abc:%.abc=%.bc)
target := $(patsubst %.abc,%,$(xsrc.abc))

dep := $(src.abc:%=$(dep.dir)/%.d) \
       $(xsrc.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target)

# %.abc_p : %.abc | $(dep.dir)
$(cpp.dir)/%.abc_cpp : %.abc | $(cpp.dir)
	$(CPP) $(CPPFLAGS) -o $@ $< 

$(asm.dir)/%.s :%.abc | $(asm.dir)
	$(ABC) -S $(ABCFLAGS) $<
	@mv $(@F) $@

%.bc : %.abc
	$(ABC) -B $(ABCFLAGS) $<

$(obj.dir)/%.o : %.abc | $(obj.dir)
	$(ABC) -c $(ABCFLAGS) $< -o $@

% : $(asm.dir)/%.s
% : $(obj.dir)/%.o
% : $(obj.dir)/%.o $(obj.abc)
	$(LINK.o) $^ -o $@

$(dep.dir): ; mkdir -p $@
$(cpp.dir): ; mkdir -p $@
$(asm.dir): ; mkdir -p $@
$(obj.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm.abc) $(obj.abc) $(bc.abc) $(lib) $(cpp.abc) $(dep)
	$(RM) -rf $(dep.dir) $(cpp.dir) $(asm.dir) $(obj.dir)

$(dep):
-include $(dep)

