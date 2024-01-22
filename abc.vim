" Vim syntax file
" Language:   ABC (A Bloody Compiler)
" Maintainer: Michael Christian Lehn <michael.lehn@uni-ulm.de>
" Last Change:        2022-7-10
" License:            Vim (see :h license)

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syntax case match

" syntax match abcIdentifier /[A-Za-z_][A-Za-z0-9_]*/ skipwhite
" syntax match abcMinus /[-]/ contained skipwhite
syntax match abcIdentifier /[A-Za-z_][A-Za-z0-9_]*/
 
syntax match keyword /fn/ skipwhite
syntax match keyword /for/ skipwhite
syntax match keyword /while/ skipwhite
syntax match keyword /if/ skipwhite
syntax match keyword /else/ skipwhite
syntax match keyword /local/ skipwhite
syntax match keyword /global/ skipwhite
syntax match keyword /return/ skipwhite
syntax match keyword /array/ skipwhite
syntax match keyword /of/ skipwhite
syntax match keyword /type/ skipwhite
syntax match keyword /let/ skipwhite
syntax match keyword /sizeof/ skipwhite
syntax match keyword /cast/ skipwhite
syntax match keyword /break/ skipwhite
syntax match keyword /continue/ skipwhite
syntax match keyword /switch/ skipwhite
syntax match keyword /case/ skipwhite
syntax match keyword /default/ skipwhite

syntax match type /ro/ skipwhite
syntax match type /u8/ skipwhite
syntax match type /u16/ skipwhite
syntax match type /u32/ skipwhite
syntax match type /u64/ skipwhite
syntax match type /struct/ skipwhite
syntax match type /union/ skipwhite
 
syntax match type /i8/ skipwhite
syntax match type /i16/ skipwhite
syntax match type /i32/ skipwhite
syntax match type /i64/ skipwhite

syn match ty "->|...|[A-Za-z][0-9A-Za-z]*" contained
syntax region type matchgroup=buflit start=/:/ end=/[,;)=\n]/ contains=ty

syntax match literal /[+-]*[1-9][0-9]*/ skipwhite
syntax match literal /nullptr/ skipwhite
syntax match literal /[0-7][0-7]*/ skipwhite
syntax match literal /0x[0-9a-zA-Z][0-9a-zA-Z]*/ skipwhite
syntax region literal start=/"/ skip=/\\"/ end=/"/ skipwhite
syntax region literal start=/'/ skip=/\\'/ end=/'/ skipwhite

syntax region comment start="//" end="$" skipwhite
syntax region comment start="/\*" end="\*/" skipwhite

highlight link keyword Statement
highlight link type Type
highlight link literal Number
highlight link comment Comment

let b:current_syntax = "abc"
