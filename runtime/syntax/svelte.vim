" Vim syntax file
" Language: Svelte
" Maintainer: 231tr0n
" Last Change: 2026 Sep 18

" Quit if a syntax file was already loaded.
if exists("b:current_syntax")
  finish
endif

" Svelte is HTML markup with embedded { } expressions, CSS and
" JavaScript/TypeScript.  Load html.vim for base syntax, then add
" Svelte-specific constructs on top.

" Disable html.vim's rendering regions (bold, italic, etc.) — not needed for
" Svelte templates and they interfere with { } expression highlighting.
if !exists("html_no_rendering")
  let html_no_rendering = 1
  let s:restore_html_no_rendering = 1
endif

runtime! syntax/html.vim
unlet! b:current_syntax

if exists("s:restore_html_no_rendering")
  unlet html_no_rendering
endif

" Embedded scripting languages: JavaScript for <script>, TypeScript for
" <script lang="ts"> and CSS for <style>.  TypeScript must be included before
" JavaScript: the include scripts set b:current_syntax and bail out early
" when it is already set, so clear it after each include.
syntax include @svelteTypeScript syntax/typescript.vim
unlet! b:current_syntax
syntax include @svelteJavaScript syntax/javascript.vim
unlet! b:current_syntax
syntax include @svelteCSS syntax/css.vim
unlet! b:current_syntax

" Override <script> / <style> regions to use Svelte's embedded languages
" instead of html.vim's default JavaScript/CSS.
syntax region svelteScriptJS matchgroup=svelteScriptTag
      \ start=+<script\>\_[^>]*>+ keepend
      \ end=+</script\_[^>]*>+me=s-1
      \ contains=@svelteJavaScript,htmlPreProc,svelteScriptTag,htmlEvent
syntax region svelteScriptTS matchgroup=svelteScriptTag
      \ start=+<script\_[^>]*lang=["']ts["']>+ keepend
      \ end=+</script\_[^>]*>+me=s-1
      \ contains=@svelteTypeScript,htmlPreProc,svelteScriptTag,htmlEvent
syntax region svelteStyle matchgroup=svelteStyleTag
      \ start=+<style\>\_[^>]*>+ keepend
      \ end=+</style\_[^>]*>+me=s-1
      \ contains=@svelteCSS,svelteStyleTag,htmlPreProc

" <script> / <style> opening tags.  These also count as htmlTag so that any
" Svelte-specific markup inside the tag is recognized.
syntax region svelteScriptTag contained start=+<script+ end=+>+ fold
      \ contains=htmlTagN,htmlString,htmlArg,htmlValue
syntax region svelteStyleTag contained start=+<style+ end=+>+ fold
      \ contains=htmlTagN,htmlString,htmlArg,htmlValue

" Svelte special component elements: <svelte:head>, <svelte:window>, ...
syntax match svelteComponent contained "svelte:\%(head\|body\|window\|document\|options\|element\|boundary\|component\|fragment\|self\)\>" containedin=htmlTagN

" Svelte 5 runes (and store auto-subscriptions) inside <script> blocks.
" Also matches dot-notation rune variants: $state.raw, $derived.by,
" $effect.pre, $effect.tracking, $effect.pending, $effect.root, $props.id
syntax match svelteRune "\$\w\+\%(\.\w\+\)\?" containedin=svelteScriptJS,svelteScriptTS

" Interpolation and expressions: { expr }
" Match inside tags, attribute values, and at top level (but not inside
" <script> blocks where { are JavaScript block delimiters).
syntax region svelteMustache matchgroup=svelteBraces
      \ start=+\v\{+ end=+\v\}+
      \ containedin=htmlTag,htmlString,htmlValue keepend

" Control-flow and special blocks.  Covers both legacy Svelte and Svelte 5:
"   {#if} {:else} {/if} {#each} {:then} {:catch} {/each}
"   {#await} {#key} {#snippet} {@html} {@const} {@debug} {@render}
syntax region svelteBlock matchgroup=svelteKeyword
      \ start=/\v\{[#/:@]\w+/ end=/\v\}/
      \ containedin=htmlTag,htmlString,htmlValue keepend

" Top-level mustache and block expressions (outside tags)
syntax region svelteMustacheTop matchgroup=svelteBraces
      \ start=+\v\{+ end=+\v\}+
      \ containedin=TOP keepend
syntax region svelteBlockTop matchgroup=svelteKeyword
      \ start=/\v\{[#/:@]\w+/ end=/\v\}/ containedin=TOP keepend

" Element directives:
"   on:click bind:value class:active use:foo transition:fade in:/out:/animate:
"   let:item slot: style:  (modifiers via "|": on:click|once)
syntax match svelteDirective contained
      \ "\v(on|bind|class|use|transition|in|out|animate|let|slot|style):[a-zA-Z0-9_-]+(\|[a-zA-Z0-9_-]+)*(\_s*[>=\s/])\@="
      \ containedin=htmlTag

" Highlight links: reuse html.vim groups where possible, add Svelte-specific ones
highlight default link svelteScriptTag htmlTag
highlight default link svelteStyleTag htmlTag
highlight default link svelteComponent htmlSpecialTagName
highlight default link svelteRune Statement
highlight default link svelteBraces Delimiter
highlight default link svelteKeyword Statement
highlight default link svelteDirective htmlArg
highlight default link svelteMustacheTop svelteMustache
highlight default link svelteBlockTop svelteBlock

let b:current_syntax = "svelte"
