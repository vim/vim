# sed(1)


# Addresses


# line number

42p
$p

/foobar/p
/foo[/]bar/p
/foo\/bar/p

\xfoobarxp
\xfoo\xbarxp
\xfoo[x]barxp

# skip bracket expressions
\a_\a_[a[:ascii:]a[.a.]a[=a=]a]_ap
\a_\a_[^a[:ascii:]a[.a.]a[=a=]a]_ap
\a_\a_[]a[:ascii:]a[.a.]a[=a=]a]_ap
\a_\a_[^]a[:ascii:]a[.a.]a[=a=]a]_ap


# range

42,84p
/foo/,/bar/p

/foo/,42p
42,/bar/p


# GNU extensions


# step

1~2p


# ignore case, multiline

/foobar/Ip
/foobar/Mp
/foobar/IMp
/foobar/MIp

\afoob\araIp
\afoob\araMp
\afoob\araIMp
\afoob\araMIp


# increment

42,+42p


# step

42,~2p

