" Change the :browse e filter to primarily show JavaScript-related files.
if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
    let b:browsefilter =
                \ "JavaScript Files (*.js)\t*.js\n"
                \ .. "JSX Files (*.jsx)\t*.jsx\n"
                \ .. "JavaScript Modules (*.es, *.es6, *.cjs, *.mjs, *.jsm)\t*.es;*.es6;*.cjs;*.mjs;*.jsm\n"
                \ .. "Vue Templates (*.vue)\t*.vue\n"
                \ .. "JSON Files (*.json)\t*.json\n"
    if has("win32")
        let b:browsefilter ..= "All Files (*.*)\t*\n"
    else
        let b:browsefilter ..= "All Files (*)\t*\n"
    endif
endif
