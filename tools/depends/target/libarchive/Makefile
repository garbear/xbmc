################################################################################
#
# libarchive - Multi-format archive and compression library
#
#   https://www.libarchive.org
#   SPDX-License-Identifier: BSD-2-Clause
#
################################################################################

-include ../../Makefile.include
DEPS = Makefile ../../Makefile.include

# lib name, version
LIBNAME=libarchive
VERSION=3.4.3
SOURCE=hunter-v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

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
BASE_URL := https://github.com/soramitsu/fuhon-libarchive/archive

# The build directory
BUILDDIR=$(PLATFORM)/build

# The generated library
LIBDYLIB=$(BUILDDIR)/$(LIBNAME).a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DENABLE_MBEDTLS=OFF \
  -DENABLE_NETTLE=OFF \
  -DENABLE_OPENSSL=ON \
  -DENABLE_LIBB2=OFF \
  -DENABLE_LZ4=ON \
  -DENABLE_LZO=ON \
  -DENABLE_LZMA=ON \
  -DENABLE_ZSTD=OFF \
  -DENABLE_ZLIB=ON \
  -DENABLE_BZip2=ON \
  -DENABLE_LIBXML2=ON \
  -DENABLE_EXPAT=ON \
  -DENABLE_PCREPOSIX=ON \
  -DENABLE_LibGCC=ON \
  -DENABLE_CNG=ON \
  -DENABLE_TAR=OFF \
  -DENABLE_TAR_SHARED=FALSE \
  -DENABLE_CPIO=OFF \
  -DENABLE_CPIO_SHARED=FALSE \
  -DENABLE_CAT=ON \
  -DENABLE_CAT_SHARED=FALSE \
  -DENABLE_XATTR=ON \
  -DENABLE_ACL=ON \
  -DENABLE_ICONV=ON \
  -DENABLE_TEST=OFF \
  -DENABLE_COVERAGE=FALSE \
  -DENABLE_INSTALL=ON \
  $(CMAKE_OPTIONS)

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
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
