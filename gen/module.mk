gen.requires.lib := \
	$(build.dir)type/libtype.a \
	$(build.dir)util/libutil.a

gen.CPPFLAGS += -Wno-unused-parameter -I `$(llvm-config.cmd) --includedir`
gen.extern.lib += `$(llvm-config.cmd) --ldflags --system-libs --libs all`

