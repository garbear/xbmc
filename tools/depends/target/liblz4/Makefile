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

include ../../Makefile.include LIBLZ4-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBLZ4-VERSION ../../download-files.include

# Build configuration
CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_STATIC_LIBS=ON \
  -DLZ4_BUNDLED_MODE=OFF \
  -DLZ4_BUILD_CLI=OFF \
  -DLZ4_BUILD_LEGACY_LZ4C=OFF \
  $(CMAKE_OPTIONS)

# Build directory
BUILDDIR=$(PLATFORM)/build-liblz4

# Generated library
LIBDYLIB=$(BUILDDIR)/$(BYPRODUCT)

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
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ../build/cmake
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
