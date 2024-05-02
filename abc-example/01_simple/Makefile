CC := ../../build/abc/abc
CFLAGS := -O3 -I ../include
DEPFLAGS = -MD -MF $(dep.dir)/$(<F).d \
	   -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

LDFLAGS += -L../lib 

dep.dir := dep
asm.dir := asm
obj.dir := obj

xsrc.abc := $(wildcard xtest_*.abc)
src.abc := $(filter-out $(xsrc.abc),$(wildcard *.abc))

asm.abc := $(src.abc:%.abc=$(asm.dir)/%.s)
obj.abc := $(src.abc:%.abc=$(obj.dir)/%.o)
xobj := $(xsrc.abc:%.abc=$(obj.dir)/%.o)
ll.abc := $(src.abc:%.abc=%.ll)
target := $(patsubst %.abc,%,$(xsrc.abc))

dep := $(src.abc:%=$(dep.dir)/%.d) \
       $(xsrc.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target) $(xobj)

$(asm.dir)/%.s :%.abc | $(asm.dir)
	$(CC) -S $(CFLAGS) $<

%.ll : %.abc
	$(CC) --emit-llvm $(CFLAGS) $<

$(obj.dir)/%.o : %.abc | $(obj.dir) $(dep.dir)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

x% : $(asm.dir)/x%.s
x% : $(obj.dir)/x%.o
x% : $(obj.dir)/x%.o $(obj.abc)
	$(LINK.o) $^ -o $@

$(dep.dir): ; mkdir -p $@
$(asm.dir): ; mkdir -p $@
$(obj.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target) $(asm.abc) $(obj.abc) $(bc.abc) $(lib) $(cpp.abc) $(dep)
	$(RM) -rf $(dep.dir) $(cpp.dir) $(asm.dir) $(obj.dir)

$(dep):
-include $(dep)

