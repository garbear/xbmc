################################################################################
# Protocol Buffers - Google's data interchange format
#
#   https://github.com/protocolbuffers/protobuf
#   SPDX-License-Identifier: BSD-3-Clause
#
################################################################################

include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=protobuf
LANG=cpp
VERSION=3.14.0-4a09d77-p0
# VERSION=3.19.4-p0
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# TODO
BASE_URL=https://github.com/cpp-pm/protobuf/archive/refs/tags

# The build directory
BUILD_DIR=$(PLATFORM)/build-cmake

APP=$(BUILD_DIR)/cmake/protoc

#
# There is no separate toolchain file for building on native at the moment, so
# we have to unset the CMAKE_TOOLCHAIN_FILE, which is part of $(CMAKE).
# Otherwise, cmake would use the target toolchain file.
#
CMAKE_OPTIONS := \
  -DCMAKE_TOOLCHAIN_FILE= \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DCMAKE_C_COMPILER="$(CC_BINARY_FOR_BUILD)" \
  -DCMAKE_CXX_COMPILER="$(CXX_BINARY_FOR_BUILD)" \
  -DCMAKE_C_FLAGS="$(NATIVE_CFLAGS)" \
  -DCMAKE_CXX_FLAGS="$(NATIVE_CXXFLAGS)" \
  -DCMAKE_EXE_LINKER_FLAGS="$(NATIVE_LDFLAGS)" \
  -DBUILD_SHARED_LIBS=OFF \
  -DHUNTER_ENABLED=OFF \
  -Dprotobuf_BUILD_TESTS=OFF \
  $(CMAKE_OPTIONS)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(APP): $(PLATFORM)
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILD_DIR)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(BUILD_DIR) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
