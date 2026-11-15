################################################################################
# General project settings                                                     #
################################################################################

PROJECTNAME := bt_host
PROJ_DIR := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SDK_DIR := $(realpath $(PROJ_DIR)/../../../..)


################################################################################
# Components                                                                   #
#                                                                              #
# After setting up the toolchain, components can contribute to the project by  #
# appending items to the project variables like INCLUDEPATHS or C_SRC.         #
################################################################################

ENABLE_DYNAMIC_LINKING := 1
include $(SDK_DIR)/app/bluetooth/component_host/toolchain.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_log.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_assert.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_signal.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_timer.mk
include $(SDK_DIR)/app/bluetooth/component_host/ncp_host_bt.mk
include $(SDK_DIR)/app/bluetooth/component_host/ncp_gatt.mk
include $(SDK_DIR)/app/bluetooth/component_host/ncp_version.mk


################################################################################
# Include paths                                                                #
################################################################################

override INCLUDEPATHS += $(PROJ_DIR) \
$(SDK_DIR)/app/bluetooth/common_host/system \
$(SDK_DIR)/platform/common/inc \
$(SDK_DIR)/protocol/bluetooth/inc \
$(SDK_DIR)/protocol/bluetooth/config \
$(PROJ_DIR)/autogen


################################################################################
# Input files                                                                  #
################################################################################

override C_SRC += \
$(SDK_DIR)/app/bluetooth/common_host/system/system.c \
$(PROJ_DIR)/autogen/gatt_db.c \
$(PROJ_DIR)/app.c \
$(PROJ_DIR)/app_builder.c \
$(PROJ_DIR)/ble.c \
$(PROJ_DIR)/model.c \
$(PROJ_DIR)/main.c \
$(PROJ_DIR)/cJSON.c \
$(PROJ_DIR)/gui/gui.c \
$(PROJ_DIR)/gui/window/main_window.c \
$(PROJ_DIR)/gui/pages/overview.c \
$(PROJ_DIR)/gui/pages/modules.c \
$(PROJ_DIR)/gui/pages/installer.c \
$(PROJ_DIR)/gui/pages/settings.c \
$(PROJ_DIR)/gui/widgets/module_card.c \
$(PROJ_DIR)/gui/widgets/status_indicator.c \
$(PROJ_DIR)/gui/widgets/log_viewer.c \
$(PROJ_DIR)/modules/spotify/spotify.c 

GATT_XML_FILES += $(PROJ_DIR)/config/btconf/gatt_configuration.btconf

################################################################################
# Target rules                                                                 #
################################################################################

override CFLAGS += -DCURL_STATICLIB

# GTK4
override CFLAGS += $(shell pkg-config --cflags gtk4)
override LDFLAGS += $(shell pkg-config --libs gtk4)

# Curl
override LDFLAGS += $(shell pkg-config --libs --static libcurl)

# OpenSSL / Windows
override LDFLAGS += -lssl -lcrypto -lws2_32

include $(SDK_DIR)/app/bluetooth/component_host/targets.mk
