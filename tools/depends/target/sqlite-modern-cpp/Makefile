################################################################################
# Lightweight modern wrapper around sqlite C API.
#
#   https://github.com/SqliteModernCpp/sqlite_modern_cpp
#   SPDX-License-Identifier: MIT
#
################################################################################

-include ../../Makefile.include
DEPS= \
  ../../Makefile.include \
  Makefile \
  SQLiteModernCppConfig.cmake \
  SQLiteModernCppTargets.cmake \

# lib name, version
LIBNAME=sqlite_modern_cpp
VERSION=db7d8ac70b0c191119bd415113180ff08ae460f8
SOURCE=$(VERSION)
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
BASE_URL := https://github.com/SqliteModernCpp/sqlite_modern_cpp/archive

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

.installed-$(PLATFORM): $(PLATFORM)
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/lib/cmake/SQLiteModernCpp

	# There is nothing to be compiled
	cp -r $(PLATFORM)/hdr/* $(PREFIX)/include/
	cp SQLiteModernCppConfig.cmake $(PREFIX)/lib/cmake/SQLiteModernCpp/
	cp SQLiteModernCppTargets.cmake $(PREFIX)/lib/cmake/SQLiteModernCpp/

	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
