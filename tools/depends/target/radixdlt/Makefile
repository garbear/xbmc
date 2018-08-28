-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=radixdlt-java
VERSION=0.9.3
SOURCE=$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
else
	# Building as part of depends
	DEPS += ../../Makefile.include
endif

# TODO
REPO=-b fat-jar https://github.com/juztamau5/radixdlt-java.git

JAR_PATH=radixdlt-java/build/libs/$(LIBNAME)-$(VERSION).jar

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)
	git clone $(REPO) $(PLATFORM)

$(JAR_PATH): $(PLATFORM)
	cd $(PLATFORM); gradle --build-cache build -x test -Pversion=$(VERSION)

.installed-$(PLATFORM): $(JAR_PATH)
	mkdir -p $(PREFIX)/lib
	cd $(PLATFORM); cp $(JAR_PATH) $(PREFIX)/lib
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
