################################################################################
# Protocol Buffers - Google's data intnerchange format
#
#   https://github.com/protocolbuffers/protobuf
#   SPDX-License-Identifier: BSD-3-Clause
#
################################################################################

-include ../../Makefile.include
DEPS = ../../Makefile.include Makefile

# lib name, version
LIBNAME=protobuf
LANG=cpp
VERSION=3.14.0-4a09d77-p0
# VERSION=3.19.4-p0
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    # Building stand-alone
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar --strip-components=1 -xf
  endif
endif

# TODO
BASE_URL=https://github.com/cpp-pm/protobuf/archive/refs/tags

# The build directory
BUILD_DIR=$(PLATFORM)/build-cmake

# The generated library
LIBDYLIB=$(BUILD_DIR)/lib$(LIBNAME).a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DBUILD_SHARED_LIBS=OFF \
  -DHUNTER_ENABLED=OFF \
  -Dprotobuf_BUILD_TESTS=OFF \
  -Dprotobuf_BUILD_PROTOC_BINARIES=OFF \
  $(CMAKE_OPTIONS)

# Fix missing log dependency when linking '__android_log_write'
# https://github.com/protocolbuffers/protobuf/issues/2719
ifeq ($(OS),android)
  LDFLAGS := $(LDFLAGS) -llog
endif

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
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILD_DIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILD_DIR) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
