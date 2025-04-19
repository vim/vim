" Vim formatter file
" Formatter:   Prettier for JavaScript and other languages
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2025 Apr 18

" At the time of writing, Prettier supports the following languages:
" - JavaScript (including experimental features)
" - JSX
" - Angular
" - Vue
" - Flow
" - TypeScript
" - CSS, Less, and SCSS
" - HTML
" - Ember/Handlebars
" - JSON
" - GraphQL
" - Markdown, including GFM and MDX v1
" - YAML

if exists("current_formatter")
  finish
endif
let current_formatter = "prettier"

FormatterSet formatprg=npx\ --yes\ prettier\ --stdin-filepath\ %
FormatterSet formatexpr=format#FormatExpr()
