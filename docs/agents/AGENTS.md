# Agent Setup

## Collaboration

This document is a collaborative effort. If you find improvements for how
agents work with Kodi, please extend this guide and send the changes back to
us.

## Build Kodi

To build Kodi in this container you'll need numerous development packages. Install them with `apt`:

```bash
sudo apt update
sudo apt install -y \
  autoconf \
  automake \
  autopoint \
  autotools-dev \
  ccache \
  cmake \
  curl \
  debhelper \
  default-jre \
  doxygen \
  gawk \
  gcc \
  gdc \
  gettext \
  gperf \
  libasound2-dev \
  libass-dev \
  libavahi-client-dev \
  libavahi-common-dev \
  libbluetooth-dev \
  libbluray-dev \
  libbz2-dev \
  libcdio-dev \
  libcec-dev \
  libcrossguid-dev \
  libcurl4-openssl-dev \
  libcwiid-dev \
  libdbus-1-dev \
  libdisplay-info-dev \
  libdrm-dev \
  libegl1-mesa-dev \
  libenca-dev \
  libexiv2-dev \
  libflac-dev \
  libflatbuffers-dev \
  libfmt-dev \
  libfontconfig-dev \
  libfreetype6-dev \
  libfribidi-dev \
  libfstrcmp-dev \
  libgbm-dev \
  libgcrypt-dev \
  libgif-dev \
  libgl1-mesa-dev \
  libgles2-mesa-dev \
  libglew-dev \
  libglu1-mesa-dev \
  libgnutls28-dev \
  libgpg-error-dev \
  libgtest-dev \
  libinput-dev \
  libiso9660-dev \
  libjpeg-dev \
  liblcms2-dev \
  liblirc-dev \
  libltdl-dev \
  liblzo2-dev \
  libmicrohttpd-dev \
  libmysqlclient-dev \
  libnfs-dev \
  libogg-dev \
  libp8-platform-dev \
  libpcre2-dev \
  libplist-dev \
  libpng-dev \
  libpulse-dev \
  libshairplay-dev \
  libsmbclient-dev \
  libspdlog-dev \
  libsqlite3-dev \
  libssl-dev \
  libtag1-dev \
  libtiff5-dev \
  libtinyxml-dev \
  libtinyxml2-dev \
  libtool \
  libudev-dev \
  libunistring-dev \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libwayland-dev \
  libxkbcommon-dev \
  libxmu-dev \
  libxrandr-dev \
  libxslt1-dev \
  libxt-dev \
  lsb-release \
  meson \
  nasm \
  ninja-build \
  nlohmann-json3-dev \
  python3-dev \
  python3-pil \
  python3-pip \
  swig \
  unzip \
  uuid-dev \
  wayland-protocols \
  waylandpp-dev \
  zip \
  zlib1g-dev
```

Once dependencies are installed, configure Kodi with:

```bash
cmake -DAPP_RENDER_SYSTEM=gl -DENABLE_INTERNAL_FFMPEG=ON ..
```

## Coding guidelines

Follow these best practices when contributing to the Kodi code base:

- Keep the code easy to understand and well documented. Add comments that
  describe why the code exists and how it works. Poorly commented code will be
  rejected during review!
- New source files should begin with the standard Kodi copyright header using
  the current year:

  ```cpp
  /*
   *  Copyright (C) 2025 Team Kodi
   *  This file is part of Kodi - https://kodi.tv
   *
   *  SPDX-License-Identifier: GPL-2.0-or-later
   *  See LICENSES/README.md for more information.
   */
  ```

## Testing guidelines

Tests are part of the code base and should be just as easy to read as
production code. When writing unit tests:

- Document the intention of each test case with clear comments. A short
  paragraph or sentence explaining what behavior is being verified helps
  reviewers understand the purpose of the test.
- Comment any non-obvious setup or test data so that future contributors can
  quickly follow how the test works.
- Keep tests focused on a single behavior, avoiding unnecessary complexity.
- Begin each test with a short comment summarizing what it verifies. This
  makes automated summaries clearer for reviewers.
