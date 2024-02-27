#in:	$1	(build, e.g. '_release')

define setup_build
$(eval $1.CC := \
    $(or $($1.CC),$(CC)))
$(eval $1.CXX := \
    $(or $($1.CXX),$(CXX)))
$(eval $1.CPPFLAGS := \
    $(or $($1.CPPFLAGS),$(CPPFLAGS)))
$(eval $1.CFLAGS := \
    $(or $($1.CFLAGS),$(CFLAGS)))
$(eval $1.LDFLAGS := \
    $(or $($1.LDFLAGS),$(LDFLAGS)))
$(eval $1.TARGET_ARCH := \
    $(or $($1.TARGET_ARCH),$(TARGET_ARCH)))
$(eval $1.COMPILE.c := \
    $($1.CC) \
    $($1.CPPFLAGS) \
    $($1.CFLAGS) \
    $($1.TARGET_ARCH) \
    -c )
$(eval $1.COMPILE.cpp := \
    $($1.CXX) \
    $($1.CPPFLAGS) \
    $($1.CXXFLAGS) \
    $($1.TARGET_ARCH) \
    -c )
$(eval $1.LINK.c.o := \
    $($1.CC) \
    $($1.LDFLAGS) \
    $($1.TARGET_ARCH))
$(eval $1.LINK.cpp.o := \
    $($1.CXX) \
    $($1.LDFLAGS) \
    $($1.TARGET_ARCH))
endef
