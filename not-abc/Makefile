CC := abc
CFLAGS += -O3 
DEPFLAGS += -MD -MF $(dep.dir)/$(<F).d \
	    -MT $(@:$(cpp.dir)/%.abc_cpp=$(obj.dir)/%.o) -MP

ulm.isa := simple
ulm.tools := $(ulm.isa)/ulm $(ulm.isa)/ulmas $(ulm.isa)/udb-tui
ulm.gen := ulm-generator

LDFLAGS += 

dep.dir := dep
obj.dir := obj

gen_src := $(wildcard gen_*.abc)
gen_obj := $(gen_src:%.abc=$(obj.dir)/%.o)

backend := $(patsubst gen_%,%,$(basename $(notdir $(gen_src))))

target_src := $(wildcard x*.abc) not-abc-written-in-abc.abc
target_obj := $(target_src:%.abc=$(obj.dir)/%.o)
target := $(foreach b,$(backend), \
             $(patsubst %.o,%_$(b),$(notdir $(target_obj))))

common_src := $(filter-out $(gen_src) $(target_src), $(wildcard *.abc))
common_obj := $(common_src:%.abc=$(obj.dir)/%.o)

dep := $(gen_src:%=$(dep.dir)/%.d) \
       $(common_src:%=$(dep.dir)/%.d) \
       $(target_src:%=$(dep.dir)/%.d)

define target_rule
$(1)_$(2): $(obj.dir)/$(1).o $(common_obj) $(obj.dir)/gen_$(2).o
	$(CC) -o $$@ $$^
endef

$(foreach t,$(basename $(notdir $(target_src))), \
	$(foreach b,$(backend), \
		$(eval $(call target_rule,$(t),$(b)))))

$(obj.dir)/%.o : %.abc | $(obj.dir) $(dep.dir)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

.DEFAULT_GOAL := all

$(dep.dir): ; mkdir -p $@
$(obj.dir): ; mkdir -p $@

.PHONY: all
all: $(target) $(common_obj) $(target_obj) $(ulm.tools)
	@./install-myabc.sh

$(ulm.tools) $(ulm.isa): $(ulm.isa).isa
	@if command -v $(ulm.gen) >/dev/null 2>&1; then \
		rm -rf $(ulm.isa); \
		$(ulm.gen) --install $(ulm.isa).isa; \
	else \
		echo; \
		echo "Warning: $(ulm.gen) is not installed."; \
		echo "The ULM assembler tools will not be built."; \
		echo "You can clone it from:"; \
		echo "  https://github.com/michael-lehn/ulm-generator"; \
		echo; \
	fi

.PHONY: tree.tex
tree.tex: xtest_parser_simple
	@echo '\\documentclass[preview, margin=0.2cm]{standalone}' > tree.tex
	@echo '\\usepackage{forest}' >> tree.tex
	@echo '\\begin{document}' >> tree.tex
	@echo '\\begin{forest}' >> tree.tex
	@echo 'Type an expression (use Control-D for EOI):'
	./xtest_parser_simple >> tree.tex
	@echo '\\end{forest}' >> tree.tex
	@echo '\\end{document}' >> tree.tex
	@echo "Generated 'tree.tex'"
	@echo
	@echo "Next two things to do:"
	@echo "1) run 'lualatex tree.tex' to generate 'tree.pdf'"
	@echo "2) run 'open tree.pdf' to view 'tree.pdf'"

.PHONY: clean
clean:
	$(RM) $(target)
	$(RM) -rf $(dep.dir) $(obj.dir)

$(dep):
-include $(dep)
