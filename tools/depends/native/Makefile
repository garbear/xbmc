include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

NATIVE= \
  autoconf \
  automake \
  cmake \
  distutilscross \
  flatbuffers \
  gas-preprocessor \
  gettext \
  giflib \
  heimdal \
  JsonSchemaBuilder \
  libffi \
  libjpeg-turbo \
  liblzo2 \
  libpng \
  libtool \
  m4 \
  meson \
  nasm \
  ninja \
  pcre swig \
  pkg-config \
  python3 \
  setuptools \
  TexturePacker \
  yasm \
  zlib

ifeq ($(OS),darwin_embedded)
  NATIVE += dpkg xz tar ldid
endif

ifeq ($(OS),linux)
  NATIVE += expat wayland-scanner pugixml waylandpp-scanner
  EXPAT = expat

  ifeq ($(RENDER_SYSTEM),gles)
    NATIVE += MarkupSafe Mako
  endif
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
autoconf: m4
automake: autoconf
distutilscross: python3
dpkg: automake gettext libtool pkg-config tar
flatbuffers: cmake
heimdal: libtool
libjpeg-turbo: cmake yasm
libpng: zlib automake
libtool: automake
meson: python3 setuptools
ninja: meson
pugixml: cmake
python3: $(EXPAT) libffi pkg-config zlib
setuptools: python3
swig: pcre
tar: xz automake
wayland-scanner: expat pkg-config
waylandpp-scanner: cmake pugixml

# python installs are not thread safe when using easy_install method.
# MarkupSafe doesn't really depend on ninja but we need to make the
# build sequential
MarkupSafe: ninja
Mako: MarkupSafe

#liblzo2 has stale packaged automake files that cause borked host/build detection
liblzo2: automake
JsonSchemaBuilder: automake
TexturePacker: automake pkg-config libpng liblzo2 giflib libjpeg-turbo

native: $(NATIVE)
$(NATIVE):
	$(MAKE) -C $@
clean:
	for d in $(NATIVE); do $(MAKE) -C $$d clean; done

# Debug target, this will DELETE all data in staging!
test-dependencies:
	( for d in $(NATIVE); do \
	rm -rf $(NATIVEPREFIX); \
        mkdir -p $(NATIVEPREFIX)/include $(NATIVEPREFIX)/share $(NATIVEPREFIX)/bin; \
	cp -f config.site $(NATIVEPREFIX)/share/; \
	$(MAKE) distclean; \
	$(MAKE) $$d; done ) && echo "$@ built successfully"

distclean::
	for d in $(NATIVE); do $(MAKE) -C $$d distclean; done

