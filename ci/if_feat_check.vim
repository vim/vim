if 1 " This prevents it from being run in tiny versions
  " Check for required features
  if exists("g:required")
    for feature in g:required
      if !has(feature)
        echo "Error: Feature '" .. feature .. "' not found"
        echo ''
        cquit
      endif
    endfor
    echo "\nChecked features: " .. string(g:required)
    echo ''
  endif
endif
" vim: sts=2 sw=2 et
