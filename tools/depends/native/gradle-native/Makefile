include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=gradle
VERSION=4.10.0
SOURCE=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
#BASE_URL=http://mirrors.kodi.tv/build-deps/sources
BASE_URL=https://github.com/gradle/gradle/archive

WRAPPER=$(realpath .)/$(PLATFORM)/gradlew
BUILDDIR=$(realpath .)/$(PLATFORM)/target
APP=$(BUILDDIR)/bin/gradle

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(WRAPPER): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../0001-Fix-build-failure-due-to-deprecated-function.patch
	touch $(WRAPPER)

$(APP): $(WRAPPER)
	cd $(PLATFORM); $(WRAPPER) --build-cache install -Pgradle_installPath=$(BUILDDIR)

.installed-$(PLATFORM): $(APP)
	cp -r $(BUILDDIR)/* $(PREFIX)
	touch $@

clean:
	cd $(PLATFORM); git clean -xdf .
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
