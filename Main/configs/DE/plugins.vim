let s:config_path = fnamemodify(expand('<sfile>:p:h'), ':p')

execute 'set runtimepath^=' . s:config_path . '/UI'

execute 'set runtimepath^=' . s:config_path . '/UI/colors'

colorscheme spacegray
