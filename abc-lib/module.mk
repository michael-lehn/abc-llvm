ABC := $(build.dir)/abc/abc

abc-lib.name := abc
abc-lib.dir := abc-lib/
abc-lib.lib := $(build.dir)$(abc-lib.dir)lib$(abc-lib.name).a

targets += $(abc-lib.lib)

ABCFLAGS := -I abc-include
DEPFLAGS = -MD -MP -MT '$@'

$(abc-lib.name).lib.abc := \
	$(wildcard $(abc-lib.dir)/*.abc)
$(abc-lib.name).lib.obj := \
	$(patsubst %,$(build.dir)%,\
		$($(abc-lib.name).lib.abc:%.abc=%.o))
dep.files += \
	$(patsubst %,$(build.dir)%,\
		$($(abc-lib.name).lib.abc:%.abc=%.d))

$(build.dir)$(abc-lib.dir)%.o : $(abc-lib.dir)%.abc  $(ABC) \
		| $(build.dir)$(abc-lib.dir)
	$(ABC) -c $(ABCFLAGS) $< -o $@ -MF $(@:%.o=%.d) -MT '$@' -MD -MP

$(abc-lib.lib)(%.o) : $(build.dir)$(abc-lib.dir)%.o
	$(AR) $(ARFLAGS) $@ $<

$(abc-lib.lib) : $(abc-lib.lib)($($(abc-lib.name).lib.obj))
	$(RANLIB) $@
