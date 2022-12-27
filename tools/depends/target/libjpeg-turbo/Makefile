include ../../Makefile.include LIBJPEG-TURBO-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBJPEG-TURBO-VERSION Makefile ../../download-files.include \
                                  01-disable-executables.patch

CMAKE_ARGS=-B build \
           -DCMAKE_ASM_NASM_COMPILER:FILEPATH=$(NATIVEPREFIX)/bin/nasm \
           -DENABLE_SHARED:BOOL=OFF \
           -DWITH_JPEG8:BOOL=ON

ifeq ($(OS),android)
  ifeq ($(CPU),i686)
    CMAKE_ARGS+= -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  endif
endif

LIBDYLIB=$(PLATFORM)/build/libjpeg.a

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-executables.patch
	cd $(PLATFORM); $(CMAKE) $(CMAKE_ARGS)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
