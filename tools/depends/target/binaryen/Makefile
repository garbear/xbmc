-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=libbinaryen
VERSION=87
SOURCE=version_$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	# TODO
	#BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	BASE_URL := https://github.com/WebAssembly/binaryen/archive
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
	CMAKE := cmake
	override CMAKE_OPTIONS += -DCMAKE_INSTALL_PREFIX=$(PREFIX)
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	BASE_URL := https://github.com/WebAssembly/binaryen/archive
endif

override CMAKE_OPTIONS += -DBUILD_STATIC_LIB=ON

BUILDDIR = $(PLATFORM)/build

LIBDYLIB=$(BUILDDIR)/lib/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	#cd $(PLATFORM); patch -p1 < ../../../native/flatbuffers-native/0001-Fix-compiler-warning.patch
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

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
