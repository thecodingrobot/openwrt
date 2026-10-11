define Device/lantiq_easy88388
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := EASY88388 Falcon FTTDP8 Reference Board
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy88388

define Device/lantiq_easy88444
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := EASY88444 Falcon FTTdp G.FAST Reference Board
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy88444

define Device/lantiq_easy98020
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon SFU Reference Board (EASY98020)
  DEVICE_VARIANT := v1.0-v1.7
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy98020

define Device/lantiq_easy98020-v18
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon SFU Reference Board (EASY98020)
  DEVICE_VARIANT := v1.8
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy98020-v18

define Device/lantiq_easy98021
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon HGU Reference Board (EASY98021)
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy98021

define Device/lantiq_easy98035synce
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon SFP Stick (EASY98035SYNCE)
  DEVICE_VARIANT := with Synchronous Ethernet
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy98035synce

define Device/lantiq_easy98035synce1588
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon SFP Stick (EASY98035SYNCE1588)
  DEVICE_VARIANT := with SyncE and IEEE1588
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_easy98035synce1588

define Device/lantiq_easy98000-nand
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := EASY98000 Falcon Eval Board
  DEVICE_VARIANT := NAND
  IMAGE_SIZE := 3904k
  DEVICE_PACKAGES := kmod-dm9000 kmod-i2c-lantiq kmod-eeprom-at24
  DEFAULT := n
endef
TARGET_DEVICES += lantiq_easy98000-nand

define Device/lantiq_easy98000-nor
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := EASY98000 Falcon Eval Board
  DEVICE_VARIANT := NOR
  IMAGE_SIZE := 3904k
  DEVICE_PACKAGES := kmod-dm9000 kmod-i2c-lantiq kmod-eeprom-at24
  DEFAULT := n
endef
TARGET_DEVICES += lantiq_easy98000-nor

define Device/lantiq_easy98000-sflash
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := EASY98000 Falcon Eval Board
  DEVICE_VARIANT := SFLASH
  IMAGE_SIZE := 7424k
  DEVICE_PACKAGES := kmod-dm9000 kmod-i2c-lantiq kmod-eeprom-at24
endef
TARGET_DEVICES += lantiq_easy98000-sflash

define Device/lantiq_falcon-mdu
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon / VINAXdp MDU Board
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_falcon-mdu

define Device/lantiq_falcon-sfp
  DEVICE_VENDOR := Lantiq
  DEVICE_MODEL := Falcon SFP Stick
  IMAGE_SIZE := 7424k
endef
TARGET_DEVICES += lantiq_falcon-sfp

define Device/sourcephotonics_sps-34-24t-hpdfo
  DEVICE_VENDOR := SourcePhotonics
  DEVICE_MODEL := SPS-34-24T-HP-TDFO
  DEVICE_ALT0_VENDOR := FS.com
  DEVICE_ALT0_MODEL := GPON-ONU-34-20BI (GPON-SFP-ONT-MAC-I)
  DEVICE_DTS := falcon-sfp
  IMAGE_SIZE := 7424k
  DEVICE_PACKAGES := kmod-ltq-gpon-onu kmod-ltq-gpon-optic gpon-tools
endef
TARGET_DEVICES += sourcephotonics_sps-34-24t-hpdfo

define Device/huawei_ma5671a
  DEVICE_VENDOR := Huawei
  DEVICE_MODEL := MA5671A
  DEVICE_DTS := falcon-sfp
  IMAGE_SIZE := 7424k
  DEVICE_PACKAGES := kmod-ltq-gpon-onu kmod-ltq-gpon-optic gpon-tools
endef
TARGET_DEVICES += huawei_ma5671a

define Device/nokia_g-010s-p
  DEVICE_VENDOR := Nokia
  DEVICE_MODEL := G-010S-P
  DEVICE_DTS := falcon-sfp
  IMAGE_SIZE := 7424k
  DEVICE_PACKAGES := kmod-ltq-gpon-onu kmod-ltq-gpon-optic gpon-tools
endef
TARGET_DEVICES += nokia_g-010s-p
