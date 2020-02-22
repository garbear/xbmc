################################################################################
# LZ4 - Extremely Fast Compression algorithm
#
#   http://www.lz4.org
#   SPDX-License-Identifier: BSD-2-Clause
#
# LZ4 is lossless compression algorithm, providing compression speed and
# scalable with multi-cores CPU. It features an extremely fast decoder, with
# speed in multiple GB/s per core, typically reaching RAM speed limits on
# multi-core systems.
#
################################################################################

-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=lz4
VERSION=1.9.2
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# The generated library
LIBDYLIB=$(PLATFORM)/contrib/cmake_unofficial/lib$(LIBNAME).a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DLZ4_BUNDLED_MODE=OFF \
  -DBUILD_STATIC_LIBS=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DLZ4_BUILD_LEGACY_LZ4C=OFF \
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
BASE_URL := https://github.com/lz4/lz4/archive

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
	cd $(PLATFORM)/contrib/cmake_unofficial; cmake $(CMAKE_OPTIONS) CMakeLists.txt
	cd $(PLATFORM)/contrib/cmake_unofficial; $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/contrib/cmake_unofficial; $(MAKE) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
