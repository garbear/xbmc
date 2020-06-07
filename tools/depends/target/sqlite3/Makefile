include ../../Makefile.include
DEPS = ../../Makefile.include Makefile sqlite3.c.patch ../../download-files.include

# lib name, version
LIBNAME=sqlite
VERSION=v3.27.2-p0
SOURCE=$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=110733e3857f64b7765a20d703f72de9ae8e08bdbe6b34091f9e95bfbc5d94a2ad4ec4d073f95829c6ff5980132d68d6c10a6caad8f5f894276f2f1594961fb2
include ../../download-files.include

# TODO
BASE_URL=https://github.com/hunter-packages/sqlite3/archive

# configuration settings
export CXXFLAGS+=-DSQLITE_ENABLE_COLUMN_METADATA=1
export CFLAGS+=-DSQLITE_TEMP_STORE=3 -DSQLITE_DEFAULT_MMAP_SIZE=0x10000000
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --enable-threadsafe --disable-readline

# The build directory
BUILDDIR=$(PLATFORM)/build

LIBDYLIB=$(BUILDDIR)/lib$(LIBNAME)3.a

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  $(CMAKE_OPTIONS)

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
# seems MAP_POPULATE is broken on aarch64
ifneq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../sqlite3.c.patch
endif

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

