.PHONY: all build flash build_52 build_52840

# BOARD = nrf52840dk_nrf52840
BOARD = nrf52dk_nrf52832
APP_DIR = app


all: build

build:
	@west build -p auto -b $(BOARD) $(APP_DIR)

build_52:
	@west build -p auto -b nrf52dk_nrf52832 $(APP_DIR)

build_52840:
	@west build -p auto -b nrf52840dk_nrf52840 $(APP_DIR)

flash:
	@west flash