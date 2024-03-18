ABC := ../../build/abc/abc
ABCFLAGS := -I ../include
DEPFLAGS = -MD -MF $(dep.dir)/$(<F).d \
	   -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

src.c := $(wildcard *.c)
obj.o := $(src.c:%.c=%.o)
lib := $(obj.o)

dep.dir := dep
asm.dir := asm
obj.dir := obj

xsrc.abc := $(wildcard xtest_*.abc)
src.abc := $(filter-out $(xsrc.abc),$(wildcard *.abc))

asm.abc := $(src.abc:%.abc=$(asm.dir)/%.s)
obj.abc := $(src.abc:%.abc=$(obj.dir)/%.o)
xobj := $(xsrc.abc:%.abc=$(obj.dir)/%.o)
bc.abc := $(src.abc:%.abc=%.bc)
target := $(patsubst %.abc,%,$(xsrc.abc))

dep := $(src.abc:%=$(dep.dir)/%.d) \
       $(xsrc.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target) $(xobj)

$(asm.dir)/%.s :%.abc | $(asm.dir)
	$(ABC) -S $(ABCFLAGS) $<

%.bc : %.abc
	$(ABC) -B $(ABCFLAGS) $<

$(obj.dir)/%.o : %.abc | $(obj.dir) $(dep.dir)
	$(ABC) -c $(ABCFLAGS) $(DEPFLAGS) $< -o $@

x% : $(asm.dir)/x%.s
x% : $(obj.dir)/x%.o
x% : $(obj.dir)/x%.o $(obj.abc)
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

