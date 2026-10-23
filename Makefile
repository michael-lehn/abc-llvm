# GNU Make 4.2 or newer is required ($(file <) in config/prefix).  This check
# must stay first and only use features of GNU Make 3.81 (Apple's system make),
# so that older versions stop here with a clear message instead of failing
# later.  Major and minor version are compared as numbers (4.10 and 5.0 are
# newer than 4.2).
make.version := $(subst ., ,$(MAKE_VERSION))
make.major := $(word 1,$(make.version))
make.minor := $(word 2,$(make.version) 0)
ifneq ($(filter 0 1 2 3,$(make.major))$(if $(filter 4,$(make.major)),$(filter 0 1,$(make.minor))),)
    make.indent := $(subst x,,x  x)
    $(info Error: GNU Make 4.2 or newer is required.)
    # Apple's /usr/bin/make runs the make of the developer tools
    ifneq ($(filter /usr/bin/make /Library/Developer/CommandLineTools/usr/bin/make %/Contents/Developer/usr/bin/make,$(MAKE)),)
        $(info Detected GNU Make $(MAKE_VERSION) (Apple's system make).)
    else
        $(info Detected GNU Make $(MAKE_VERSION) ($(MAKE)).)
    endif
    $(info )
    ifneq ($(wildcard /System/Library/CoreServices/SystemVersion.plist),)
        $(info Install a recent GNU Make using either:)
        $(info )
        $(info $(make.indent)Homebrew:)
        $(info $(make.indent)$(make.indent)brew install make)
        $(info )
        $(info $(make.indent)MacPorts:)
        $(info $(make.indent)$(make.indent)sudo port install gmake)
        $(info )
        $(info Then build using:)
        $(info $(make.indent)$(make.indent)gmake)
    else
        $(info Install GNU Make 4.2 or newer, e.g. with the package manager.)
    endif
    $(info )
    $(error GNU Make 4.2 or newer is required)
endif

#CPPFLAGS += -Werror -Wextra -Wall -pedantic -fsanitize=undefined,address
CPPFLAGS += -Wextra -Wall -pedantic
RANLIB := ranlib

build.dir := build/
abc-std-lib := $(build.dir)libabc.a


include config/ar
include config/cxx_and_llvm
include config/prefix

llvm-config ?= llvm-config

ABC := $(build.dir)abc/abc
ABCFLAGS := -I abc-include

CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config) --includedir`
#CXXFLAGS += `$(llvm-config) --cxxflags`


CFLAGS += `$(llvm-config) --cflags`
LDFLAGS += `$(llvm-config) --ldflags --system-libs --libs all`
CXXFLAGS += -std=c++20 -ftrapv


src.cpp := \
	$(wildcard abc/*.cpp) \
	$(wildcard ast/*.cpp) \
	$(wildcard expr/*.cpp) \
	$(wildcard gen/*.cpp) \
	$(wildcard lexer/*.cpp) \
	$(wildcard parser/*.cpp) \
	$(wildcard symtab/*.cpp) \
	$(wildcard type/*.cpp) \
	$(wildcard util/*.cpp)

src.abc := \
	$(wildcard abc-lib/*.abc)

src.o := \
	$(patsubst %,$(build.dir)%,\
		$(src.cpp:.cpp=.o)) \
	$(patsubst %,$(build.dir)%,\
		$(src.abc:.abc=.o))

src.d := \
	$(src.o:.o=.d)

build.subdir := \
	$(sort $(dir $(src.o)))

prg.cpp := \
	$(wildcard abc/xtest*.cpp) abc/abc.cpp \
	$(wildcard ast/xtest*.cpp) \
	$(wildcard expr/xtest*.cpp) \
	$(wildcard gen/xtest*.cpp) \
	$(wildcard lexer/xtest*.cpp) \
	$(wildcard parser/xtest*.cpp) \
	$(wildcard symtab/xtest*.cpp) \
	$(wildcard type/xtest*.cpp) \
	$(wildcard util/xtest*.cpp)

prg.abc :=

lib.cpp := \
	$(filter-out $(prg.cpp), $(src.cpp))

lib.abc := \
	$(filter-out $(prg.abc), $(src.abc))

prg.cpp.o := \
	$(patsubst %,$(build.dir)%,\
		$(prg.cpp:.cpp=.o))

prg.abc.o := \
	$(patsubst %,$(build.dir)%,\
		$(prg.abc:.abc=.o))

prg.cpp.exe := \
	$(patsubst %,$(build.dir)%,\
		$(prg.cpp:.cpp=))

prg.abc.exe := \
	$(patsubst %,$(build.dir)%,\
		$(prg.abc:.abc=))

lib.cpp.o := \
	$(patsubst %,$(build.dir)%,\
		$(lib.cpp:.cpp=.o))

lib.abc.o := \
	$(patsubst %,$(build.dir)%,\
		$(lib.abc:.abc=.o))

test.unit.cpp := \
	$(wildcard test/unit/*.cpp)

test.unit.exe := \
	$(patsubst %,$(build.dir)%,\
		$(test.unit.cpp:.cpp=))

test.unit.d := \
	$(test.unit.exe:=.d)

$(build.dir)abc/abc.o : abc/abc.cpp \
		$(build.dir)prefix \
		$(build.dir)libdir \
		$(build.dir)includedir \
		| $(build.subdir)
	$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<

$(build.dir)%.o : %.cpp | $(build.subdir)
	$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<

$(build.dir)% : $(build.dir)%.o $(lib.cpp.o)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) $(TARGET_ARCH) -o $@

$(build.dir)%.o : %.abc $(ABC) | $(build.subdir) $(ABC)
	$(ABC) -c $(ABCFLAGS) $< -o $@ -MF $(@:%.o=%.d) -MT '$@' -MD -MP

# Only this recipe writes the archive.  The objects are compiled in parallel,
# then the archive is created in one step (archive member targets,
# lib.a(member.o), are not safe with make -j: concurrent ar calls on the same
# archive lose members).  The archive is built next to the destination and
# renamed only when ar and ranlib succeeded, so a failed or interrupted build
# never replaces a valid archive.
$(abc-std-lib) : $(lib.abc.o)
	@$(RM) $@.tmp
	$(AR) $(ARFLAGS) $@.tmp $^ && $(RANLIB) $@.tmp && mv -f $@.tmp $@ \
		|| { $(RM) $@.tmp; exit 1; }


#-- generate files: E.g. headers like inttypes.hdr

gen := $(build.dir)/inttypes.hdr

$(build.dir)/inttypes.hdr: $(build.dir)util/xtest_gen_inttypes_hdr
	$(build.dir)/util/xtest_gen_inttypes_hdr > $@


%/: ; @mkdir -p $@

$(info PREFIX=$(PREFIX))
$(info LIBDIR=$(LIBDIR))
$(info INCLUDEDIR=$(INCLUDEDIR))

.DEFAULT_GOAL := all
.PHONY: all
all: $(ABC) $(abc-std-lib) $(prg.cpp.exe) $(lib.cpp.o) $(prg.cpp.o) $(gen) \
	compile_cmd

# Clang compilation database for the C++ sources (used by clangd and other
# tools). It is regenerated by every 'make' and describes the current build
# configuration (compiler, flags, llvm-config, build.dir).
.PHONY: compile_cmd
compile_cmd:
	@sh config/compile_commands.sh compile_commands.json $(build.dir) \
		$(sort $(src.cpp) $(test.unit.cpp)) -- $(COMPILE.cpp)

.PHONY: install
install:  all | $(PREFIX)/bin/ $(LIBDIR) $(INCLUDEDIR)
	cp abc-include/* $(INCLUDEDIR)
	cp $(build.dir)/*.hdr $(INCLUDEDIR)
	cp $(abc-std-lib) $(LIBDIR)
	cp $(ABC) $(PREFIX)/bin/


#-- tests: unit tests (test/unit), compiler tests (test/run, test/fail,
#   test/design, test/ir) and tool checks (test/tool), see test/run-tests.sh

$(test.unit.exe:=.o) : | $(build.dir)test/unit/

.PHONY: check
check: $(ABC) $(abc-std-lib) $(gen) $(test.unit.exe) compile_cmd
	@ABC=$(ABC) \
	 ABC_INCLUDE=abc-include \
	 ABC_BUILD=$(build.dir) \
	 LLVM_BINDIR=`$(llvm-config) --bindir` \
	 UNIT_TESTS="$(test.unit.exe)" \
	 CDB_SOURCES="$(sort $(src.cpp) $(test.unit.cpp))" \
	 CDB_COMPILER="$(CXX)" \
	 CDB_BUILD_DIR=$(build.dir) \
	 LLVM_MAJOR=$(llvm-config.version.major) \
	 LLVM_INCLUDEDIR=`$(llvm-config) --includedir` \
	 sh test/run-tests.sh

.PHONY: clang-version
clang-version:
	clang++ --version

.PHONY: clean
clean:
	$(RM) -rf $(build.dir)

-include $(src.d) $(test.unit.d)
