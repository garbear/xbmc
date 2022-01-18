-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=qrencode
VERSION=4.1.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CMAKE_OPTIONS := \
  -DWITH_TOOLS=NO \
  $(CMAKE_OPTIONS)

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  CXXFLAGS += -std=c++17
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
    CMAKE := cmake -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_PREFIX_PATH=$(PREFIX)
  endif
endif

# TODO
BASE_URL=https://fukuchi.org/works/qrencode

LIBDYLIB=$(PLATFORM)/build/libqrencode.a

.PHONY: .installed-native

all: .installed-$(PLATFORM)

download: $(TARBALLS_LOCATION)/$(ARCHIVE)

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
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
