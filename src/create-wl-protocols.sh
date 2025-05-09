wayland-scanner client-header protocols/ext-data-control-v1.xml ext-data-control-unstable-v1.h
wayland-scanner private-code protocols/ext-data-control-v1.xml ext-data-control-unstable-v1.c
wayland-scanner client-header protocols/wlr-data-control-unstable-v1.xml wlr-data-control-unstable-v1.h
wayland-scanner private-code protocols/wlr-data-control-unstable-v1.xml wlr-data-control-unstable-v1.c

sed -i "1s/^/#ifdef FEAT_WAYLAND_CLIPBOARD\n/" *-data-control-unstable-v1.*
sed -i "1s/^/#include \"vim.h\"\n/" *-data-control-unstable-v1.c
echo '#endif // FEAT_WAYLAND_CLIPBOARD' | tee -a *-data-control-unstable-v1.* > /dev/null
