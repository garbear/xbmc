include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=protobuf
LANG=cpp
VERSION=3.11.4
SOURCE=$(LIBNAME)-$(LANG)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
#BASE_URL=http://mirrors.kodi.tv/build-deps/sources
# TODO
BASE_URL=https://github.com/protocolbuffers/protobuf/releases/download/v$(VERSION)

APP=$(PLATFORM)/src/protoc

CONFIGURE_OPTIONS := \
  --prefix=$(PREFIX) \
  --disable-maintainer-mode \
  --disable-static \
  --disable-shared \

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(PLATFORM)/Makefile: $(PLATFORM)
	cd $(PLATFORM); ./configure $(CONFIGURE_OPTIONS)

$(APP): $(PLATFORM)/Makefile
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
