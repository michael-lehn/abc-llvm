parser.requires.lib := \
	$(build.dir)ast/libast.a \
	$(build.dir)symtab/libsymtab.a \
	$(build.dir)expr/libexpr.a \
	$(build.dir)gen/libgen.a \
	$(build.dir)type/libtype.a \
	$(build.dir)lexer/liblexer.a \
	$(build.dir)util/libutil.a

parser.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
parser.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`
