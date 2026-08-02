#!/bin/bash
set -e

apt-get install -y weston

cat <<EOT >/etc/systemd/system/weston.service
[Unit]
Description=Weston Compositor Service
After=network.target
[Service]
ExecStart=/usr/bin/weston --backend=headless --width=2560 --height=1440 --socket=/tmp/weston_sock
[Install]
WantedBy=multi-user.target
EOT

systemctl enable weston.service
systemctl start weston.service
