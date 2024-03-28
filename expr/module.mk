expr.requires.lib := \
	$(build.dir)gen/libgen.a \
	$(build.dir)type/libtype.a \
	$(build.dir)lexer/liblexer.a \
	$(build.dir)util/libutil.a

expr.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
expr.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`

