#CPPFLAGS += -Werror -Wextra -Wall -pedantic -fsanitize=undefined,address
CPPFLAGS += -Werror -Wextra -Wall -pedantic
RANLIB := ranlib

build.dir := build/
abc-std-lib := $(build.dir)libabc.a

include config/ar
include config/cxx_and_llvm
include config/prefix

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

src.o.compile_cmd := \
	$(patsubst %,$(build.dir)%.compile_cmd,\
		$(src.cpp:.cpp=.o))

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

$(build.dir)abc/abc.o : abc/abc.cpp \
		$(build.dir)prefix \
		$(build.dir)libdir \
		$(build.dir)includedir \
		| $(build.subdir)
	@echo "{ \"directory\": \"$(PWD)\", \"file\": \"$<\", \
		\"command\": \"$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<\" \
		}" > $@.compile_cmd
	$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<

$(build.dir)abc/abc.o.compile_cmd : abc/abc.cpp \
		$(build.dir)prefix \
		$(build.dir)libdir \
		$(build.dir)includedir \
		| $(build.subdir)
	@echo "{ \"directory\": \"$(PWD)\", \"file\": \"$<\", \
		\"command\": \"$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<\" \
		}" > $@

$(build.dir)%.o : %.cpp | $(build.subdir)
	$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<

$(build.dir)%.o.compile_cmd : %.cpp | $(build.subdir)
	@echo "{ \"directory\": \"$(PWD)\", \"file\": \"$<\", \
		\"command\": \"$(COMPILE.cpp) -I. -o $@ -MT '$@' -MMD -MP $<\" \
		}" > $@

$(build.dir)% : $(build.dir)%.o $(lib.cpp.o)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) $(TARGET_ARCH) -o $@

$(build.dir)%.o : %.abc $(ABC) | $(build.subdir) $(ABC)
	$(ABC) -c $(ABCFLAGS) $< -o $@ -MF $(@:%.o=%.d) -MT '$@' -MD -MP

$(abc-std-lib)(%.o) : $(build.dir)%.o | $(build.dir)
	$(AR) $(ARFLAGS) $@ $<

$(abc-std-lib) : $(abc-std-lib)($(lib.abc.o)) | $(lib.abc.o)
	$(RANLIB) $@


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

.PHONY: compile_cmd
compile_cmd: $(src.o.compile_cmd)
	@echo '[' > compile_commands.json
	@cat $(src.o.compile_cmd) >> compile_commands.json
	@echo ']' >> compile_commands.json

.PHONY: install
install: $(ABC) $(abc-std-lib) | $(PREFIX)/bin/ $(LIBDIR) $(INCLUDEDIR)
	cp abc-include/* $(INCLUDEDIR)
	cp $(build.dir)/*.hdr $(INCLUDEDIR)
	cp $(abc-std-lib) $(LIBDIR)
	cp $(ABC) $(PREFIX)/bin/


.PHONY: clang-version
clang-version:
	clang++ --version

.PHONY: clean
clean:
	$(RM) -rf $(build.dir)

-include $(src.d)
