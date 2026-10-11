# omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Host build (default). For the MIPS stick target cross-compile with
# e.g.:  make CC=mipsel-openwrt-linux-musl-gcc
# (see ../mod-optic-modern/PORTING.md for the toolchain recipe)

CC      ?= gcc
CFLAGS  ?= -O2 -g
CFLAGS  += -std=c99 -Wall -Wextra -Werror -I. -Iomci -Iplatform -Icore
LDFLAGS ?=

# ---- objects ---------------------------------------------------------------
OMCI_OBJS  = omci/codec.o omci/me_defs.o omci/mib.o
PLAT_OBJS  = platform/ini.o
SIM_OBJS   = sim/drv_mock.o sim/olt_sim.o
CORE_OBJS  = core/activation.o core/omcc.o

# GPL driver headers (interface A) for the real ioctl backend
GPON_DRV_INC ?= $(HOME)/gpon-port/gpon_onu_drv/src/include
IFXOS_INC    ?= $(HOME)/gpon-port/lib_ifxos/src/include
IOCFLAGS = -DLINUX -I$(GPON_DRV_INC) -I$(IFXOS_INC)

# ---- targets ---------------------------------------------------------------
.PHONY: all test sim clean

all: libomci.a

libomci.a: $(OMCI_OBJS) $(PLAT_OBJS) $(CORE_OBJS) $(SIM_OBJS)
	$(AR) rcs $@ $^

tests/test_codec: tests/test_codec.c omci/codec.c omci/omci.h
	$(CC) $(CFLAGS) -o $@ tests/test_codec.c omci/codec.c $(LDFLAGS)

tests/test_ini: tests/test_ini.c omci/codec.c omci/me_defs.c omci/mib.c platform/ini.c omci/omci.h omci/me_defs.h omci/mib.h platform/ini.h
	$(CC) $(CFLAGS) -o $@ tests/test_ini.c omci/codec.c omci/me_defs.c omci/mib.c platform/ini.c $(LDFLAGS)

tests/test_omcc: tests/test_omcc.c omci/codec.c omci/me_defs.c omci/mib.c platform/ini.c core/activation.c core/omcc.c sim/drv_mock.c sim/olt_sim.c
	$(CC) $(CFLAGS) -o $@ tests/test_omcc.c omci/codec.c omci/me_defs.c omci/mib.c platform/ini.c core/activation.c core/omcc.c sim/drv_mock.c sim/olt_sim.c $(LDFLAGS)

sim/omcid2_sim: sim/omcid2_sim.c libomci.a
	$(CC) $(CFLAGS) -o $@ sim/omcid2_sim.c libomci.a $(LDFLAGS)

sim: sim/omcid2_sim

# compile-check the real interface-A client against the GPL headers
.PHONY: check-ioctl
test_drv_ioctl.o: platform/drv_ioctl.c platform/drv_adapter.h
	$(CC) $(CFLAGS) $(IOCFLAGS) -c -o $@ platform/drv_ioctl.c
check-ioctl: test_drv_ioctl.o
	@echo "drv_ioctl compiles against $(GPON_DRV_INC)"

test: tests/test_codec tests/test_ini tests/test_omcc
	./tests/test_codec
	./tests/test_ini
	./tests/test_omcc

clean:
	rm -f libomci.a $(OMCI_OBJS) $(PLAT_OBJS) $(CORE_OBJS) $(SIM_OBJS) \
		tests/test_codec tests/test_ini tests/test_omcc sim/omcid2_sim \
		test_drv_ioctl.o
