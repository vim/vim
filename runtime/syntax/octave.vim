" Vim syntax file
" Language:             Octave
" Maintainer:           Rik <rik@octave.org>
" Original Maintainers: Jaroslav Hajek <highegg@gmail.com>
"                       Francisco Castro <fcr@adinet.com.uy>
"                       Preben 'Peppe' Guldberg <peppe-vim@wielders.org>
" Original Author: Mario Eusebio
" Last Change: 13 Nov 2016
" Syntax matched to Octave Release: 4.2.0
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Use case sensitive matching of keywords
syn case match

" Stop keywords embedded in structures from lighting up
" For example, mystruct.length = 1 should not highlight length.
" WARNING: beginning of word pattern \< will no longer match '.'
setlocal iskeyword +=.

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Syntax group definitions for Octave
syn keyword octaveBeginKeyword  for parfor function if switch
syn keyword octaveBeginKeyword  try unwind_protect while do
syn keyword octaveBeginKeyword  classdef enumeration events methods properties
syn keyword octaveEndKeyword    end endfor endparfor endfunction endif endswitch
syn keyword octaveEndKeyword    end_try_catch end_unwind_protect endwhile until
syn keyword octaveEndKeyword    endclassdef endenumeration endevents endmethods
syn keyword octaveEndKeyword    endproperties
syn keyword octaveElseKeyword   case catch else elseif otherwise
syn keyword octaveElseKeyword   unwind_protect_cleanup

syn keyword octaveStatement  break continue global persistent return

syn keyword octaveVarKeyword varargin varargout

syn keyword octaveReserved  __FILE__ __LINE__

" List of commands (these don't require a parenthesis to invoke)
syn keyword octaveCommand contained  cd chdir clear close dbcont dbquit dbstep
syn keyword octaveCommand contained  demo diary doc echo edit edit_history
syn keyword octaveCommand contained  example format help history hold ishold
syn keyword octaveCommand contained  load lookfor ls mkoctfile more pkg run
syn keyword octaveCommand contained  run_history save shg test type what which
syn keyword octaveCommand contained  who whos

" List of functions which set internal variables
syn keyword octaveSetVarFun contained  EDITOR EXEC_PATH I IMAGE_PATH Inf J NA
syn keyword octaveSetVarFun contained  NaN PAGER PAGER_FLAGS PS1 PS2 PS4
syn keyword octaveSetVarFun contained  allow_noninteger_range_as_index ans
syn keyword octaveSetVarFun contained  beep_on_error built_in_docstrings_file
syn keyword octaveSetVarFun contained  completion_append_char
syn keyword octaveSetVarFun contained  confirm_recursive_rmdir
syn keyword octaveSetVarFun contained  crash_dumps_octave_core debug_java
syn keyword octaveSetVarFun contained  debug_jit debug_on_error
syn keyword octaveSetVarFun contained  debug_on_interrupt debug_on_warning
syn keyword octaveSetVarFun contained  disable_diagonal_matrix
syn keyword octaveSetVarFun contained  disable_permutation_matrix disable_range
syn keyword octaveSetVarFun contained  do_braindead_shortcircuit_evaluation
syn keyword octaveSetVarFun contained  doc_cache_file e echo_executing_commands
syn keyword octaveSetVarFun contained  eps false filemarker fixed_point_format
syn keyword octaveSetVarFun contained  gnuplot_binary graphics_toolkit history
syn keyword octaveSetVarFun contained  history_control history_file
syn keyword octaveSetVarFun contained  history_save history_size
syn keyword octaveSetVarFun contained  history_timestamp_format_string i
syn keyword octaveSetVarFun contained  ignore_function_time_stamp inf info_file
syn keyword octaveSetVarFun contained  info_program j
syn keyword octaveSetVarFun contained  java_matrix_autoconversion
syn keyword octaveSetVarFun contained  java_unsigned_autoconversion jit_enable
syn keyword octaveSetVarFun contained  jit_failcnt jit_startcnt ls_command
syn keyword octaveSetVarFun contained  makeinfo_program max_recursion_depth
syn keyword octaveSetVarFun contained  missing_component_hook
syn keyword octaveSetVarFun contained  missing_function_hook mouse_wheel_zoom
syn keyword octaveSetVarFun contained  nan nargin nargout
syn keyword octaveSetVarFun contained  octave_core_file_limit
syn keyword octaveSetVarFun contained  octave_core_file_name
syn keyword octaveSetVarFun contained  octave_core_file_options
syn keyword octaveSetVarFun contained  optimize_subsasgn_calls
syn keyword octaveSetVarFun contained  output_max_field_width output_precision
syn keyword octaveSetVarFun contained  page_output_immediately
syn keyword octaveSetVarFun contained  page_screen_output path pathsep pi
syn keyword octaveSetVarFun contained  prefdir print_empty_dimensions
syn keyword octaveSetVarFun contained  print_struct_array_contents realmax
syn keyword octaveSetVarFun contained  realmin save_default_options
syn keyword octaveSetVarFun contained  save_header_format_string save_precision
syn keyword octaveSetVarFun contained  sighup_dumps_octave_core
syn keyword octaveSetVarFun contained  sigterm_dumps_octave_core
syn keyword octaveSetVarFun contained  silent_functions sparse_auto_mutate
syn keyword octaveSetVarFun contained  split_long_rows string_fill_char
syn keyword octaveSetVarFun contained  struct_levels_to_print
syn keyword octaveSetVarFun contained  suppress_verbose_help_message svd_driver
syn keyword octaveSetVarFun contained  texi_macros_file true whos_line_format

" List of functions which query internal variables
" Excluded i,j from list above because they are often used as loop variables
" They will be highlighted appropriately by the rule which matches numbers
syn keyword octaveVariable contained  EDITOR EDITOR EXEC_PATH F_SETFL I
syn keyword octaveVariable contained  IMAGE_PATH Inf J NA NaN PAGER
syn keyword octaveVariable contained  PAGER_FLAGS PS1 PS2 PS4
syn keyword octaveVariable contained  allow_noninteger_range_as_index ans
syn keyword octaveVariable contained  beep_on_error built_in_docstrings_file
syn keyword octaveVariable contained  completion_append_char
syn keyword octaveVariable contained  confirm_recursive_rmdir
syn keyword octaveVariable contained  crash_dumps_octave_core debug_java
syn keyword octaveVariable contained  debug_jit debug_on_error
syn keyword octaveVariable contained  debug_on_interrupt debug_on_warning
syn keyword octaveVariable contained  disable_diagonal_matrix
syn keyword octaveVariable contained  disable_permutation_matrix disable_range
syn keyword octaveVariable contained  do_braindead_shortcircuit_evaluation
syn keyword octaveVariable contained  doc_cache_file e echo_executing_commands
syn keyword octaveVariable contained  eps false filemarker fixed_point_format
syn keyword octaveVariable contained  gnuplot_binary graphics_toolkit history
syn keyword octaveVariable contained  history_control history_file
syn keyword octaveVariable contained  history_save history_size
syn keyword octaveVariable contained  history_timestamp_format_string i
syn keyword octaveVariable contained  ignore_function_time_stamp inf info_file
syn keyword octaveVariable contained  info_program j
syn keyword octaveVariable contained  java_matrix_autoconversion
syn keyword octaveVariable contained  java_unsigned_autoconversion jit_enable
syn keyword octaveVariable contained  jit_failcnt jit_startcnt ls_command
syn keyword octaveVariable contained  makeinfo_program max_recursion_depth
syn keyword octaveVariable contained  missing_component_hook
syn keyword octaveVariable contained  missing_function_hook mouse_wheel_zoom
syn keyword octaveVariable contained  nan nargin nargout
syn keyword octaveVariable contained  octave_core_file_limit
syn keyword octaveVariable contained  octave_core_file_name
syn keyword octaveVariable contained  octave_core_file_options
syn keyword octaveVariable contained  optimize_subsasgn_calls
syn keyword octaveVariable contained  output_max_field_width output_precision
syn keyword octaveVariable contained  page_output_immediately
syn keyword octaveVariable contained  page_screen_output path pathsep pi
syn keyword octaveVariable contained  prefdir print_empty_dimensions
syn keyword octaveVariable contained  print_struct_array_contents realmax
syn keyword octaveVariable contained  realmin save_default_options
syn keyword octaveVariable contained  save_header_format_string save_precision
syn keyword octaveVariable contained  sighup_dumps_octave_core
syn keyword octaveVariable contained  sigterm_dumps_octave_core
syn keyword octaveVariable contained  silent_functions sparse_auto_mutate
syn keyword octaveVariable contained  split_long_rows string_fill_char
syn keyword octaveVariable contained  struct_levels_to_print
syn keyword octaveVariable contained  suppress_verbose_help_message svd_driver
syn keyword octaveVariable contained  texi_macros_file true whos_line_format

" Read-only variables
syn keyword octaveVariable contained  F_DUPFD F_GETFD F_GETFL F_SETFD F_SETFL
syn keyword octaveVariable contained  OCTAVE_HOME OCTAVE_VERSION O_APPEND
syn keyword octaveVariable contained  O_ASYNC O_CREAT O_EXCL O_NONBLOCK
syn keyword octaveVariable contained  O_RDONLY O_RDWR O_SYNC O_TRUNC O_WRONLY
syn keyword octaveVariable contained  P_tmpdir SEEK_CUR SEEK_END SEEK_SET SIG
syn keyword octaveVariable contained  WCONTINUE WCOREDUMP WEXITSTATUS
syn keyword octaveVariable contained  WIFCONTINUED WIFEXITED WIFSIGNALED
syn keyword octaveVariable contained  WIFSTOPPED WNOHANG WSTOPSIG WTERMSIG
syn keyword octaveVariable contained  WUNTRACED argv
syn keyword octaveVariable contained  available_graphics_toolkits
syn keyword octaveVariable contained  command_line_path have_window_system
syn keyword octaveVariable contained  isstudent loaded_graphics_toolkits
syn keyword octaveVariable contained  matlabroot namelengthmax
syn keyword octaveVariable contained  native_float_format
syn keyword octaveVariable contained  program_invocation_name program_name pwd
syn keyword octaveVariable contained  stderr stdin stdout

" List of functions
syn keyword octaveFunction contained  S_ISBLK S_ISCHR S_ISDIR S_ISFIFO S_ISLNK
syn keyword octaveFunction contained  S_ISREG S_ISSOCK __accumarray_max__
syn keyword octaveFunction contained  __accumarray_min__ __accumarray_sum__
syn keyword octaveFunction contained  __accumdim_sum__
syn keyword octaveFunction contained  __actual_axis_position__ __all_opts__
syn keyword octaveFunction contained  __builtins__ __calc_dimensions__
syn keyword octaveFunction contained  __clabel__ __compactformat__
syn keyword octaveFunction contained  __contourc__ __current_scope__
syn keyword octaveFunction contained  __db_next_breakpoint_quiet__
syn keyword octaveFunction contained  __default_plot_options__ __delaunayn__
syn keyword octaveFunction contained  __diaryfile__ __diarystate__
syn keyword octaveFunction contained  __dispatch__ __display_tokens__
syn keyword octaveFunction contained  __dsearchn__ __dump_load_path__
syn keyword octaveFunction contained  __dump_symtab_info__ __dump_typeinfo__
syn keyword octaveFunction contained  __echostate__ __eigs__ __fieldnames__
syn keyword octaveFunction contained  __finish__ __fltk_check__
syn keyword octaveFunction contained  __fltk_uigetfile__ __fnmatch__
syn keyword octaveFunction contained  __formatstring__ __ftp__ __ftp_ascii__
syn keyword octaveFunction contained  __ftp_binary__ __ftp_close__ __ftp_cwd__
syn keyword octaveFunction contained  __ftp_delete__ __ftp_dir__ __ftp_mget__
syn keyword octaveFunction contained  __ftp_mkdir__ __ftp_mode__ __ftp_mput__
syn keyword octaveFunction contained  __ftp_pwd__ __ftp_rename__ __ftp_rmdir__
syn keyword octaveFunction contained  __get__ __get_cmdline_fcn_txt__
syn keyword octaveFunction contained  __getlegenddata__ __glpk__
syn keyword octaveFunction contained  __gnuplot_drawnow__ __go_axes__
syn keyword octaveFunction contained  __go_axes_init__ __go_delete__
syn keyword octaveFunction contained  __go_execute_callback__ __go_figure__
syn keyword octaveFunction contained  __go_figure_handles__ __go_handles__
syn keyword octaveFunction contained  __go_hggroup__ __go_image__ __go_light__
syn keyword octaveFunction contained  __go_line__ __go_patch__ __go_surface__
syn keyword octaveFunction contained  __go_text__ __go_uibuttongroup__
syn keyword octaveFunction contained  __go_uicontextmenu__ __go_uicontrol__
syn keyword octaveFunction contained  __go_uimenu__ __go_uipanel__
syn keyword octaveFunction contained  __go_uipushtool__ __go_uitoggletool__
syn keyword octaveFunction contained  __go_uitoolbar__
syn keyword octaveFunction contained  __gripe_missing_component__ __gud_mode__
syn keyword octaveFunction contained  __have_feature__ __have_gnuplot__
syn keyword octaveFunction contained  __ichol0__ __icholt__ __ilu0__ __iluc__
syn keyword octaveFunction contained  __ilutp__ __image_pixel_size__
syn keyword octaveFunction contained  __init_fltk__ __init_gnuplot__
syn keyword octaveFunction contained  __is_handle_visible__ __java_exit__
syn keyword octaveFunction contained  __java_get__ __java_init__ __java_set__
syn keyword octaveFunction contained  __keywords__ __lexer_debug_flag__
syn keyword octaveFunction contained  __lin_interpn__ __list_functions__
syn keyword octaveFunction contained  __luinc__ __magick_finfo__
syn keyword octaveFunction contained  __magick_formats__ __magick_ping__
syn keyword octaveFunction contained  __magick_read__ __magick_write__
syn keyword octaveFunction contained  __makeinfo__ __meta_class_query__
syn keyword octaveFunction contained  __meta_get_package__ __methods__
syn keyword octaveFunction contained  __mkdir__ __next_line_color__
syn keyword octaveFunction contained  __next_line_style__
syn keyword octaveFunction contained  __octave_config_info__
syn keyword octaveFunction contained  __octave_link_edit_file__
syn keyword octaveFunction contained  __octave_link_enabled__
syn keyword octaveFunction contained  __octave_link_file_dialog__
syn keyword octaveFunction contained  __octave_link_input_dialog__
syn keyword octaveFunction contained  __octave_link_list_dialog__
syn keyword octaveFunction contained  __octave_link_message_dialog__
syn keyword octaveFunction contained  __octave_link_question_dialog__
syn keyword octaveFunction contained  __octave_link_show_doc__
syn keyword octaveFunction contained  __octave_link_show_preferences__
syn keyword octaveFunction contained  __open_with_system_app__ __opengl_info__
syn keyword octaveFunction contained  __operators__ __osmesa_print__
syn keyword octaveFunction contained  __parent_classes__ __parse_file__
syn keyword octaveFunction contained  __parser_debug_flag__ __pathorig__
syn keyword octaveFunction contained  __pchip_deriv__ __player_audioplayer__
syn keyword octaveFunction contained  __player_get_channels__
syn keyword octaveFunction contained  __player_get_fs__ __player_get_id__
syn keyword octaveFunction contained  __player_get_nbits__
syn keyword octaveFunction contained  __player_get_sample_number__
syn keyword octaveFunction contained  __player_get_tag__
syn keyword octaveFunction contained  __player_get_total_samples__
syn keyword octaveFunction contained  __player_get_userdata__
syn keyword octaveFunction contained  __player_isplaying__ __player_pause__
syn keyword octaveFunction contained  __player_play__ __player_playblocking__
syn keyword octaveFunction contained  __player_resume__ __player_set_fs__
syn keyword octaveFunction contained  __player_set_tag__
syn keyword octaveFunction contained  __player_set_userdata__ __player_stop__
syn keyword octaveFunction contained  __plt_get_axis_arg__ __pltopt__
syn keyword octaveFunction contained  __printf_assert__ __profiler_data__
syn keyword octaveFunction contained  __profiler_enable__ __profiler_reset__
syn keyword octaveFunction contained  __prog_output_assert__ __qp__
syn keyword octaveFunction contained  __recorder_audiorecorder__
syn keyword octaveFunction contained  __recorder_get_channels__
syn keyword octaveFunction contained  __recorder_get_fs__ __recorder_get_id__
syn keyword octaveFunction contained  __recorder_get_nbits__
syn keyword octaveFunction contained  __recorder_get_sample_number__
syn keyword octaveFunction contained  __recorder_get_tag__
syn keyword octaveFunction contained  __recorder_get_total_samples__
syn keyword octaveFunction contained  __recorder_get_userdata__
syn keyword octaveFunction contained  __recorder_getaudiodata__
syn keyword octaveFunction contained  __recorder_isrecording__
syn keyword octaveFunction contained  __recorder_pause__ __recorder_record__
syn keyword octaveFunction contained  __recorder_recordblocking__
syn keyword octaveFunction contained  __recorder_resume__ __recorder_set_fs__
syn keyword octaveFunction contained  __recorder_set_tag__
syn keyword octaveFunction contained  __recorder_set_userdata__
syn keyword octaveFunction contained  __recorder_stop__ __request_drawnow__
syn keyword octaveFunction contained  __run_test_suite__ __sort_rows_idx__
syn keyword octaveFunction contained  __superclass_reference__ __textscan__
syn keyword octaveFunction contained  __token_count__ __unimplemented__
syn keyword octaveFunction contained  __usage__ __varval__ __version_info__
syn keyword octaveFunction contained  __voronoi__ __wglob__ __which__ __zoom__
syn keyword octaveFunction contained  abs accumarray accumdim acos acosd acosh
syn keyword octaveFunction contained  acot acotd acoth acsc acscd acsch
syn keyword octaveFunction contained  add_input_event_hook addlistener addpath
syn keyword octaveFunction contained  addpref addproperty addtodate airy all
syn keyword octaveFunction contained  allchild amd ancestor and angle
syn keyword octaveFunction contained  annotation anova any arch_fit arch_rnd
syn keyword octaveFunction contained  arch_test area arg argnames arma_rnd
syn keyword octaveFunction contained  arrayfun asctime asec asecd asech asin
syn keyword octaveFunction contained  asind asinh assert assignin atan atan2
syn keyword octaveFunction contained  atan2d atand atanh atexit audiodevinfo
syn keyword octaveFunction contained  audioformats audioinfo audioread
syn keyword octaveFunction contained  audiowrite autoload autoreg_matrix
syn keyword octaveFunction contained  autumn axes axis balance bandwidth bar
syn keyword octaveFunction contained  barh bartlett bartlett_test base2dec
syn keyword octaveFunction contained  base64_decode base64_encode beep bessel
syn keyword octaveFunction contained  besselh besseli besselj besselk bessely
syn keyword octaveFunction contained  beta betacdf betainc betaincinv betainv
syn keyword octaveFunction contained  betaln betapdf betarnd bicg bicgstab
syn keyword octaveFunction contained  bicubic bin2dec bincoeff binocdf binoinv
syn keyword octaveFunction contained  binopdf binornd bitand bitcmp bitget
syn keyword octaveFunction contained  bitmax bitor bitpack bitset bitshift
syn keyword octaveFunction contained  bitunpack bitxor blackman blanks blkdiag
syn keyword octaveFunction contained  blkmm bone box brighten bsxfun
syn keyword octaveFunction contained  bug_report builtin bunzip2 bzip2
syn keyword octaveFunction contained  calendar camlight canonicalize_file_name
syn keyword octaveFunction contained  cart2pol cart2sph cast cat cauchy_cdf
syn keyword octaveFunction contained  cauchy_inv cauchy_pdf cauchy_rnd caxis
syn keyword octaveFunction contained  cbrt ccolamd ceil cell cell2mat
syn keyword octaveFunction contained  cell2struct celldisp cellfun
syn keyword octaveFunction contained  cellindexmat cellslices cellstr center
syn keyword octaveFunction contained  cgs char chi2cdf chi2inv chi2pdf chi2rnd
syn keyword octaveFunction contained  chisquare_test_homogeneity
syn keyword octaveFunction contained  chisquare_test_independence chol
syn keyword octaveFunction contained  chol2inv choldelete cholinsert cholinv
syn keyword octaveFunction contained  cholshift cholupdate chop circshift
syn keyword octaveFunction contained  citation cla clabel class clc clf clock
syn keyword octaveFunction contained  cloglog closereq cmpermute cmunique
syn keyword octaveFunction contained  colamd colloc colon colorbar colorcube
syn keyword octaveFunction contained  colormap colperm colstyle columns comet
syn keyword octaveFunction contained  comet3 comma common_size
syn keyword octaveFunction contained  commutation_matrix compan
syn keyword octaveFunction contained  compare_versions compass
syn keyword octaveFunction contained  completion_matches complex computer cond
syn keyword octaveFunction contained  condeig condest conj contour contour3
syn keyword octaveFunction contained  contourc contourf contrast conv conv2
syn keyword octaveFunction contained  convhull convhulln convn cool copper
syn keyword octaveFunction contained  copyfile copyobj cor_test corr cos cosd
syn keyword octaveFunction contained  cosh cot cotd coth cov cplxpair cputime
syn keyword octaveFunction contained  cross csc cscd csch cstrcat csvread
syn keyword octaveFunction contained  csvwrite csymamd ctime ctranspose
syn keyword octaveFunction contained  cubehelix cummax cummin cumprod cumsum
syn keyword octaveFunction contained  cumtrapz curl cylinder daspect daspk
syn keyword octaveFunction contained  daspk_options dasrt dasrt_options dassl
syn keyword octaveFunction contained  dassl_options date datenum datestr
syn keyword octaveFunction contained  datetick datevec dawson dbclear dbdown
syn keyword octaveFunction contained  dblist dblquad dbnext dbstack dbstatus
syn keyword octaveFunction contained  dbstop dbtype dbup dbwhere deal deblank
syn keyword octaveFunction contained  debug dec2base dec2bin dec2hex deconv
syn keyword octaveFunction contained  deg2rad del2 delaunay delaunay3
syn keyword octaveFunction contained  delaunayn delete dellistener desktop det
syn keyword octaveFunction contained  detrend diag dialog diff diffpara
syn keyword octaveFunction contained  diffuse dir dir_in_loadpath discrete_cdf
syn keyword octaveFunction contained  discrete_inv discrete_pdf discrete_rnd
syn keyword octaveFunction contained  disp display divergence dlmread dlmwrite
syn keyword octaveFunction contained  dmperm do_string_escapes
syn keyword octaveFunction contained  doc_cache_create dos dot double drawnow
syn keyword octaveFunction contained  dsearch dsearchn dump_prefs dup2
syn keyword octaveFunction contained  duplication_matrix durbinlevinson eig
syn keyword octaveFunction contained  eigs ellipj ellipke ellipsoid
syn keyword octaveFunction contained  empirical_cdf empirical_inv
syn keyword octaveFunction contained  empirical_pdf empirical_rnd endgrent
syn keyword octaveFunction contained  endpwent eomday eq erf erfc erfcinv
syn keyword octaveFunction contained  erfcx erfi erfinv errno errno_list error
syn keyword octaveFunction contained  error_ids errorbar errordlg etime etree
syn keyword octaveFunction contained  etreeplot eval evalc evalin exec exist
syn keyword octaveFunction contained  exit exp expcdf expint expinv expm expm1
syn keyword octaveFunction contained  exppdf exprnd eye ezcontour ezcontourf
syn keyword octaveFunction contained  ezmesh ezmeshc ezplot ezplot3 ezpolar
syn keyword octaveFunction contained  ezsurf ezsurfc f_test_regression fact
syn keyword octaveFunction contained  factor factorial fail fcdf fclear fclose
syn keyword octaveFunction contained  fcntl fdisp feather feof ferror feval
syn keyword octaveFunction contained  fflush fft fft2 fftconv fftfilt fftn
syn keyword octaveFunction contained  fftshift fftw fgetl fgets fieldnames
syn keyword octaveFunction contained  figure file_in_loadpath file_in_path
syn keyword octaveFunction contained  fileattrib fileparts fileread filesep
syn keyword octaveFunction contained  fill filter filter2 find
syn keyword octaveFunction contained  find_dir_in_path findall findfigs
syn keyword octaveFunction contained  findobj findstr finite finv fix flag
syn keyword octaveFunction contained  flintmax flip flipdim fliplr flipud
syn keyword octaveFunction contained  floor fminbnd fminsearch fminunc fmod
syn keyword octaveFunction contained  fnmatch fopen fork formula fpdf fplot
syn keyword octaveFunction contained  fprintf fputs fractdiff frame2im fread
syn keyword octaveFunction contained  freport freqz freqz_plot frewind frnd
syn keyword octaveFunction contained  fscanf fseek fskipl fsolve ftell full
syn keyword octaveFunction contained  fullfile func2str functions fwrite fzero
syn keyword octaveFunction contained  gallery gamcdf gaminv gamma gammainc
syn keyword octaveFunction contained  gammaln gampdf gamrnd gca gcbf gcbo gcd
syn keyword octaveFunction contained  gcf gco ge genpath genvarname geocdf
syn keyword octaveFunction contained  geoinv geopdf geornd get
syn keyword octaveFunction contained  get_first_help_sentence get_help_text
syn keyword octaveFunction contained  get_help_text_from_file
syn keyword octaveFunction contained  get_home_directory getappdata getegid
syn keyword octaveFunction contained  getenv geteuid getfield getgid getgrent
syn keyword octaveFunction contained  getgrgid getgrnam gethostname getpgrp
syn keyword octaveFunction contained  getpid getppid getpref getpwent getpwnam
syn keyword octaveFunction contained  getpwuid getrusage getuid ginput givens
syn keyword octaveFunction contained  glob glpk gls gmap40 gmres gmtime gplot
syn keyword octaveFunction contained  grabcode gradient gray gray2ind grid
syn keyword octaveFunction contained  griddata griddata3 griddatan gt gtext
syn keyword octaveFunction contained  guidata guihandles gunzip gzip hadamard
syn keyword octaveFunction contained  hamming hankel hanning hash hdl2struct
syn keyword octaveFunction contained  helpdlg hess hex2dec hex2num hggroup
syn keyword octaveFunction contained  hgload hgsave hidden hilb hist histc
syn keyword octaveFunction contained  home horzcat hot hotelling_test
syn keyword octaveFunction contained  hotelling_test_2 housh hsv hsv2rgb hurst
syn keyword octaveFunction contained  hygecdf hygeinv hygepdf hygernd hypot
syn keyword octaveFunction contained  ichol idivide ifelse ifft ifft2 ifftn
syn keyword octaveFunction contained  ifftshift ilu im2double im2frame imag
syn keyword octaveFunction contained  image imagesc imfinfo imformats
syn keyword octaveFunction contained  importdata imread imshow imwrite
syn keyword octaveFunction contained  ind2gray ind2rgb ind2sub index
syn keyword octaveFunction contained  inferiorto info inline inpolygon input
syn keyword octaveFunction contained  inputParser inputdlg inputname int16
syn keyword octaveFunction contained  int2str int32 int64 int8 interp1 interp2
syn keyword octaveFunction contained  interp3 interpft interpn intersect
syn keyword octaveFunction contained  intmax intmin inv inverse invhilb
syn keyword octaveFunction contained  ipermute iqr is_absolute_filename
syn keyword octaveFunction contained  is_dq_string is_function_handle
syn keyword octaveFunction contained  is_leap_year is_rooted_relative_filename
syn keyword octaveFunction contained  is_sq_string is_valid_file_id isa
syn keyword octaveFunction contained  isalnum isalpha isappdata isargout
syn keyword octaveFunction contained  isascii isaxes isbanded isbool iscell
syn keyword octaveFunction contained  iscellstr ischar iscntrl iscolormap
syn keyword octaveFunction contained  iscolumn iscomplex isdebugmode
syn keyword octaveFunction contained  isdefinite isdeployed isdiag isdigit
syn keyword octaveFunction contained  isdir isempty isequal isequaln isfield
syn keyword octaveFunction contained  isfigure isfinite isfloat isglobal
syn keyword octaveFunction contained  isgraph isguirunning ishandle
syn keyword octaveFunction contained  ishermitian ishghandle isieee isindex
syn keyword octaveFunction contained  isinf isinteger isjava iskeyword
syn keyword octaveFunction contained  isletter islogical islower ismac
syn keyword octaveFunction contained  ismatrix ismember ismethod isna isnan
syn keyword octaveFunction contained  isnull isnumeric isobject isocaps
syn keyword octaveFunction contained  isocolors isonormals isosurface ispc
syn keyword octaveFunction contained  ispref isprime isprint isprop ispunct
syn keyword octaveFunction contained  isreal isrow isscalar issorted isspace
syn keyword octaveFunction contained  issparse issquare isstr isstrprop
syn keyword octaveFunction contained  isstruct issymmetric istril istriu
syn keyword octaveFunction contained  isunix isupper isvarname isvector
syn keyword octaveFunction contained  isxdigit java2mat javaArray javaMethod
syn keyword octaveFunction contained  javaObject java_get java_set javaaddpath
syn keyword octaveFunction contained  javachk javaclasspath javamem javarmpath
syn keyword octaveFunction contained  jet kbhit kendall keyboard kill
syn keyword octaveFunction contained  kolmogorov_smirnov_cdf
syn keyword octaveFunction contained  kolmogorov_smirnov_test
syn keyword octaveFunction contained  kolmogorov_smirnov_test_2 kron
syn keyword octaveFunction contained  kruskal_wallis_test krylov kurtosis
syn keyword octaveFunction contained  laplace_cdf laplace_inv laplace_pdf
syn keyword octaveFunction contained  laplace_rnd lasterr lasterror lastwarn
syn keyword octaveFunction contained  lcm ldivide le legend legendre length
syn keyword octaveFunction contained  lgamma license light lighting lin2mu
syn keyword octaveFunction contained  line lines link linkaxes linkprop
syn keyword octaveFunction contained  linsolve linspace list_in_columns
syn keyword octaveFunction contained  list_primes listdlg loadaudio loadobj
syn keyword octaveFunction contained  localfunctions localtime log log10 log1p
syn keyword octaveFunction contained  log2 logical logistic_cdf logistic_inv
syn keyword octaveFunction contained  logistic_pdf logistic_regression
syn keyword octaveFunction contained  logistic_rnd logit loglog loglogerr logm
syn keyword octaveFunction contained  logncdf logninv lognpdf lognrnd logspace
syn keyword octaveFunction contained  lookup lower lscov lsode lsode_options
syn keyword octaveFunction contained  lsqnonneg lstat lt lu luinc luupdate
syn keyword octaveFunction contained  magic mahalanobis make_absolute_filename
syn keyword octaveFunction contained  manova mat2cell mat2str material
syn keyword octaveFunction contained  matrix_type max mcnemar_test md5sum mean
syn keyword octaveFunction contained  meansq median menu merge mesh meshc
syn keyword octaveFunction contained  meshgrid meshz metaclass methods mex
syn keyword octaveFunction contained  mexext mfilename mgorth min minus
syn keyword octaveFunction contained  mislocked mkdir mkfifo mkpp mkstemp
syn keyword octaveFunction contained  mktime mldivide mlock mod mode moment
syn keyword octaveFunction contained  movefile mpoles mpower mrdivide msgbox
syn keyword octaveFunction contained  mtimes mu2lin munlock nargchk narginchk
syn keyword octaveFunction contained  nargoutchk nbincdf nbininv nbinpdf
syn keyword octaveFunction contained  nbinrnd nchoosek ndgrid ndims ne newplot
syn keyword octaveFunction contained  news nextpow2 nfields nnz nonzeros norm
syn keyword octaveFunction contained  normcdf normest normest1 norminv normpdf
syn keyword octaveFunction contained  normrnd not now nproc nth_element
syn keyword octaveFunction contained  nthargout nthroot ntsc2rgb null num2cell
syn keyword octaveFunction contained  num2hex num2str numel numfields nzmax
syn keyword octaveFunction contained  ocean octave_config_info
syn keyword octaveFunction contained  octave_tmp_file_name ode23 ode45 odeget
syn keyword octaveFunction contained  odeplot odeset ols onCleanup onenormest
syn keyword octaveFunction contained  ones open optimget optimset or
syn keyword octaveFunction contained  orderfields ordschur orient orth
syn keyword octaveFunction contained  ostrsplit pack padecoef pan paren pareto
syn keyword octaveFunction contained  parseparams pascal patch pathdef pause
syn keyword octaveFunction contained  pbaspect pcg pchip pclose pcolor pcr
syn keyword octaveFunction contained  peaks periodogram perl perms permute pie
syn keyword octaveFunction contained  pie3 pink pinv pipe planerot playaudio
syn keyword octaveFunction contained  plot plot3 plotmatrix plotyy plus
syn keyword octaveFunction contained  poisscdf poissinv poisspdf poissrnd
syn keyword octaveFunction contained  pol2cart polar poly polyaffine polyarea
syn keyword octaveFunction contained  polyder polyeig polyfit polygcd polyint
syn keyword octaveFunction contained  polyout polyreduce polyval polyvalm
syn keyword octaveFunction contained  popen popen2 postpad pow2 power powerset
syn keyword octaveFunction contained  ppder ppint ppjumps ppplot ppval
syn keyword octaveFunction contained  pqpnonneg prctile preferences prepad
syn keyword octaveFunction contained  primes print print_usage printd printf
syn keyword octaveFunction contained  prism probit prod profexplore profexport
syn keyword octaveFunction contained  profile profshow prop_test_2 psi publish
syn keyword octaveFunction contained  putenv puts python qmr qp qqplot qr
syn keyword octaveFunction contained  qrdelete qrinsert qrshift qrupdate quad
syn keyword octaveFunction contained  quad_options quadcc quadgk quadl quadv
syn keyword octaveFunction contained  quantile questdlg quit quiver quiver3 qz
syn keyword octaveFunction contained  qzhess rad2deg rainbow rand rande randg
syn keyword octaveFunction contained  randi randn randp randperm range rank
syn keyword octaveFunction contained  ranks rat rats rcond rdivide readdir
syn keyword octaveFunction contained  readline_re_read_init_file
syn keyword octaveFunction contained  readline_read_init_file readlink real
syn keyword octaveFunction contained  reallog realpow realsqrt record
syn keyword octaveFunction contained  rectangle rectint recycle reducepatch
syn keyword octaveFunction contained  reducevolume refresh refreshdata regexp
syn keyword octaveFunction contained  regexpi regexprep regexptranslate
syn keyword octaveFunction contained  register_graphics_toolkit rehash rem
syn keyword octaveFunction contained  remove_input_event_hook rename repelems
syn keyword octaveFunction contained  repmat reset reshape residue resize
syn keyword octaveFunction contained  restoredefaultpath rethrow rgb2hsv
syn keyword octaveFunction contained  rgb2ind rgb2ntsc rgbplot ribbon rindex
syn keyword octaveFunction contained  rmappdata rmdir rmfield rmpath rmpref
syn keyword octaveFunction contained  roots rose rosser rot90 rotate rotate3d
syn keyword octaveFunction contained  rotdim round roundb rows rref rsf2csf
syn keyword octaveFunction contained  run_count run_test rundemos runlength
syn keyword octaveFunction contained  runtests saveas saveaudio saveobj
syn keyword octaveFunction contained  savepath scanf scatter scatter3 schur
syn keyword octaveFunction contained  sec secd sech semicolon semilogx
syn keyword octaveFunction contained  semilogxerr semilogy semilogyerr set
syn keyword octaveFunction contained  setappdata setaudio setdiff setenv
syn keyword octaveFunction contained  setfield setgrent setpref setpwent
syn keyword octaveFunction contained  setxor shading shift shiftdim
syn keyword octaveFunction contained  shrinkfaces sign sign_test signbit sin
syn keyword octaveFunction contained  sinc sind sinetone sinewave single sinh
syn keyword octaveFunction contained  size size_equal sizemax sizeof skewness
syn keyword octaveFunction contained  sleep slice smooth3 sombrero sort
syn keyword octaveFunction contained  sortrows sound soundsc source spalloc
syn keyword octaveFunction contained  sparse spaugment spconvert spdiags
syn keyword octaveFunction contained  spearman spectral_adf spectral_xdf
syn keyword octaveFunction contained  specular speed spencer speye spfun
syn keyword octaveFunction contained  sph2cart sphere spinmap spline splinefit
syn keyword octaveFunction contained  spones spparms sprand sprandn sprandsym
syn keyword octaveFunction contained  sprank spring sprintf spstats spy sqp
syn keyword octaveFunction contained  sqrt sqrtm squeeze sscanf stairs stat
syn keyword octaveFunction contained  statistics std stdnormal_cdf
syn keyword octaveFunction contained  stdnormal_inv stdnormal_pdf
syn keyword octaveFunction contained  stdnormal_rnd stem stem3 stemleaf stft
syn keyword octaveFunction contained  str2double str2func str2num strcat
syn keyword octaveFunction contained  strchr strcmp strcmpi strfind strftime
syn keyword octaveFunction contained  strjoin strjust strmatch strncmp
syn keyword octaveFunction contained  strncmpi strptime strread strrep
syn keyword octaveFunction contained  strsplit strtok strtrim strtrunc struct
syn keyword octaveFunction contained  struct2cell struct2hdl structfun strvcat
syn keyword octaveFunction contained  sub2ind subplot subsasgn subsindex
syn keyword octaveFunction contained  subspace subsref substr substruct sum
syn keyword octaveFunction contained  summer sumsq superiorto surf surface
syn keyword octaveFunction contained  surfc surfl surfnorm svd svds swapbytes
syn keyword octaveFunction contained  syl sylvester symamd symbfact symlink
syn keyword octaveFunction contained  symrcm symvar synthesis system t_test
syn keyword octaveFunction contained  t_test_2 t_test_regression table tan
syn keyword octaveFunction contained  tand tanh tar tcdf tempdir tempname
syn keyword octaveFunction contained  terminal_size tetramesh text textread
syn keyword octaveFunction contained  textscan tic tilde_expand time times
syn keyword octaveFunction contained  tinv title tmpfile tmpnam toascii toc
syn keyword octaveFunction contained  toeplitz tolower toupper tpdf trace
syn keyword octaveFunction contained  transpose trapz treelayout treeplot tril
syn keyword octaveFunction contained  trimesh triplequad triplot trisurf triu
syn keyword octaveFunction contained  trnd tsearch tsearchn typecast typeinfo
syn keyword octaveFunction contained  u_test uibuttongroup uicontextmenu
syn keyword octaveFunction contained  uicontrol uigetdir uigetfile uimenu
syn keyword octaveFunction contained  uint16 uint32 uint64 uint8 uipanel
syn keyword octaveFunction contained  uipushtool uiputfile uiresume
syn keyword octaveFunction contained  uitoggletool uitoolbar uiwait umask
syn keyword octaveFunction contained  uminus uname undo_string_escapes unidcdf
syn keyword octaveFunction contained  unidinv unidpdf unidrnd unifcdf unifinv
syn keyword octaveFunction contained  unifpdf unifrnd union unique unix unlink
syn keyword octaveFunction contained  unmkpp unpack unsetenv untabify untar
syn keyword octaveFunction contained  unwrap unzip uplus upper urlread
syn keyword octaveFunction contained  urlwrite usage usejava usleep
syn keyword octaveFunction contained  validateattributes validatestring vander
syn keyword octaveFunction contained  var var_test vec vech vectorize ver
syn keyword octaveFunction contained  version vertcat view viridis voronoi
syn keyword octaveFunction contained  voronoin waitbar waitfor
syn keyword octaveFunction contained  waitforbuttonpress waitpid warndlg
syn keyword octaveFunction contained  warning warning_ids warranty waterfall
syn keyword octaveFunction contained  wavread wavwrite wblcdf wblinv wblpdf
syn keyword octaveFunction contained  wblrnd weekday welch_test white whitebg
syn keyword octaveFunction contained  wienrnd wilcoxon_test wilkinson winter
syn keyword octaveFunction contained  xlabel xlim xor yes_or_no ylabel ylim
syn keyword octaveFunction contained  yulewalker z_test z_test_2 zeros zip
syn keyword octaveFunction contained  zlabel zlim zoom zscore

" Add functions defined in .m file being read to list of highlighted functions
function! s:CheckForFunctions()
  let i = 1
  while i <= line('$')
    let line = getline(i)
    " Only look for functions at start of line.
    " Commented function, '# function', will not trigger as match returns 3
    if match(line, '\Cfunction') == 0
      let line = substitute(line, '\vfunction *([^(]*\= *)?', '', '')
      let nfun = matchstr(line, '\v^\h\w*')
      if !empty(nfun)
        execute "syn keyword octaveFunction" nfun
      endif
    " Include anonymous functions 'func = @(...)'.
    " Use contained keyword to prevent highlighting on LHS of '='
    elseif match(line, '\<\(\h\w*\)\s*=\s*@\s*(') != -1
      let list = matchlist(line, '\<\(\h\w*\)\s*=\s*@\s*(')
      let nfun = list[1]
      if !empty(nfun)
        execute "syn keyword octaveFunction contained" nfun
      endif
    endif
    let i = i + 1
  endwhile
endfunction

call s:CheckForFunctions()

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Define clusters for ease of writing subsequent rules
syn cluster AllFuncVarCmd contains=octaveVariable,octaveFunction,octaveCommand
syn cluster AllFuncSetCmd contains=octaveSetVarFun,octaveFunction,octaveCommand

" Switch highlighting of variables based on coding use.
" Query -> Constant, Set -> Function
" order of items is is important here
syn match octaveQueryVar "\<\h\w*[^(]"me=e-1  contains=@AllFuncVarCmd
syn match octaveSetVar   "\<\h\w*\s*("me=e-1  contains=@AllFuncSetCmd
syn match octaveQueryVar "\<\h\w*\s*\((\s*)\)\@="  contains=@AllFuncVarCmd

" Don't highlight Octave keywords on LHS of '=', these are user vars
syn match octaveUserVar  "\<\h\w*\ze[^<>!~="']\{-}==\@!"
syn match octaveUserVar  "\<\h\w*\s*[<>!~=]=" contains=octaveVariable

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Errors (placed early so they may be overriden by more specific rules
" Struct with nonvalid identifier starting with number (Example: 1.a or a.1b)
syn region octaveError  start="\<\d\+\(\w*\.\)\@="  end="[^0-9]"he=s-1 oneline
syn region octaveError  start="\.\d\+\(\w*\)\@="hs=s+1  end="[^0-9]"he=s-1 oneline
" Numbers with double decimal points (Example: 1.2.3)
syn region octaveError  start="\<-\?\d\+\.\d\+\.[^*/\\^]"hs=e-1 end="\>"  oneline
syn region octaveError  start="\<-\?\d\+\.\d\+[eEdD][-+]\?\d\+\.[^*/\\^]"hs=e-1 end="\>"  oneline

" Operators
" Uncommment "Hilink octaveOperator" below to highlight these
syn match octaveLogicalOperator     "[&|~!]"
syn match octaveArithmeticOperator  "\.\?[-+*/\\^]"
syn match octaveRelationalOperator  "[=!~]="
syn match octaveRelationalOperator  "[<>]=\?"

" User Variables
" Uncomment this syntax group and "Hilink octaveIdentifier" below to highlight
"syn match octaveIdentifier  "\<\h\w*\>"

" Strings
syn region octaveString  start=/'/  end=/'/  skip=/''/ contains=octaveLineContinuation,@Spell
syn region octaveString  start=/"/  end=/"/  skip=/\\./re=e+1 contains=octaveLineContinuation,@Spell

" Standard numbers
syn match octaveNumber  "\<\d\+[ij]\?\>"
" Floating point number, with dot, optional exponent
syn match octaveFloat   "\<\d\+\(\.\d*\)\?\([edED][-+]\?\d\+\)\?[ij]\?\>"
" Floating point number, starting with a dot, optional exponent
syn match octaveFloat   "\.\d\+\([edED][-+]\?\d\+\)\?[ij]\?\>"
" Hex numbers
syn match octaveNumber  "\<0[xX][0-9a-fA-F][0-9a-fA-F]\+\>"
" Binary numbers
syn match octaveNumber  "\<0[bB][01][01]\+\>"

" Delimiters and transpose character
syn match octaveDelimiter          "[][(){}@]"
syn match octaveTransposeOperator  "[])}[:alnum:]._]\@<='"

" Tabs, for possibly highlighting as errors
syn match octaveTab  "\t"
" Other special constructs
syn match octaveSemicolon  ";"
syn match octaveTilde "\~\s*[[:punct:]]"me=e-1

" Line continuations, order of matches is important here
syn match octaveLineContinuation  "\.\{3}$"
syn match octaveLineContinuation  "\\$"
syn match octaveError  "\.\{3}.\+$"hs=s+3
syn match octaveError  "\\\s\+$"hs=s+1
" Line continuations w/comments
syn match octaveLineContinuation  "\.\{3}\s*[#%]"me=e-1
syn match octaveLineContinuation  "\\\s*[#%]"me=e-1

" Comments, order of matches is important here
syn keyword octaveFIXME contained  FIXME TODO
syn match  octaveComment  "[%#].*$"  contains=octaveFIXME,octaveTab,@Spell
syn match  octaveError    "[#%][{}]"
syn region octaveBlockComment  start="^\s*[#%]{\s*$"  end="^\s*[#%]}\s*$" contains=octaveFIXME,octaveTab,@Spell

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Apply highlight groups to syntax groups defined above

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_octave_syntax_inits")
  if version < 508
    let did_octave_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink octaveBeginKeyword             Conditional
  HiLink octaveElseKeyword              Conditional
  HiLink octaveEndKeyword               Conditional
  HiLink octaveVarKeyword               Conditional
  HiLink octaveReserved                 Conditional

  HiLink octaveStatement                Statement
  HiLink octaveVariable                 Constant
  HiLink octaveSetVarFun                Function
  HiLink octaveCommand                  Statement
  HiLink octaveFunction                 Function

  HiLink octaveConditional              Conditional
  HiLink octaveLabel                    Label
  HiLink octaveRepeat                   Repeat
  HiLink octaveFIXME                    Todo
  HiLink octaveString                   String
  HiLink octaveDelimiter                Identifier
  HiLink octaveNumber                   Number
  HiLink octaveFloat                    Float
  HiLink octaveError                    Error
  HiLink octaveComment                  Comment
  HiLink octaveBlockComment             Comment
  HiLink octaveSemicolon                SpecialChar
  HiLink octaveTilde                    SpecialChar
  HiLink octaveLineContinuation         Special

  HiLink octaveTransposeOperator        octaveOperator
  HiLink octaveArithmeticOperator       octaveOperator
  HiLink octaveRelationalOperator       octaveOperator
  HiLink octaveLogicalOperator          octaveOperator

" Optional highlighting
"  HiLink octaveOperator                Operator
"  HiLink octaveIdentifier              Identifier
"  HiLink octaveTab                     Error

  delcommand HiLink
endif

let b:current_syntax = "octave"

"EOF	vim: ts=2 et tw=80 sw=2 sts=0
