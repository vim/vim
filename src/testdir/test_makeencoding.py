#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Test program for :make, :grep and :cgetfile.

from __future__ import print_function, unicode_literals
import locale
import io
import sys

def set_output_encoding(enc=None):
    """Set the encoding of stdout and stderr

    arguments:
      enc -- Encoding name.
             If omitted, locale.getpreferredencoding() is used.
    """
    if enc is None:
        enc = locale.getpreferredencoding()

    def get_text_writer(fo, **kwargs):
        kw = dict(kwargs)
        kw.setdefault('errors', 'backslashreplace') # use \uXXXX style
        kw.setdefault('closefd', False)

        writer = io.open(fo.fileno(), mode='w', **kw)
        return writer

    sys.stdout = get_text_writer(sys.stdout, encoding=enc)
    sys.stderr = get_text_writer(sys.stderr, encoding=enc)


def main():
    enc = 'utf-8'
    if len(sys.argv) > 1:
        enc = sys.argv[1]
    set_output_encoding(enc)

    message_tbl = {
            'utf-8': 'ÀÈÌÒÙ こんにちは 你好',
            'latin1': 'ÀÈÌÒÙ',
            'cp932': 'こんにちは',
            'cp936': '你好',
            }

    print('Xfoobar.c(10) : %s (%s)' % (message_tbl[enc], enc))


if __name__ == "__main__":
    main()
