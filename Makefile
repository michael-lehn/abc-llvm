CXX := clang++
#CXX := g++-13

#llvm.comp := `llvm-config --cxxflags`
llvm.comp := -I/usr/local/Cellar/llvm/17.0.6/include
llvm.link := `llvm-config --ldflags --system-libs --libs core`

CXXFLAGS += $(llvm.comp)
CXXFLAGS += -std=c++20

src.dir := src
bin.dir := bin
dep.dir := dep
obj.dir := obj

# support generation of dep files
CXXFLAGS += -MT $@ -MMD -MP -MF $(dep.dir)/$(<F).d

xsrc := $(wildcard $(src.dir)/xtest_*.cpp)
src := $(filter-out $(xsrc),$(wildcard $(src.dir)/*.cpp))
obj := $(src:$(src.dir)/%.cpp=$(obj.dir)/%.o)
dep := $(src:$(src.dir)/%=$(dep.dir)/%.d) $(xsrc:$(src.dir)/%.cpp=$(dep.dir)/%.d)

target := $(patsubst $(src.dir)/%.cpp,$(bin.dir)/%,$(xsrc))

.DEFAULT_GOAL := all

.PHONY: all
all: $(target)
	@echo $(dep)	

.PHONY: clean
clean:
	$(RM) $(obj) $(dep) $(target)

$(obj.dir)/%.o :$(src.dir)/%.cpp
$(obj.dir)/%.o : $(src.dir)/%.cpp | $(dep.dir) $(obj.dir)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(bin.dir)/xtest_%: $(src.dir)/%.cpp
$(bin.dir)/xtest_%: $(obj.dir)/xtest_%.o $(obj) | $(bin.dir)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(llvm.link) -o $@

$(obj.dir): ; mkdir -p $@
$(dep.dir): ; mkdir -p $@
$(bin.dir): ; mkdir -p $@

$(dep):
-include $(dep)
