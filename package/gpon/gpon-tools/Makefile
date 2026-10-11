# Copyright (C) 2026 thecodingrobot
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=gpon-tools
PKG_VERSION:=4.5.0
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-or-later BSD-2-Clause

# Sources vendored in-tree from the Lantiq UGW SDK driver tarballs
# (gpon_onu_drv / gpon_optic_drv 4.5.0, dual BSD/GPL) - the GPL companion
# CLI tools of the kernel drivers.

include $(INCLUDE_DIR)/package.mk

define Package/gpon-tools
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lantiq Falcon GPON ONU control tools
  DEPENDS:=@(TARGET_lantiq_falcon)
endef

define Package/gpon-tools/description
  Userspace CLI tools for the Lantiq Falcon GPON SFP sticks:
   - onu   : ONU driver control CLI (/dev/onu0 ioctls)
   - gtop  : ONU debug TUI (GTC/GPE/SCE counters)
   - otop  : optic (BOSA/DCDC) debug TUI (/dev/optic0)
  Built from the GPL driver sources (tools/ subdirs).
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		LD="$(TARGET_LD)" \
		CFLAGS="$(TARGET_CFLAGS) -DLINUX -DHAVE_CONFIG_H -w"
endef

define Package/gpon-tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/onu/onu $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gtop/gtop $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/otop/otop $(1)/usr/bin/
endef

$(eval $(call BuildPackage,gpon-tools))
