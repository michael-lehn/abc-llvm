ast.requires.lib := \
	$(build.dir)gen/libgen.a \
	$(build.dir)expr/libexpr.a \
	$(build.dir)type/libtype.a \
	$(build.dir)lexer/liblexer.a \
	$(build.dir)symtab/libsymtab.a \
	$(build.dir)util/libutil.a

ast.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
ast.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`

