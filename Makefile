CFLAGS += 
CXXFLAGS += -std=c++20
CPPFLAGS += -Wextra -Wall -I.


include config/ar
include config/nfs
include config/llvm

include zmk/make.mk
