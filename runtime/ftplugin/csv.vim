vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2024-06-18

if !exists("b:csv_delimiter")
    # detect delimiter
    var delimiters = ",;\t|"

    var max = 0
    for d in delimiters
        var count = getline(1)->split(d)->len() + getline(2)->split(d)->len()
        if count > max
            max = count
            b:csv_delimiter = d
        endif
    endfor
endif

if exists("b:did_ftplugin")
    finish
endif
b:did_ftplugin = 1
