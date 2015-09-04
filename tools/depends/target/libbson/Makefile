-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=libbson
VERSION=1.1.10
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL := https://github.com/mongodb/libbson/releases/download/1.1.10

LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

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

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); ./configure
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	@[ -d $(PREFIX)/include ] || mkdir -p $(PREFIX)/include
	@[ -d $(PREFIX)/lib ] || mkdir $(PREFIX)/lib
	cp -r $(PLATFORM)/src/bson/*.h $(PREFIX)/include/
	cp $(LIBDYLIB) $(PREFIX)/lib/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
