" Vim syntax file
" Language:	SWIG
" Maintainer:	Roman Stanchak (rstanchak@yahoo.com)
" Last Change:	2023 November 23

if exists("b:current_syntax")
  finish
endif

" Read the C++ syntax to start with
runtime! syntax/cpp.vim
unlet b:current_syntax

" SWIG extentions
syn keyword swigMostCommonDirective %addmethods %apply %beginfile %clear %constant %define %echo %enddef %endoffile
syn keyword swigMostCommonDirective %extend %feature %fragment %ignore %import %importfile %include %includefile %inline
syn keyword swigMostCommonDirective %insert %keyword %module %name %namewarn %native %newobject %parms %pragma
syn keyword swigMostCommonDirective %rename %template %typedef %typemap %types %varargs %warn
syn keyword swigDirective %aggregate_check %alias %allocators %allowexception %array_class %array_functions %attribute %attribute2 %attribute2ref
syn keyword swigDirective %attribute_ref %attributeref %attributestring %attributeval %auto_ptr %bang %bar %begin %callback
syn keyword swigDirective %catches %cdata %cleardirector %clearimmutable %contract %copyctor %csattributes %csconst %csconstvalue
syn keyword swigDirective %csmethodmodifiers %csnothrowexception %cstring_bounded_mutable %cstring_bounded_output %cstring_chunk_output %cstring_input_binary %cstring_mutable %cstring_output_allocate %cstring_output_allocate_size
syn keyword swigDirective %cstring_output_maxsize %cstring_output_withsize %cwstring_bounded_mutable %cwstring_bounded_output %cwstring_chunk_output %cwstring_input_binary %cwstring_mutable %cwstring_output_allocate %cwstring_output_allocate_size
syn keyword swigDirective %cwstring_output_maxsize %cwstring_output_withsize %dconstvalue %delete_array %delobject %director %dmanifestconst %dmethodmodifiers %exception
syn keyword swigDirective %exceptionclass %extend_smart_pointer %factory %fastdispatch %free %freefunc %go_import %header %immutable
syn keyword swigDirective %implicit %implicitconv %init %interface %interface_custom %interface_impl %intrusive_ptr %intrusive_ptr_no_wrap %javaconst
syn keyword swigDirective %javaconstvalue %javaexception %javamethodmodifiers %luacode %malloc %markfunc %minit %mshutdown %multiple_values
syn keyword swigDirective %mutable %naturalvar %nocallback %nocopyctor %nodefaultctor %nodefaultdtor %nojavaexception %nonaturalvar %nonspace
syn keyword swigDirective %nspace %pointer_cast %pointer_class %pointer_functions %predicate %proxycode %pybinoperator %pybuffer_binary %pybuffer_mutable_binary
syn keyword swigDirective %pybuffer_mutable_string %pybuffer_string %pythonappend %pythonbegin %pythoncode %pythondynamic %pythonnondynamic %pythonprepend %raise
syn keyword swigDirective %refobject %remane %rinit %rshutdown %runtime %scilabconst %set_output %shared_ptr %std_comp_methods
syn keyword swigDirective %std_nodefconst_type %trackobjects %typecheck %typemaps_string %unique_ptr %unrefobject %values_as_list %values_as_vector %valuewrapper
syn keyword swigDirective %warnfilter %wrapper

syn match swigVerbatim "%\({\|}\)"
syn match swigUserDef "%[-_a-zA-Z0-9]\+"

" Default highlighting
hi def link swigMostCommonDirective Exception
hi def link swigDirective Exception
hi def link swigVerbatim Exception
hi def link swigUserDef PreProc

let b:current_syntax = "swig"

" vim: ts=8
