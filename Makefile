CXX := g++
#CXX := clang++
#CXX := g++-13

#llvm.comp := `llvm-config --cxxflags`
llvm.comp := -I `llvm-config --includedir`
llvm.link := `llvm-config --ldflags --system-libs --libs all`

CXXFLAGS += $(llvm.comp)
CXXFLAGS += -std=c++20 -Werror -Wall -Wextra -Wno-unused-parameter -pedantic \
	    -Wuninitialized
CXXFLAGS += -O3

src.dir := src
bin.dir := bin
dep.dir := dep
obj.dir := obj

# support generation of dep files
CXXFLAGS += -MT $@ -MMD -MP -MF $(dep.dir)/$(<F).d

compiler.src := src/abc.cpp

xsrc := $(wildcard $(src.dir)/xtest_*.cpp) \
	$(compiler.src)
src := $(filter-out $(xsrc),$(wildcard $(src.dir)/*.cpp))
obj := $(src:$(src.dir)/%.cpp=$(obj.dir)/%.o)
xobj := $(xsrc:$(src.dir)/%.cpp=$(obj.dir)/%.o)
dep := $(src:$(src.dir)/%=$(dep.dir)/%.d) \
       $(xsrc:$(src.dir)/%=$(dep.dir)/%.d)

target := $(patsubst $(src.dir)/%.cpp,$(bin.dir)/%,$(xsrc)) $(xobj)

.DEFAULT_GOAL := all

.PHONY: all
all: $(target)

.PHONY: clean
clean:
	$(RM) $(obj) $(xobj) $(dep) $(target)

$(obj.dir)/%.o :$(src.dir)/%.cpp
$(obj.dir)/%.o : $(src.dir)/%.cpp | $(dep.dir) $(obj.dir)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(bin.dir)/%: $(obj.dir)/%.cpp
$(bin.dir)/%: $(obj.dir)/%.o
$(bin.dir)/%: $(obj.dir)/%.o $(obj) | $(bin.dir)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< $(obj) $(llvm.link) -o $@

$(bin.dir)/xtest_lexer: $(obj.dir)/xtest_lexer.o $(obj.dir)/lexer.o $(obj.dir)/error.o $(obj.dir)/ustr.o | $(bin.dir)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(llvm.link) -o $@

$(obj.dir): ; mkdir -p $@
$(dep.dir): ; mkdir -p $@
$(bin.dir): ; mkdir -p $@

$(dep):
-include $(dep)
