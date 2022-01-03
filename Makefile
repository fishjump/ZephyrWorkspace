all:
	cmake -DBOARD=nrf52dk_nrf52832 -S app -B build
	# cmake -DBOARD=nrf52840dk_nrf52840 -S app -B build
	make -C build

# build_52:
# 	@west build -p auto -b nrf52dk_nrf52832 $(APP_DIR)

# build_52840:
# 	@west build -p auto -b nrf52840dk_nrf52840 $(APP_DIR)

flash:
	make flash -C build

clean:
	rm -r build