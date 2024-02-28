$(this).requires.lib := \
	util/libutil.a \
	lexer/liblexer.a \
	type/libtype.a \
	gen/libgen.a

$(this).CPPFLAGS += -Wno-unused-parameter -I `llvm-config --includedir`
$(this).extra_libs += `llvm-config --ldflags --system-libs --libs all`

# All files in source directory beginning with 'xtest' are optional targets
# (built with 'make opt'). Here we here default targets (built with 'make').

$(this).install := xtest_expr
