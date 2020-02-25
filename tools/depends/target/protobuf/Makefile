-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=protobuf
LANG=cpp
VERSION=3.11.4
SOURCE=$(LIBNAME)-$(LANG)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# The compiled library
LIBDYLIB=$(PLATFORM)/src/lib$(LIBNAME).la

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	CONFIGURE_OPTIONS := \
	  --with-protoc=$(NATIVEPREFIX)/bin/protoc \
	  $(CONFIGURE_OPTIONS)
endif

# TODO
BASE_URL=https://github.com/protocolbuffers/protobuf/releases/download/v$(VERSION)

CONFIGURE_OPTIONS := \
  --prefix=$(PREFIX) \
  --disable-maintainer-mode \
  --disable-shared \
  $(CONFIGURE_OPTIONS)

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

$(PLATFORM)/Makefile: $(PLATFORM)
	cd $(PLATFORM); LDFLAGS="$(LDFLAGS)" ./configure $(CONFIGURE_OPTIONS)

$(LIBDYLIB): $(PLATFORM)/Makefile
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
