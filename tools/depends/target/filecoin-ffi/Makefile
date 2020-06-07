################################################################################
# Fuhon - C++17 implementation of Filecoin
#
#   https://filecoin.io
#   SPDX-License-Identifier: GPL-3.0-or-later
#
# Fuhon is an implementation of the Filecoin protocol being developed in C++.
#
################################################################################

-include ../../Makefile.include
DEPS = ../../Makefile.include Makefile

# lib name, version
LIBNAME=filecoin-ffi
VERSION=proofs-v11-0-2
SOURCE=$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# The build directory
BUILDDIR=$(PLATFORM)/build

# The generated library
# TODO
LIBDYLIB=$(BUILDDIR)/core/storage/ipfs/libipfs_blockservice.a

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar --strip-components=1 -xf
  endif
endif

# TODO
BASE_URL := https://github.com/filecoin-project/filecoin-ffi/archive

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); ./install-filcrypto

.installed-$(PLATFORM): $(LIBDYLIB)
	mkdir -p $(PREFIX)/include/filecoin-ffi
	cp $(PLATFORM)/filcrypto.h $(PREFIX)/include/filecoin-ffi
	cp $(PLATFORM)/libfilcrypto.a $(PREFIX)/lib
	cp $(PLATFORM)/filcrypto.pc $(PREFIX)/lib/pkgconfig
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
