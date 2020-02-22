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

include ../../Makefile.include
include LIBLZ4-VERSION
include ../../download-files.include

DEPS := \
  ../../download-files.include \
  ../../Makefile.include \
  LIBLZ4-VERSION \
  Makefile \

# Build configuration
CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_STATIC_LIBS=ON \
  -DLZ4_BUILD_CLI=OFF \
  -DLZ4_BUILD_LEGACY_LZ4C=OFF \
  -DLZ4_BUNDLED_MODE=OFF \
  $(CMAKE_OPTIONS) \

# Build directory
BUILDDIR := $(PLATFORM)/build-liblz4

# Generated library
LIBDYLIB := $(BUILDDIR)/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(BUILDDIR)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ../build/cmake

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
