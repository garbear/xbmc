-include ../../Makefile.include
DEPS=Makefile \
	0001-Don-t-err-on-warnings.patch \
	0002-Autoconf-Relax-parameter-restrictions.patch \
	0003-Fix-error-on-parallel-make.patch

# Added depends:
#sudo apt-get install libx11-dev libxext-dev libxrender-dev libxtst-dev libxt-dev
#sudo apt-get install libcups2-dev

# lib name, version
LIBNAME=jdk
VERSION=12+8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

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
endif

# TODO
#BASE_URL=http://hg.openjdk.java.net/jdk/jdk/archive
BASE_URL=https://github.com/AdoptOpenJDK/openjdk-jdk/archive

# configuration settings
#--openjdk-target=x86_64
CONFIGURE=configure
CONFIGURE += --prefix="$(PREFIX)"
CONFIGURE += --with-extra-cflags="$(CFLAGS)"
CONFIGURE += --with-extra-cxxflags="$(CXXFLAGS)"
CONFIGURE += --with-extra-ldflags="$(LDFLAGS)"
CONFIGURE += --with-toolchain-path="$(TOOLCHAIN)"
CONFIGURE += AR="$(AR)"
CONFIGURE += NM="$(NM)"
CONFIGURE += OBJCOPY="$(OBJCOPY)"
CONFIGURE += OBJDUMP="$(OBJDUMP)"
CONFIGURE += STRIP="$(STRIP)"
CONFIGURE += BUILD_CC="$(CC)"
CONFIGURE += BUILD_CXX="$(CXX)"
CONFIGURE += BUILD_AR="$(AR)"
CONFIGURE += BUILD_NM="$(NM)"
CONFIGURE += BUILD_OBJCOPY="$(OBJCOPY)"
CONFIGURE += BUILD_STRIP="$(STRIP)"

# TODO: Better way to detect debug build
ifneq (,$(findstring _DEBUG,$(CFLAGS)))
	CONFIGURE += --enable-debug
else
	CONFIGURE += --disable-debug
endif

ifeq ($(OS),android)
	CONFIGURE += --with-toolchain-type=clang
	CONFIGURE += --openjdk-target=aarch64-linux-android
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
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../0001-Don-t-err-on-warnings.patch
	cd $(PLATFORM); patch -p1 < ../0002-Autoconf-Relax-parameter-restrictions.patch
	cd $(PLATFORM); patch -p1 < ../0003-Fix-error-on-parallel-make.patch
	cd $(PLATFORM); patch -p1 < ../0004-Don-t-install-bin-directory.patch
	cd $(PLATFORM); patch -p1 < ../0005-Autoconf-Add-Android-OS-to-platform-detection.patch

$(PLATFORM)/build: $(PLATFORM)
	rm -rf $(PLATFORM)/build
	cd $(PLATFORM); bash $(CONFIGURE)

.installed-$(PLATFORM): $(PLATFORM)/build
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
