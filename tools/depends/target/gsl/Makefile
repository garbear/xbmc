################################################################################
# GSL: Guidelines Support Library
#
#   https://github.com/microsoft/GSL
#   SPDX-License-Identifier: MIT
#
# The Guidelines Support Library (GSL) contains functions and types that are
# suggested for use by the C++ Core Guidelines maintained by the Standard C++
# Foundation.
#
################################################################################

-include ../../Makefile.include
DEPS = ../../Makefile.include Makefile \
  0001-Make-library-importable-by-CMake-using-find_package.patch \
  0002-Add-Microsoft.GSL-GSL-alias-for-GSL.patch \
  0003-CMakeLists-add-VERSION-to-project-and-package.patch \
  0004-Fix-CMake-config-file-name.patch \

# lib name, version
LIBNAME=gsl
VERSION=2.1.0
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# The build directory
BUILDDIR=$(PLATFORM)/build

# The generated library
LIBDYLIB=$(PLATFORM)/lib$(LIBNAME).a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DGSL_TEST=OFF \
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
BASE_URL := https://github.com/microsoft/GSL/archive

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
	cd $(PLATFORM); patch -p1 -i ../0001-Make-library-importable-by-CMake-using-find_package.patch
	cd $(PLATFORM); patch -p1 -i ../0002-Add-Microsoft.GSL-GSL-alias-for-GSL.patch
	cd $(PLATFORM); patch -p1 -i ../0003-CMakeLists-add-VERSION-to-project-and-package.patch
	cd $(PLATFORM); patch -p1 -i ../0004-Fix-CMake-config-file-name.patch

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
