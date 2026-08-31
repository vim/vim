" Vim syntax file
" Language: Svelte
" Maintainer: 231tr0n
" Last Change: 2026 Aug 29

" Quit if a syntax file was already loaded.
if exists("b:current_syntax")
  finish
endif

" <script lang="ts"> should use TypeScript highlighting.  html.vim only knows
" about JavaScript, so pull in the TypeScript syntax first and replace its
" <script> region (cleared below) with two variants: a TypeScript one for
" lang="ts" and the original JavaScript one for everything else.
" The include sets b:current_syntax; clear it before loading html.vim so that
" html.vim does not bail out early (it would otherwise leave its <script>
" region, and therefore javaScript, undefined).
syntax include @htmlTypeScript syntax/typescript.vim
unlet! b:current_syntax

" Svelte files are HTML with embedded CSS, JavaScript and a {} expression
" syntax, so the HTML syntax (which already handles <script> and <style>)
" gives us most of what we need.
runtime! syntax/html.vim

syntax clear javaScript
syntax region javaScript matchgroup=htmlScriptTag
      \ start=+<script\>\_[^>]*>+ keepend
      \ end=+</script\_[^>]*>+me=s-1
      \ contains=@htmlJavaScript,htmlCssStyleComment,htmlScriptTag,@htmlPreproc
syntax region svelteScriptTS matchgroup=htmlScriptTag
      \ start=+<script\_[^>]*lang=["']ts["']>+ keepend
      \ end=+</script\_[^>]*>+me=s-1
      \ contains=@htmlTypeScript,htmlCssStyleComment,htmlScriptTag,@htmlPreproc

" Svelte special component elements: <svelte:head>, <svelte:window>, ...
syntax match svelteComponent "\v<svelte:(head|body|window|document|options|element|boundary|component|fragment|self)>" containedin=htmlTagN

" Svelte 5 runes (and store auto-subscriptions) inside <script> blocks.
syntax match svelteRune "\$\w\+" containedin=javaScript,svelteScriptTS

" Interpolation and expressions: { expr }
syntax region svelteMustache matchgroup=svelteBraces
      \ start=+\v\{+ end=+\v\}+
      \ containedin=htmlString,htmlValue keepend

" Control-flow and special blocks.  Covers both legacy Svelte and Svelte 5:
"   {#if} {:else} {/if} {#each} {:then} {:catch} {/each}
"   {#await} {#key} {#snippet} {@html} {@const} {@debug} {@render}
syntax region svelteBlock matchgroup=svelteKeyword
      \ start=+\v\{[#/:@]\w*+ end=+\v\}+ keepend

" Element directives:
"   on:click bind:value class:active use:foo transition:fade in:/out:/animate:
"   let:item slot: style:  (modifiers via "|": on:click|once)
syntax match svelteDirective
      \ "\v(on|bind|class|use|transition|in|out|animate|let|slot|style):[a-zA-Z0-9_-]+(\|[a-zA-Z0-9_-]+)*"
      \ containedin=htmlTag contained

highlight default link svelteComponent htmlTagName
highlight default link svelteRune	 Statement
highlight default link svelteBraces	 Delimiter
highlight default link svelteKeyword	 Statement
highlight default link svelteDirective	 Type

let b:current_syntax = "svelte"
