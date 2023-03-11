.PHONY: build install

INSTALL_DIR=$(HOME)/.mellow/bin
MELLOW="`./find-mellow.sh`"

build:
	$(MELLOW) build

install:
	$(MELLOW) build --profile release
	mkdir -p $(INSTALL_DIR)
	cp build/release/mellow/mellow "$(INSTALL_DIR)/"
	@echo "Mellow installed to $(INSTALL_DIR), make sure to include that path in your PATH env variable"

