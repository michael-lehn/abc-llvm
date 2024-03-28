CXXFLAGS += -std=c++20 -ftrapv
CPPFLAGS += -Wextra -Wall


RANLIB := ranlib
build.dir := build/

targets :=
module.list :=
dep.files := 

include config/ar
include config/cxx_and_llvm
include zmk/setup_module.mk

#
# Scan all modules
#
$(foreach m,$(wildcard */module.mk),\
 	$(eval \
		$(call setup_module,$(m))))


#
# List of required build directories
#
mk.build.dir := $(sort $(dir $(obj.files)))

#
# List of libraries
#
lib.list :=
$(foreach m,$(module.list),\
	$(if $($(m).lib.cpp),\
		$(eval lib.list += lib$(m).a)))


%/: ; mkdir -p $@

.DEFAULT_GOAL := all
.PHONY: all
all: $(targets)

$(info dep.files = $(dep.files))

-include $(dep.files)
