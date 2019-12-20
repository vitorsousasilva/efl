#!/bin/sh

set -e
. .ci/travis.sh

if [ "$DISTRO" != "" ] ; then
  # Why do we need to disable the imf loaders here?
  OPTS=" -Decore-imf-loaders-disabler=scim,ibus"

  MONO_LINUX_COPTS=" -Dbindings=luajit,cxx,mono -Dmono-beta=true"

  WAYLAND_LINUX_COPTS=" -Dwl=true -Ddrm=true -Dopengl=es-egl -Dwl-deprecated=true -Ddrm-deprecated=true"

  # TODO:
  # - No libelogind package in fedora 30 repo
  # - RPM fusion repo for xine and libvlc
  # - Ibus
  ENABLED_LINUX_COPTS=" -Dfb=true -Dsdl=true -Dbuffer=true -Dbuild-id=travis-build \
  -Ddebug-threads=true -Dglib=true -Dg-mainloop=true -Dxpresent=true -Dxinput22=true \
  -Devas-loaders-disabler=json -Decore-imf-loaders-disabler= -Demotion-loaders-disabler=libvlc,xine \
  -Demotion-generic-loaders-disabler=vlc -Dharfbuzz=true -Dpixman=true -Dhyphen=true \
  -Dvnc-server=true -Dbindings=luajit,cxx,mono -Delogind=false -Dinstall-eo-files=true -Dphysics=true"

  # Enabled png, jpeg evas loader for in tree edje file builds
  DISABLED_LINUX_COPTS=" -Daudio=false -Davahi=false -Dx11=false -Dphysics=false -Deeze=false \
  -Dopengl=none -Deina-magic-debug=false -Dbuild-examples=false -Dbuild-tests=false \
  -Dcrypto=gnutls -Dglib=false -Dgstreamer=false -Dsystemd=false -Dpulseaudio=false \
  -Dnetwork-backend=connman -Dxinput2=false -Dtslib=false \
  -Devas-loaders-disabler=gst,pdf,ps,raw,svg,xcf,bmp,dds,eet,generic,gif,ico,jp2k,json,pmaps,psd,tga,tgv,tiff,wbmp,webp,xpm \
  -Decore-imf-loaders-disabler=xim,ibus,scim  -Demotion-loaders-disabler=gstreamer1,libvlc,xine \
  -Demotion-generic-loaders-disabler=vlc -Dfribidi=false -Dfontconfig=false \
  -Dedje-sound-and-video=false -Dembedded-lz4=false -Dlibmount=false -Dv4l2=false \
  -Delua=true -Dnls=false -Dbindings= -Dlua-interpreter=luajit -Dnative-arch-optimization=false"
  #evas_filter_parser.c:(.text+0xc59): undefined reference to `lua_getglobal' with interpreter lua

  RELEASE_READY_LINUX_COPTS=" --buildtype=release"

  MINGW_COPTS="--cross-file .ci/cross_toolchain.txt -Davahi=false -Deeze=false -Dsystemd=false \
  -Dpulseaudio=false -Dx11=false -Dopengl=none -Dlibmount=false \
  -Devas-loaders-disabler=json,pdf,ps,raw,svg,rsvg \
  -Dharfbuzz=true -Dpixman=true -Dembedded-lz4=false "

  if [ "$1" = "default" ]; then
    OPTS="$OPTS $MONO_LINUX_COPTS"
  elif [ "$1" = "options-enabled" ]; then
    OPTS="$OPTS $ENABLED_LINUX_COPTS $WAYLAND_LINUX_COPTS"
  elif [ "$1" = "options-disabled" ]; then
    OPTS="$OPTS $DISABLED_LINUX_COPTS"
  elif [ "$1" = "wayland" ]; then
    OPTS="$OPTS $WAYLAND_LINUX_COPTS"
  elif [ "$1" = "release-ready" ]; then
    OPTS="$OPTS $RELEASE_READY_LINUX_COPTS"
  elif [ "$1" = "coverity" ]; then
    OPTS="$OPTS $WAYLAND_LINUX_COPTS"
    travis_fold cov-download cov-download
    docker exec --env COVERITY_SCAN_TOKEN=$COVERITY_SCAN_TOKEN $(cat $HOME/cid) sh -c '.ci/coverity-tools-install.sh'
    travis_endfold cov-download
  elif [ "$1" = "mingw" ]; then
    OPTS="$OPTS $MINGW_COPTS"
    travis_fold cross-native cross-native
    docker exec $(cat $HOME/cid) sh -c '.ci/bootstrap-efl-native-for-cross.sh'
    travis_endfold cross-native
  fi

  if [ "$1" = "asan" ]; then
    travis_fold meson meson
    docker exec --env EIO_MONITOR_POLL=1 --env CC="ccache gcc" \
      --env CXX="ccache g++" --env CFLAGS="-O0 -g" --env CXXFLAGS="-O0 -g" \
      --env LD="ld.gold" $(cat $HOME/cid) sh -c "mkdir build && meson build $OPTS -Db_sanitize=address"
    travis_endfold meson
  elif [ "$1" = "mingw" ]; then
    travis_fold meson meson
    docker exec --env EIO_MONITOR_POLL=1 --env PKG_CONFIG_PATH="/ewpi-64-install/lib/pkgconfig/" \
       $(cat $HOME/cid) sh -c "mkdir build && meson build $OPTS"
    travis_endfold meson
  elif [ "$1" = "coverity" ]; then
    travis_fold meson meson
    docker exec --env EIO_MONITOR_POLL=1 --env CFLAGS="-fdirectives-only"  --env CC="gcc" --env CXX="g++"\
    --env CXXFLAGS="-fdirectives-only" $(cat $HOME/cid) sh -c "mkdir build && meson build $OPTS"
    travis_endfold meson
  else
    travis_fold meson meson
    docker exec --env EIO_MONITOR_POLL=1 --env CC="ccache gcc" \
      --env CXX="ccache g++" --env CFLAGS="-fdirectives-only" --env CXXFLAGS="-fdirectives-only" \
      --env LD="ld.gold" $(cat $HOME/cid) sh -c "mkdir build && meson build $OPTS"
    travis_endfold meson
  fi
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  # Prepare OSX env for build
  mkdir -p ~/Library/LaunchAgents
  ln -sfv /usr/local/opt/d-bus/*.plist ~/Library/LaunchAgents
  launchctl load ~/Library/LaunchAgents/org.freedesktop.dbus-session.plist
  export PATH="/usr/local/opt/ccache/libexec:$(brew --prefix gettext)/bin:$PATH"

  export CFLAGS="-I/usr/local/opt/openssl/include -frewrite-includes $CFLAGS"
  export LDFLAGS="-L/usr/local/opt/openssl/lib $LDFLAGS"
  LIBFFI_VER=$(brew list --versions libffi|head -n1|cut -d' ' -f2)
  export PKG_CONFIG_PATH="/usr/local/opt/openssl/lib/pkgconfig:/usr/local/Cellar/libffi/$LIBFFI_VER/lib/pkgconfig"
  export CC="ccache gcc"
  travis_fold meson meson
  mkdir build && meson build -Dopengl=full -Decore-imf-loaders-disabler=scim,ibus -Dx11=false -Davahi=false -Deeze=false -Dsystemd=false -Dnls=false -Dcocoa=true -Demotion-loaders-disabler=gstreamer1,libvlc,xine
  travis_endfold meson
fi
