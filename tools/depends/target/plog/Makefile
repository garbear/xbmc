-include ../../Makefile.include
include PLOG-VERSION
DEPS = Makefile PLOG-VERSION ../../download-files.include

CMAKE_OPTIONS=-DPLOG_BUILD_SAMPLES=OFF -DPLOG_INSTALL=ON

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    #BASE_URL := http://mirrors.kodi.tv/build-deps/sources # TODO
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
    CMAKE := cmake
    CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_INSTALL_LIBDIR=lib $(CMAKE_OPTIONS)
    HASH_TOOL := sha512sum
  endif
endif

include ../../download-files.include

.PHONY: .installed-native

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
