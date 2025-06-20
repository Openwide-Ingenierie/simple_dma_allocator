MODULE_NAME ?= my_dma_allocator
APP_NAME ?= my_rpmsg_test

BUILD_DIR := $(PWD)/build
BUILD_DIR_MAKEFILE := $(BUILD_DIR)/Makefile

obj-m += $(addsuffix .o, $(MODULE_NAME))

all: $(BUILD_DIR_MAKEFILE) app module

clean:
	rm -rf $(BUILD_DIR)

module:
ifndef KDIR  				# Kernel source path
	$(error KDIR not set)
endif
ifndef CROSS_COMPILE  		# Target arch prefix, ex : aarch64-linux-
	$(error CROSS_COMPILE not set)
endif
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) CROSS_COMPILE=$(CROSS_COMPILE) src=$(PWD) modules

app:
ifndef CROSS_COMPILER  		# Path to cross compiler for target, ex : armgcc
	$(error CROSS_COMPILER not set)
endif
	$(CROSS_COMPILER) $(PWD)/$(APP_NAME).c -Wall -o $(BUILD_DIR)/$(APP_NAME)

$(BUILD_DIR):
	@mkdir -p "$@"

$(BUILD_DIR_MAKEFILE): $(BUILD_DIR)
	@touch "$@"

.PHONY: clean all app