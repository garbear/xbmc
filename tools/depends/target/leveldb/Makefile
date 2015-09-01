-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=leveldb
VERSION=v1.18
SOURCE=$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL := https://github.com/google/leveldb/archive


LIBDYLIB=$(PLATFORM)/lib$(LIBNAME).a

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
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	@[ -d $(PREFIX)/include ] || mkdir -p $(PREFIX)/include
	@[ -d $(PREFIX)/lib ] || mkdir $(PREFIX)/lib
	cp -r $(PLATFORM)/include/leveldb $(PREFIX)/include/
	cp $(LIBDYLIB) $(PREFIX)/lib/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
