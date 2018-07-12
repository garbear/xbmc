-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=liblmdb
VERSION=0.9.24
SOURCE=LMDB_$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL := https://github.com/LMDB/lmdb/archive

BUILDDIR=$(PLATFORM)/libraries/liblmdb
LIBDYLIB=$(BUILDDIR)/$(LIBNAME).a

MAKE_OPTIONS = \
  CC="$(CC)" \
  AR="$(AR)" \
  XCFLAGS="$(CFLAGS)" \
  prefix="$(PREFIX)" \

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

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR) $(MAKE_OPTIONS)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) $(MAKE_OPTIONS) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
