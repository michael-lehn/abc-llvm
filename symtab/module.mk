symtab.requires.lib := \
	$(build.dir)gen/libgen.a \
	$(build.dir)expr/libexpr.a \
	$(build.dir)type/libtype.a \
	$(build.dir)lexer/liblexer.a \
	$(build.dir)util/libutil.a


symtab.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
symtab.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`

