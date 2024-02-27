#in: $1	    (id, e.g. build.variant.module)
#in: $2	    (prg, e.g. build/variant/module/foo)

# requirements:
# $(notdir $1).in   (list of input files, e.g. foo.in = in.txt)
# $(notdir $1).out  (list of output files, e.g. foo.out = out.txt)
# $($1.src_dir)	    (directory with input files)
# $($1.build_dir)   (directory for output files)

# effect:
# Build rules for output files. E.g.
#
# build/variant/module/out.txt : build/variant/module/foo module/in.txt
#	    build/variant/module/foo module/in.txt build/variant/module/out.txt

define gen
$(eval src_dir := \
    $($1.src_dir))
$(eval variant_dir := \
    $($1.variant_dir))
$(eval build_dir := \
    $($1.build_dir))
$(eval build_dir.top := \
    $($1.build_dir.top))

$(eval gen.prg := \
    $(notdir $2))
$(eval gen.module := \
    $($1.module))
$(eval gen.in := \
    $($(gen.module).$(gen.prg).in))
$(eval gen.out := \
    $(patsubst %,$($1.build_dir)%,\
	$($(gen.module).$(gen.prg).out)))


$(gen.out) : $2 $(gen.in) | $($1.build_dir)
	$(value 2) $(value gen.in) $(value gen.out)
endef

#$(eval TARGET += $(gen.out))
