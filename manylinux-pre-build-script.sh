#!/usr/bin/env bash
# Build the native libraries that are not supplied by the manylinux image.
set -euo pipefail

install_build_dependencies() {
    dnf install -y \
        alsa-lib-devel \
        autoconf \
        automake \
        gcc \
        gcc-c++ \
        gettext-devel \
        guile-devel \
        libtool \
        make \
        portaudio-devel \
        portmidi-devel \
        zlib-devel
}

download() {
    local url="$1"
    local filename="$2"
    local checksum="$3"

    curl --fail --location --retry 3 --retry-delay 2 --silent --show-error \
        --output "$filename" "$url"
    echo "$checksum  $filename" | sha256sum --check --status
}

install_build_dependencies

workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT
cd "$workdir"

echo "====== Build and install liblo. ======"
download \
    "https://sourceforge.net/projects/liblo/files/liblo/0.31/liblo-0.31.tar.gz/download" \
    liblo-0.31.tar.gz \
    2b4f446e1220dcd624ecd8405248b08b7601e9a0d87a0b94730c2907dbccc750
tar -xzf liblo-0.31.tar.gz
pushd liblo-0.31
./configure
make -j"$(nproc)"
make install
popd

echo "====== Build and install libsndfile. ======"
download \
    "https://github.com/libsndfile/libsndfile/releases/download/1.0.31/libsndfile-1.0.31.tar.bz2" \
    libsndfile-1.0.31.tar.bz2 \
    a8cfb1c09ea6e90eff4ca87322d4168cdbe5035cb48717b40bf77e751cc02163
tar -xjf libsndfile-1.0.31.tar.bz2
pushd libsndfile-1.0.31
./autogen.sh
./configure
make -j"$(nproc)"
make install
popd

echo "====== Build and install alsa-lib. ======"
download \
    "https://www.alsa-project.org/files/pub/lib/alsa-lib-1.2.8.tar.bz2" \
    alsa-lib-1.2.8.tar.bz2 \
    1ab01b74e33425ca99c2e36c0844fd6888273193bd898240fe8f93accbcbf347
tar -xjf alsa-lib-1.2.8.tar.bz2
pushd alsa-lib-1.2.8
./configure --with-configdir=/usr/share/alsa
make -j"$(nproc)"
make install
popd

echo "====== Build and install JACK2. ======"
download \
    "https://github.com/jackaudio/jack2/archive/refs/tags/v1.9.21.tar.gz" \
    jack2-1.9.21.tar.gz \
    8b044a40ba5393b47605a920ba30744fdf8bf77d210eca90d39c8637fe6bc65d
tar -xzf jack2-1.9.21.tar.gz
pushd jack2-1.9.21
python3 waf configure LDFLAGS="-lstdc++"
python3 waf build
python3 waf install
popd

ldconfig
