# top-level build for the GPON stick CLI tools
# (onu, gtop, otop) - self-contained, driver headers staged in drv-headers/

CFLAGS ?= -O2
CFLAGS += -DLINUX -DHAVE_CONFIG_H -include stdio.h -include string.h -include stdlib.h

ONU_INC := -I../drv-headers/onu -I../drv-headers/ifxos
OPTIC_INC := -I../drv-headers/optic -I../drv-headers/ifxos -I.

.PHONY: all onu gtop otop clean

all: onu gtop otop

onu:
	$(MAKE) -C onu CC='$(CC)' BASE_CFLAGS='$(CFLAGS)' ONU_INC='$(ONU_INC)'

gtop:
	$(MAKE) -C gtop CC='$(CC)' BASE_CFLAGS='$(CFLAGS)' ONU_INC='$(ONU_INC)'

otop:
	$(MAKE) -C otop CC='$(CC)' BASE_CFLAGS='$(CFLAGS)' OPTIC_INC='$(OPTIC_INC)'

clean:
	$(MAKE) -C onu clean
	$(MAKE) -C gtop clean
	$(MAKE) -C otop clean
