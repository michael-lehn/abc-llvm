CC := abc
CFLAGS += -O3 
DEPFLAGS += -MD -MF $(dep.dir)/$(<F).d \
	    -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

LDFLAGS += 

dep.dir := dep
obj.dir := obj

xsrc.abc := $(wildcard xtest_*.abc)
src.abc := $(filter-out $(xsrc.abc),$(wildcard *.abc))

obj.abc := $(src.abc:%.abc=$(obj.dir)/%.o)
xobj := $(xsrc.abc:%.abc=$(obj.dir)/%.o)
ll.abc := $(src.abc:%.abc=%.ll)
target := $(patsubst %.abc,%,$(xsrc.abc))

dep := $(src.abc:%=$(dep.dir)/%.d) \
       $(xsrc.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target) $(xobj)

%.ll : %.abc
	$(CC) --emit-llvm $(CFLAGS) $<

$(obj.dir)/%.o : %.abc | $(obj.dir) $(dep.dir)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

x% : $(obj.dir)/x%.o
x% : $(obj.dir)/x%.o $(obj.abc)
	$(LINK.o) $^ -o $@

$(dep.dir): ; mkdir -p $@
$(obj.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target)
	$(RM) -rf $(dep.dir) $(obj.dir)

$(dep):
-include $(dep)

