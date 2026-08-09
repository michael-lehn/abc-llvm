CC := abc
CFLAGS += -O3
LDFLAGS  += $(shell pkg-config --libs raylib)

src    := $(wildcard *.abc)
target := $(patsubst %.abc,%,$(src))

.PHONY: all clean

all: $(target)

% : %.o
	$(LINK.o) $^ -o $@

%.o : %.abc raylib.hdr
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(target)
