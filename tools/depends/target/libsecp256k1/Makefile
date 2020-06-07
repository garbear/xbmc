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
DEPS = ../../Makefile.include Makefile \
  0001-CMake-Add-install-target.patch \

# lib name, version
LIBNAME=libsecp256k1
VERSION=f09001e0081fd7709323c689e114f6301437cb2b
SOURCE=$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# The build directory
BUILDDIR=$(PLATFORM)/build

# The generated library
# TODO
LIBDYLIB=$(BUILDDIR)/core/storage/ipfs/libipfs_blockservice.a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  $(CMAKE_OPTIONS)

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
BASE_URL := https://github.com/Alexey-N-Chernyshov/libsecp256k1/archive

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
	cd $(PLATFORM); patch -p1 -i ../0001-CMake-Add-install-target.patch

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
