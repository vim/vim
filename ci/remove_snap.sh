#!/usr/bin/env sh
pushd /etc/apt/preferences.d/
cat > nosnap.pref <<EOF
# To prevent repository packages from triggering the installation of snap,
# this file forbids snapd from being installed by APT.

Package: snapd
Pin: release a=*
Pin-Priority: -10
EOF
popd
snap remove --purge $(snap list | awk '!/^Name|^core/ {print $1}')
apt-get purge -y snapd
