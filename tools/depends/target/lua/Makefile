-include ../../Makefile.include
DEPS = Makefile 0001-Remove-readline-dependency.patch

# lib name, version
LIBNAME=lua
VERSION=5.3.5
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
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
BASE_URL := https://github.com/lua/lua/archive

LIBDYLIB=$(PLATFORM)/lib$(LIBNAME).a

.PHONY: .installed-native

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
	cd $(PLATFORM); patch -p1 < ../0001-Remove-readline-dependency.patch

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(MAKE) \
	  --file=makefile \
	  CC="$(CC)" \
	  CFLAGS="$(CFLAGS)" \
	  AR="$(AR) rcu" \
	  RANLIB="$(RANLIB)" \
	  RM="$(RM)"

.installed-$(PLATFORM): $(LIBDYLIB)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp $(LIBDYLIB) $(PREFIX)/lib
	cd $(PLATFORM); cp \
	  lauxlib.h \
	  lua.h \
	  luaconf.h \
	  $(PREFIX)/include
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

