PROGRAM=EtsySoldOrdersCounters
PROGRAM_SRC_DIR=./src
SERIAL_PORT=/dev/ttyUSB0
SERIAL_BAUD=115200

ESPPORT=$(SERIAL_PORT)
ESPBAUD=$(SERIAL_BAUD)

EXTRA_COMPONENTS = extras/bearssl
EXTRA_CFLAGS +=-DCONFIG_EPOCH_TIME=$(shell date --utc '+%s')

include ./lib/esp-open-rtos/common.mk

# Utility targets
.PHONY: dev bootstrap monitor

# Step in to containerized development environment (includes necessary dependencies)
DOCKER_IMG := bschwind/esp-open-rtos
dev:
	docker run \
		--rm \
		-it \
		--privileged \
		--env SKIP_DOCKER=true \
		--device=$(SERIAL_PORT) \
		--volume /dev/bus/usb:/dev/bus/usb \
		--volume $(PWD):/home/esp/esp-open-rtos/examples/project \
		$(DOCKER_IMG) \
		/bin/bash

bootstrap:
	git submodule update --init --recursive

monitor:
	picocom -b $(SERIAL_BAUD) $(SERIAL_PORT)
