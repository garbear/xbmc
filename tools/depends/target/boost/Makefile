################################################################################
# Boost C++ Libraries
#
#   https://www.boost.org/
#   SPDX-License-Identifier: BSL-1.0
#
# Boost is a set of libraries for the C++ programming language that provide
# support for common tasks.
#
################################################################################

-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=boost
VERSION=1.72.0
#SOURCE=$(LIBNAME)_$(shell echo $(VERSION) | tr "." "_")
SOURCE=v$(VERSION)-p0
ARCHIVE=$(SOURCE).tar.gz

# The comma-separated list of boost libraries to build
BOOST_LIBRARIES=random

# The generated library
LIBDYLIB=$(PLATFORM)/stage/lib/libboost_system.a

# Boost build options
BOOST_OPTIONS := \
  link=static \
  cflags="$(CFLAGS)" \
  cxxflags="$(CXXFLAGS)" \
  linkflags="$(LDFLAGS)" \

ifeq ($(DEBUG_BUILD),yes)
  BOOST_OPTIONS += variant=debug
else
  BOOST_OPTIONS += variant=release
endif

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include

  BOOST_OPTIONS += toolset=gcc-cross
  ifneq (,$(findstring arm64,$(CPU)))
    BOOST_OPTIONS += architecture=arm address-model=64
  else ifneq (,$(findstring arm,$(CPU)))
    BOOST_OPTIONS += architecture=arm address-model=32
  else ifeq ($(CPU),i686)
    BOOST_OPTIONS += architecture=x86 address-model=32
  else ifeq ($(CPU),x86_64)
    BOOST_OPTIONS += architecture=x86 address-model=64
  endif

  # TODO: FreeBSD (verify), iOS, tvOS
  ifeq ($(OS),linux)
    BOOST_OPTIONS += target-os=linux
  else ifeq ($(OS),android)
    BOOST_OPTIONS += target-os=android
  else ifeq ($(OS),osx)
    BOOST_OPTIONS += target-os=darwin
  else ifeq ($(OS),freebsd)
    BOOST_OPTIONS += target-os=freebsd
  endif
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
#BASE_URL := https://dl.bintray.com/boostorg/release/$(VERSION)/source
BASE_URL=https://github.com/cpp-pm/boost/archive

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

$(PLATFORM)/b2: $(PLATFORM)
	cd $(PLATFORM); ./bootstrap.sh \
	  --prefix=$(PREFIX) \
	  --with-python=$(NATIVEPREFIX)/bin/python3 \
	  --with-libraries=$(BOOST_LIBRARIES) \
	  --without-icu
ifeq ($(CROSS_COMPILING), yes)
	echo "using gcc : cross : $(CXX) ;" >> $(PLATFORM)/project-config.jam
endif

$(LIBDYLIB): $(PLATFORM)/b2
	cd $(PLATFORM); ./b2 $(BOOST_OPTIONS)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); ./b2 $(BOOST_OPTIONS) install
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
