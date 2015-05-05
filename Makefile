include $(TOPDIR)/rules.mk
PKG_NAME:=pic-update
PKG_RELEASE:=1.0
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

define Package/pic-update
SECTION:=utils
CATEGORY:=Utilities
TITLE:=pic update usage
endef

define Package/pic-update/description
If you can’t figure out what this program does, you’re probably
brain-dead and need immediate medical attention.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) CFLAGS="$(TARGET_CFLAGS) -I$(LINUX_DIR)/include"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/pic-update.init $(1)/etc/init.d/pic-update
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/pic-update.config $(1)/etc/config/pic-update
	$(INSTALL_CONF) ./files/miner_pic.hex $(1)/etc/config/miner_pic.hex
endef

# This line executes the necessary commands to compile our program.
# The above define directives specify all the information needed, but this
# line calls BuildPackage which in turn actually uses this information to
# build a package.
$(eval $(call BuildPackage,$(PKG_NAME)))
