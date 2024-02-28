#in:	$1	(module, e.g. 'utils')
#in:	$2	(variant, e.g. 'ulm')
#in:	$3	(build, e.g. '_release')

# requirements:
# $1.dir	(directory of the module)

# effect:
# $1.h		($1.dir/*.h)
# $1.c		($1.dir/*.c)
# $1.c.d	full path
# $1.prg.c.o	full path
# $1.lib.c.o	object filename only

include zmk/gen.mk
include zmk/install.mk

define setup_file_lists
$(eval id := $3.$2.$1)
$(eval $(id).module := $1)
$(eval $(id).variant := $2)
$(eval $(id).build := $3)
$(eval $(id).src_dir := $($1.dir))
$(eval $(id).variant_dir := $($2.dir))
$(eval $(id).install_dir := $($3.dir)$2/)
$(eval $(id).build_dir.top := $($(id).install_dir)/opt/)
$(eval $(id).build_dir := $($(id).build_dir.top)$1/)
$(eval $(id).dep_dir.top := $($(id).install_dir).dep/)
$(eval $(id).dep_dir := $($(id).dep_dir.top)$2/$1/)

$(if $($($(id).module).has.extra), \
    $(call $($(id).module).extra,\
	$($(id).src_dir),\
	$($(id).build_dir),\
	$($(id).install_dir), \
	$($(id).variant)))

$(eval $(id).requires.gen := \
    $(foreach i,$($1.requires.gen), \
	$(patsubst %,$($(id).build_dir.top)%,$i)))

$(foreach i,$($1.install), \
    $(call install,$i,$($(id).build_dir),$($(id).install_dir)))

$(eval $(id).h := \
    $(wildcard $($(id).src_dir)*.h))
$(eval $(id).c := \
    $(wildcard $($(id).src_dir)*.c))
$(eval $(id).prg.c := \
    $(patsubst %,$($(id).src_dir)%,$($1.prg.c)))
$(eval $(id).prg.c += \
    $(filter-out $($(id).prg.c), \
	$(wildcard $($(id).src_dir)xtest*.c)))
$(eval $(id).lib.c := \
    $(filter-out $($(id).prg.c),$($(id).c)))

$(eval $(id).hpp := \
    $(wildcard $($(id).src_dir)*.hpp))
$(eval $(id).cpp := \
    $(wildcard $($(id).src_dir)*.cpp))
$(eval $(id).prg.cpp := \
    $(patsubst %,$($(id).src_dir)%,$($1.prg.cpp)))
$(eval $(id).prg.cpp += \
    $(filter-out $($(id).prg.cpp), \
	$(wildcard $($(id).src_dir)xtest*.cpp)))
$(eval $(id).lib.cpp := \
    $(filter-out $($(id).prg.cpp),$($(id).cpp)))

$(eval $(id).prg.c.o := \
    $(patsubst $($(id).src_dir)%,$($(id).build_dir)%,\
	$($(id).prg.c:.c=.o)))
$(eval $(id).lib.c.o := \
    $(patsubst $($(id).src_dir)%,%,\
	$($(id).lib.c:.c=.o)))
$(eval $(id).c.d := \
    $(patsubst $($(id).src_dir)%,$($(id).dep_dir)%,\
	$($(id).c:.c=.c.d)))

$(eval $(id).prg.cpp.o := \
    $(patsubst $($(id).src_dir)%,$($(id).build_dir)%,\
	$($(id).prg.cpp:.cpp=.o)))
$(eval $(id).lib.cpp.o := \
    $(patsubst $($(id).src_dir)%,%,\
	$($(id).lib.cpp:.cpp=.o)))
$(eval $(id).cpp.d := \
    $(patsubst $($(id).src_dir)%,$($(id).dep_dir)%,\
	$($(id).cpp:.cpp=.cpp.d)))

$(eval $(id).dep.content := \
    $(wildcard $($(id).dep_dir)*.d))
$(eval $(id).dep.content.obsolete := \
    $(filter-out $($(id).c.d) $($(id).cpp.d),$($(id).dep.content)))

$(eval $(id).prg := \
    $(patsubst $($(id).src_dir)%,$($(id).build_dir)%,\
	$($(id).prg.c:.c=) $($(id).prg.cpp:.cpp=)))
$(eval $(id).lib := \
    $(if $(strip $($(id).lib.c.o) $($(id).lib.cpp.o)),\
	$($(id).build_dir)lib$1.a))
$(eval $(id).lib.content := \
    $(if $(wildcard $($(id).lib)),\
	$(shell ar t $($(id).lib) | grep -v "^__")))
$(eval $(id).lib.content.obsolete := \
    $(filter-out $($(id).lib.c.o) $($(id).lib.cpp.o),$($(id).lib.content)))
$(eval $(id).lib.ar_d := \
    $(if $($(id).lib.content.obsolete),$(id).ar_d))

$(eval $(id).link.libs := \
	$($(id).lib) \
	$(patsubst %,$($(id).build_dir.top)%,\
		$($1.requires.lib)))

$(eval $(id).LINK.o := \
    $(if $($(id).cpp),$($3.LINK.cpp.o),$($3.LINK.c.o)))

$(eval OPT_TARGET += \
    $($(id).lib) $($(id).prg) $($(id).prg.c.o) $($(id).prg.cpp.o))
$(eval DEP_FILES += $($(id).c.d) $($(id).cpp.d))
$(eval EXTRA_DIRS += $($(id).build_dir) $($(id).dep_dir))
$(eval CLEAN_DIRS := \
    $(sort $(CLEAN_DIRS) $($(id).build_dir.top) $($(id).dep_dir.top)))

.PHONY: $($(id).dep.content.obsolete) $($(id).lib.ar_d)


$($(id).lib.ar_d):
	ar d $($(id).lib) $($(id).lib.content.obsolete) 

$($(id).lib) : $($(id).lib)($($(id).lib.c.o) $($(id).lib.cpp.o)) \
    | $($(id).build_dir) $($(id).lib.ar_d)
	$(RANLIB) $(value $(id).lib)

$($(id).build_dir)%.o : $($(id).src_dir)%.c \
    | $($(id).build_dir) $($(id).dep_dir) $($(id).dep.content.obsolete) \
      $($(id).requires.gen)
	$($3.COMPILE.c) \
	    -o $$@ \
	    -I $($(id).build_dir).. \
	    $($($(id).module).CXXFLAGS) \
	    -MT '$$@' \
	    -MMD -MP \
	    -MF $($(id).dep_dir)$$(notdir $$<).d \
	    $$<

$($(id).build_dir)%.o : $($(id).src_dir)%.cpp \
    | $($(id).build_dir) $($(id).dep_dir) $($(id).dep.content.obsolete) \
      $($(id).requires.gen)
	$($3.COMPILE.cpp) \
	    -o $$@ \
	    -I $($(id).build_dir).. \
	    $($($(id).module).CXXFLAGS) \
	    $($($(id).module).CPPFLAGS) \
	    -MT '$$@' \
	    -MMD -MP \
	    -MF $($(id).dep_dir)$$(notdir $$<).d \
	    $$<

$($(id).build_dir)% : $($(id).build_dir)%.o $($(id).link.libs) \
    | $($(id).build_dir) $($(id).dep_dir) $($(id).dep.content.obsolete)
	$($(id).LINK.o) $($1.LDFLAGS) $$< \
	    $($($(id).module).extra_libs) \
	    $($(id).link.libs) \
	    $($1.LDFLAGS) \
	    -o $$@

$($(id).lib)(%.o) : $($(id).src_dir)%.c \
    | $($(id).build_dir) $($(id).dep_dir) $($(id).dep.content.obsolete) \
      $($(id).requires.gen)
	$($3.COMPILE.c) \
	    $($1.CFLAGS) \
	    $($($(id).module).CFLAGS) \
	    -o $($(id).build_dir)$$(notdir $$%) \
	    -I $($(id).build_dir.top) \
	    -MT '$$@($$(notdir $$%))' \
	    -MMD -MP \
	    -MF $($(id).dep_dir)$$(notdir $$<).d $$<
	$(AR) $(ARFLAGS) $$@ $($(id).build_dir)$$(notdir $$%)
	$(RM) $($(id).build_dir)$$(notdir $$%)

$($(id).lib)(%.o) : $($(id).src_dir)%.cpp \
    | $($(id).build_dir) $($(id).dep_dir) $($(id).dep.content.obsolete) \
      $($(id).requires.gen)
	$($3.COMPILE.cpp) \
	    $($1.CFLAGS) \
	    $($($(id).module).CXXFLAGS) \
	    $($($(id).module).CPPFLAGS) \
	    -o $($(id).build_dir)$$(notdir $$%) \
	    -I $($(id).build_dir.top) \
	    -MT '$$@($$(notdir $$%))' \
	    -MMD -MP \
	    -MF $($(id).dep_dir)$$(notdir $$<).d $$<
	$(AR) $(ARFLAGS) $$@ $($(id).build_dir)$$(notdir $$%)
	$(RM) $($(id).build_dir)$$(notdir $$%)

$(eval \
    $(foreach p,$($(id).prg),\
	$(call gen,$(id),$p)))
endef
