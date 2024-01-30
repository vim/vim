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
systemctl stop snapd
systemctl stop snapd.socket
systemctl disable snapd
systemctl disable snapd.socket
apt-get purge -y snapd gnome-software-plugin-snap
systemctl daemon-reload
rm -rf ~/snap
rm -rf /snap
rm -rf /var/snap
rm -rf /var/lib/snapd
rm -rf /var/cache/snapd
