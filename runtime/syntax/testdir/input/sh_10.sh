curl --http2 --tlsv1.2 -O https://example.com/file.txt
wget --inet6-only https://example.com/file.txt
gzip -9 file
xz -T0 file
find /tmp -type f -print0 | xargs -0 echo
cut -f1 file
nice -n10 make -j4
kill -9 12345
tail -n10 error.log
git log -n10
