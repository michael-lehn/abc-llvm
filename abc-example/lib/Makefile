ABC := ../../build/abc/abc
ABCFLAGS := -I ../include
DEPFLAGS = -MD -MF $(dep.dir)/$(<F).d \
	   -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

dep.dir := dep
obj.dir := obj

src.abc := $(wildcard *.abc)
obj.abc := $(src.abc:%.abc=$(obj.dir)/%.o)
target := libabc.a

dep := $(src.abc:%=$(dep.dir)/%.d) \
       $(xsrc.abc:%=$(dep.dir)/%.d)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target)

%.a : $(obj.abc)
	ar rcs $@ $^

$(obj.dir)/%.o : %.abc | $(obj.dir) $(dep.dir)
	$(ABC) -c $(ABCFLAGS) $(DEPFLAGS) $< -o $@

$(dep.dir): ; mkdir -p $@
$(obj.dir): ; mkdir -p $@

.PHONY: clean
clean:
	$(RM) $(target) $(obj.abc)
	$(RM) -rf $(dep.dir) $(obj.dir)

$(dep):
-include $(dep)

