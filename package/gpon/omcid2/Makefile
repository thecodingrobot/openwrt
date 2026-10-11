# Copyright (C) 2026 thecodingrobot
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=omcid2
PKG_VERSION:=0.3.0
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-or-later

# Clean-room OMCI daemon for Lantiq Falcon GPON sticks (M0-M3 complete:
# codec + MIB engine + driver adapter + simulated OLT). Sources vendored
# in-tree; MIB seed profiles from the stick reference dumps.

include $(INCLUDE_DIR)/package.mk

define Package/omcid2
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Clean-room OMCI daemon for Lantiq Falcon GPON sticks (experimental)
  DEPENDS:=@(TARGET_lantiq_falcon)
endef

define Package/omcid2/description
  Clean-room userspace OMCI daemon for the Lantiq Falcon GPON SFP
  sticks. Currently implements the G.988 codec, MIB engine with .ini
  seed profiles, OMCC engine and a driver adapter with a mock backend
  plus the real ioctl client for the GPL gpon_onu driver.

  Experimental: the daemon is not yet feature-complete (provisioning
  managed entities pending); the omcid2_sim simulator and self-test
  binaries are installed for on-target validation.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src $(PKG_BUILD_DIR)/src
	$(CP) ./mibs $(PKG_BUILD_DIR)/mibs
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		CC="$(TARGET_CC)" AR="$(TARGET_AR)" \
		CFLAGS="$(TARGET_CFLAGS) -std=c99 -I. -Iomci -Iplatform -Icore" \
		GPON_DRV_INC="$(CURDIR)/../gpon-tools/src/drv-headers/onu" \
		IFXOS_INC="$(CURDIR)/../gpon-tools/src/drv-headers/ifxos" \
		all sim tests/test_codec tests/test_ini tests/test_omcc check-ioctl
endef

define Package/omcid2/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/usr/share/omcid2/mibs
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/sim/omcid2_sim $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tests/test_codec $(1)/usr/bin/omcid2-test-codec
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tests/test_ini $(1)/usr/bin/omcid2-test-ini
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tests/test_omcc $(1)/usr/bin/omcid2-test-omcc
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/mibs/*.ini $(1)/usr/share/omcid2/mibs/
endef

$(eval $(call BuildPackage,omcid2))
