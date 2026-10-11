# Copyright (C) 2026 thecodingrobot
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-gpon-optic
PKG_VERSION:=4.5.0
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-or-later BSD-2-Clause
PKG_LICENSE_FILES:=src/LICENSE

# Source vendored in-tree from the Lantiq/Intel UGW SDK gpon_optic_drv 4.5.0
# (dual BSD/GPL, mirrored at gitea.osmocom.org/gpon) with a port to modern
# kernels - see PORTING notes in the source tree.

include $(INCLUDE_DIR)/package.mk

IFXOS_DIR:=$(KERNEL_BUILD_DIR)/ifxos-ugw_8.5.2.10

define KernelPackage/ltq-gpon-optic
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=Lantiq Falcon GPON optic (BOSA) driver
  DEPENDS:=@TARGET_lantiq_falcon +kmod-ltq-ifxos
  FILES:=$(PKG_BUILD_DIR)/mod_optic.ko
  AUTOLOAD:=$(call AutoLoad,25,mod_optic)
endef

define KernelPackage/ltq-gpon-optic/description
  Lantiq Falcon GPON optic driver (mod_optic.ko) - BOSA optics control,
  DCDC monitoring and calibration for the GPON SFP sticks
  (Huawei MA5671A, Nokia G-010S-P, FS.com GPON-ONU-34-20BI /
  SourcePhotonics SPS-34-24T-HP-TDFO).
  Ported from the UGW SDK 4.5.0 source to modern kernels.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src $(PKG_BUILD_DIR)/src
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)/src" \
		IFXOS_SRC="$(IFXOS_DIR)" \
		KBUILD_EXTRA_SYMBOLS="$(IFXOS_DIR)/src/Module.symvers" \
		modules
endef

$(eval $(call KernelPackage,ltq-gpon-optic))
