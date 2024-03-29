abc.requires.lib := \
	$(build.dir)parser/libparser.a \
	$(build.dir)ast/libast.a \
	$(build.dir)expr/libexpr.a \
	$(build.dir)symtab/libsymtab.a \
	$(build.dir)gen/libgen.a \
	$(build.dir)type/libtype.a \
	$(build.dir)lexer/liblexer.a \
	$(build.dir)util/libutil.a

abc.prg.cpp += abc/abc.cpp

abc.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
abc.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`
