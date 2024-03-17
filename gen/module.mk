$(this).requires.lib := \
	type/libtype.a \
	util/libutil.a

$(this).CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
$(this).extra_libs += `$(llvm-config.cmd) --ldflags --system-libs --libs all`

# All files in source directory beginning with 'xtest' are optional targets
# (built with 'make opt'). Here we here default targets (built with 'make').

