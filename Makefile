CXX := g++
#CXX := clang++
#CXX := g++-13

#llvm.comp := `llvm-config --cxxflags`
llvm.comp := -I `llvm-config --includedir`
llvm.link := `llvm-config --ldflags --system-libs --libs core`

CXXFLAGS += $(llvm.comp)
CXXFLAGS += -O3 -std=c++20 -Wall -Wextra -Wno-unused-parameter -pedantic -Wuninitialized

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

$(bin.dir)/xtest_expr3 : $(obj.dir)/xtest_expr3.o
$(bin.dir)/xtest_expr3 : $(obj.dir)/xtest_expr3.o $(obj.dir)/expr.o	\
			$(obj.dir)/integerliteral.o $(obj.dir)/gen.o \
			$(obj.dir)/type.o  $(obj.dir)/ustr.o  \
			$(obj.dir)/error.o  $(obj.dir)/lexer.o  \
			$(obj.dir)/symtab.o $(obj.dir)/identifier.o \
			$(obj.dir)/binaryexpr.o $(obj.dir)/castexpr.o \
			$(obj.dir)/proxyexpr.o $(obj.dir)/unaryexpr.o \
			$(obj.dir)/stringliteral.o $(obj.dir)/promotion.o \
			$(obj.dir)/exprvector.o $(obj.dir)/conditionalexpr.o\
			| $(bin.dir)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(llvm.link) -o $@

$(bin.dir)/xtest_expr : $(obj.dir)/xtest_expr.o
$(bin.dir)/xtest_expr : $(obj.dir)/xtest_expr.o $(obj.dir)/expr.o	\
			$(obj.dir)/integerliteral.o $(obj.dir)/gen.o \
			$(obj.dir)/type.o  $(obj.dir)/ustr.o  \
			$(obj.dir)/error.o  $(obj.dir)/lexer.o  \
			$(obj.dir)/symtab.o $(obj.dir)/identifier.o \
			$(obj.dir)/binaryexpr.o $(obj.dir)/castexpr.o \
			$(obj.dir)/proxyexpr.o $(obj.dir)/unaryexpr.o \
			$(obj.dir)/stringliteral.o $(obj.dir)/promotion.o \
			$(obj.dir)/exprvector.o $(obj.dir)/conditionalexpr.o\
			| $(bin.dir)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(llvm.link) -o $@

$(obj.dir): ; mkdir -p $@
$(dep.dir): ; mkdir -p $@
$(bin.dir): ; mkdir -p $@

$(dep):
-include $(dep)
