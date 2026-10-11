# Copyright (C) 2026 thecodingrobot
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-gpon-onu
PKG_VERSION:=4.5.0
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-or-later BSD-2-Clause
PKG_LICENSE_FILES:=src/LICENSE

# Source vendored in-tree from the Lantiq/Intel UGW SDK gpon_onu_drv 4.5.0
# (dual BSD/GPL, mirrored at gitea.osmocom.org/gpon) with a port to modern
# kernels - mod_onu.ko plus the netdev and led-trigger companion modules.

PKG_EXTMOD_SUBDIRS:=src netdev led-trigger

include $(INCLUDE_DIR)/package.mk

IFXOS_DIR:=$(KERNEL_BUILD_DIR)/ifxos-ugw_8.5.2.10

define KernelPackage/ltq-gpon-onu
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=Lantiq Falcon GPON ONU driver
  DEPENDS:=@TARGET_lantiq_falcon +kmod-ltq-ifxos +kmod-ltq-gpon-optic
  FILES:= \
	$(PKG_BUILD_DIR)/src/mod_onu.ko \
	$(PKG_BUILD_DIR)/netdev/mod_onu_netdev.ko \
	$(PKG_BUILD_DIR)/led-trigger/mod_onu_led_trigger.ko
  AUTOLOAD:=$(call AutoLoad,30,mod_onu mod_onu_netdev mod_onu_led_trigger)
endef

define KernelPackage/ltq-gpon-onu/description
  Lantiq Falcon GPON ONU driver (mod_onu.ko) - GTC/PLOAM/GPE engine control
  for the GPON SFP sticks, plus the netdev companion (LAN-side Ethernet
  interface) and the led-trigger companion. Exposes the CPE API char device
  /dev/onu0 consumed by the 'onu' and 'gtop' userspace tools and by OMCI
  daemons. Ported from the UGW SDK 4.5.0 source to modern kernels.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src $(PKG_BUILD_DIR)/src
	$(CP) ./netdev $(PKG_BUILD_DIR)/netdev
	$(CP) ./led-trigger $(PKG_BUILD_DIR)/led-trigger
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)/src" \
		IFXOS_SRC="$(IFXOS_DIR)" \
		KBUILD_EXTRA_SYMBOLS="$(wildcard $(KERNEL_BUILD_DIR)/symvers/*.symvers)" \
		modules
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)/netdev" \
		IFXOS_SRC="$(IFXOS_DIR)" \
		ONU_SRC="$(PKG_BUILD_DIR)/src" \
		KBUILD_EXTRA_SYMBOLS="$(wildcard $(KERNEL_BUILD_DIR)/symvers/*.symvers) $(PKG_BUILD_DIR)/src/Module.symvers" \
		modules
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)/led-trigger" \
		IFXOS_SRC="$(IFXOS_DIR)" \
		ONU_SRC="$(PKG_BUILD_DIR)/src" \
		KBUILD_EXTRA_SYMBOLS="$(wildcard $(KERNEL_BUILD_DIR)/symvers/*.symvers) $(PKG_BUILD_DIR)/src/Module.symvers" \
		modules
endef

$(eval $(call KernelPackage,ltq-gpon-onu))
