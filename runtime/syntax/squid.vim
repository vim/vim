" Vim syntax file
" Language:	Squid config file
" Maintainer:	Klaus Muth <klaus@hampft.de>
" Last Change:	2004 Feb 01
" URL:		http://www.hampft.de/vim/syntax/squid.vim
" ThanksTo:	Ilya Sher <iso8601@mail.ru>


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" squid.conf syntax seems to be case insensitive
syn case ignore

syn keyword	squidTodo	contained TODO
syn match	squidComment	"#.*$" contains=squidTodo,squidTag
syn match	squidTag	contained "TAG: .*$"

" Lots & lots of Keywords!
syn keyword	squidConf	acl always_direct announce_host
syn keyword	squidConf	announce_period announce_port announce_to
syn keyword	squidConf	anonymize_headers append_domain
syn keyword	squidConf	as_whois_server authenticate_children
syn keyword	squidConf	authenticate_program authenticate_ttl
syn keyword	squidConf	broken_posts buffered_logs cache_access_log
syn keyword	squidConf	cache_announce cache_dir cache_dns_program
syn keyword	squidConf	cache_effective_group cache_effective_user
syn keyword	squidConf	cache_host cache_host_acl cache_host_domain
syn keyword	squidConf	cache_log cache_mem cache_mem_high
syn keyword	squidConf	cache_mem_low cache_mgr cachemgr_passwd
syn keyword	squidConf	cache_peer cache_stoplist
syn keyword	squidConf	cache_stoplist_pattern cache_store_log
syn keyword	squidConf	cache_swap cache_swap_high cache_swap_log
syn keyword	squidConf	cache_swap_low client_db client_lifetime
syn keyword	squidConf	client_netmask connect_timeout coredump_dir
syn keyword	squidConf	dead_peer_timeout debug_options delay_access
syn keyword	squidConf	delay_class delay_initial_bucket_level
syn keyword	squidConf	delay_parameters delay_pools dns_children
syn keyword	squidConf	dns_defnames dns_nameservers dns_testnames
syn keyword	squidConf	emulate_httpd_log err_html_text
syn keyword	squidConf	fake_user_agent firewall_ip forwarded_for
syn keyword	squidConf	forward_snmpd_port fqdncache_size
syn keyword	squidConf	ftpget_options ftpget_program ftp_list_width
syn keyword	squidConf	ftp_user half_closed_clients
syn keyword	squidConf	hierarchy_stoplist htcp_port http_access
syn keyword	squidConf	http_anonymizer httpd_accel httpd_accel_host
syn keyword	squidConf	httpd_accel_port httpd_accel_uses_host_header
syn keyword	squidConf	httpd_accel_with_proxy http_port
syn keyword	squidConf	http_reply_access icp_access icp_hit_stale
syn keyword	squidConf	icp_port icp_query_timeout ident_lookup
syn keyword	squidConf	ident_lookup_access ident_timeout
syn keyword	squidConf	incoming_http_average incoming_icp_average
syn keyword	squidConf	inside_firewall ipcache_high ipcache_low
syn keyword	squidConf	ipcache_size local_domain local_ip
syn keyword	squidConf	logfile_rotate log_fqdn log_icp_queries
syn keyword	squidConf	log_mime_hdrs maximum_object_size
syn keyword	squidConf	maximum_single_addr_tries mcast_groups
syn keyword	squidConf	mcast_icp_query_timeout mcast_miss_addr
syn keyword	squidConf	mcast_miss_encode_key mcast_miss_port
syn keyword	squidConf	memory_pools mime_table min_http_poll_cnt
syn keyword	squidConf	min_icp_poll_cnt minimum_direct_hops
syn keyword	squidConf	minimum_retry_timeout miss_access
syn keyword	squidConf	negative_dns_ttl negative_ttl
syn keyword	squidConf	neighbor_timeout neighbor_type_domain
syn keyword	squidConf	netdb_high netdb_low netdb_ping_period
syn keyword	squidConf	netdb_ping_rate no_cache passthrough_proxy
syn keyword	squidConf	pconn_timeout pid_filename pinger_program
syn keyword	squidConf	positive_dns_ttl prefer_direct proxy_auth
syn keyword	squidConf	proxy_auth_realm query_icmp quick_abort
syn keyword	squidConf	quick_abort quick_abort_max quick_abort_min
syn keyword	squidConf	quick_abort_pct range_offset_limit
syn keyword	squidConf	read_timeout redirect_children
syn keyword	squidConf	redirect_program
syn keyword	squidConf	redirect_rewrites_host_header reference_age
syn keyword	squidConf	reference_age refresh_pattern reload_into_ims
syn keyword	squidConf	request_size request_timeout
syn keyword	squidConf	shutdown_lifetime single_parent_bypass
syn keyword	squidConf	siteselect_timeout snmp_access
syn keyword	squidConf	snmp_incoming_address snmp_port source_ping
syn keyword	squidConf	ssl_proxy store_avg_object_size
syn keyword	squidConf	store_objects_per_bucket strip_query_terms
syn keyword	squidConf	swap_level1_dirs swap_level2_dirs
syn keyword	squidConf	tcp_incoming_address tcp_outgoing_address
syn keyword	squidConf	tcp_recv_bufsize test_reachability
syn keyword	squidConf	udp_hit_obj udp_hit_obj_size
syn keyword	squidConf	udp_incoming_address udp_outgoing_address
syn keyword	squidConf	unique_hostname unlinkd_program
syn keyword	squidConf	uri_whitespace useragent_log visible_hostname
syn keyword	squidConf	wais_relay wais_relay_host wais_relay_port

syn keyword	squidOpt	proxy-only weight ttl no-query default
syn keyword	squidOpt	round-robin multicast-responder
syn keyword	squidOpt	on off all deny allow

" Security Actions for cachemgr_passwd
syn keyword	squidAction	shutdown info parameter server_list
syn keyword	squidAction	client_list
syn match	squidAction	"stats/\(objects\|vm_objects\|utilization\|ipcache\|fqdncache\|dns\|redirector\|io\|reply_headers\|filedescriptors\|netdb\)"
syn match	squidAction	"log\(/\(status\|enable\|disable\|clear\)\)\="
syn match	squidAction	"squid\.conf"

" Keywords for the acl-config
syn keyword	squidAcl	url_regex urlpath_regex referer_regex port proto
syn keyword	squidAcl	req_mime_type rep_mime_type
syn keyword	squidAcl	method browser user src dst

syn match	squidNumber	"\<\d\+\>"
syn match	squidIP		"\<\d\{1,3}\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}\>"
syn match	squidStr	"\(^\s*acl\s\+\S\+\s\+\(\S*_regex\|re[pq]_mime_type\|browser\|_domain\|user\)\+\s\+\)\@<=.*" contains=squidRegexOpt
syn match	squidRegexOpt	contained "\(^\s*acl\s\+\S\+\s\+\S\+\(_regex\|_mime_type\)\s\+\)\@<=[-+]i\s\+"

" All config is in one line, so this has to be sufficient
" Make it fast like hell :)
syn sync minlines=3

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_squid_syntax_inits")
  if version < 508
    let did_squid_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink squidTodo	Todo
  HiLink squidComment	Comment
  HiLink squidTag	Special
  HiLink squidConf	Keyword
  HiLink squidOpt	Constant
  HiLink squidAction	String
  HiLink squidNumber	Number
  HiLink squidIP	Number
  HiLink squidAcl	Keyword
  HiLink squidStr	String
  HiLink squidRegexOpt	Special

  delcommand HiLink
endif

let b:current_syntax = "squid"

" vim: ts=8
