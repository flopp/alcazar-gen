THIS_MAKEFILE   = $(lastword $(MAKEFILE_LIST))
BASE_DIR        = $(dir $(realpath $(THIS_MAKEFILE)))
BUILD_DIR       = $(BASE_DIR)build

.PHONY: all veryclean config

all: $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) $@
veryclean:
	@echo "-- Cleaning up"
	rm -rf $(BUILD_DIR) bin
	rm -rf $$(find $(BASE_DIR) -name "*~")
config: $(BUILD_DIR)
	$(MAKE) edit_cache
$(BUILD_DIR):
	@echo "-- Creating build directory: $@"
	mkdir -p $@
	(cd $@; cmake $(BASE_DIR))

