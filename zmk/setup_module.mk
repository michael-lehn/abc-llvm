define setup_module
	$(eval include $1)
	$(eval module.dir := $(dir $1))
	$(eval module.name := $(lastword $(subst /,,$(module.dir))))
	$(eval module.list += $(module.name))
:	All source files in module
	$(eval $(module.name).cpp := \
		$(wildcard $(module.dir)*.cpp))
:	Update list of dependencies
	$(eval dep.files += \
		$(patsubst %,$(build.dir)%,\
			$($(module.name).cpp:%.cpp=%.d)))
:	Source files for test programs
	$(eval $(module.name).prg.cpp := \
		$($(module.name).prg.cpp) \
		$(filter $(module.dir)xtest%,\
			$($(module.name).cpp)))
:	Object files for test programs
	$(eval $(module.name).prg.obj := \
		$(patsubst %,$(build.dir)%,\
			$($(module.name).prg.cpp:%.cpp=%.o)))
:	Executable files for test programs
	$(eval $(module.name).prg := \
		$(patsubst %,$(build.dir)%,\
			$($(module.name).prg.cpp:%.cpp=%)))
:	Source files for library
	$(eval $(module.name).lib.cpp := \
		$(filter-out $($(module.name).prg.cpp),\
			$($(module.name).cpp)))
:	Object files for library
	$(eval $(module.name).lib.obj := \
		$(patsubst %,$(build.dir)%,\
			$($(module.name).lib.cpp:%.cpp=%.o)))
:	Library (eventually)
	$(if $($(module.name).lib.obj),\
		$(eval module.lib := \
			$(build.dir)$(module.dir)lib$(module.name).a))
:	Update targets with test programs, object files for test programs and
:	eventually the module library
	$(eval targets += $($(module.name).prg.obj))
	$(eval targets += $($(module.name).prg))
	$(if $($(module.name).lib.obj),\
		$(eval targets += $(module.lib)))


$(build.dir)$(module.dir)%.o : $(module.dir)%.cpp \
		| $(build.dir)$(module.dir)
	$(COMPILE.cpp) -I. $($(module.name).CPPFLAGS) -o $$@ -MT '$$@' -MMD -MP $$<

$(module.lib)(%.o) : $(build.dir)$(module.dir)%.o
	$(AR) $(ARFLAGS) $$@ $$<

$(module.lib) : $(module.lib)($($(module.name).lib.obj))
	$(RANLIB) $$@

$(build.dir)$(module.dir)%: $(build.dir)$(module.dir)%.o $(module.lib) \
				$($(module.name).requires.lib)
	$(LINK.cc) -o $$@ $$< $(module.lib) $($(module.name).requires.lib) $($(module.name).extern.lib)

endef
