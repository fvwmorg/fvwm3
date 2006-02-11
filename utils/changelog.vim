" File: changelog.vim
" Summary: A function adding a ChangeLog entry.
" Author: David Necas (Yeti) <yeti@physics.muni.cz>
" URL: http://trific.ath.cx/Ftp/vim/scripts/changelog.vim
" License: This Vim script is in the public domain.
" Version: 2004-10-08
" Usage: Do :ChangeLog inside a function you've just changed.
" Customization:
" Set your name and e-mail in .vimrc:
"   let changelog_maintainer_name = "Me <me@example.net>"
" Set function name matching regex for particular filetype:
"   let b:changelog_function_re = "^\\s*sub\\s*"
"   let b:changelog_function_re = "^\\s*\\(def\\|class\\)\\s*"
"   let b:changelog_function_re = "^\\(procedure\\|function\\)\\s\\+"
"   ...
" The default is "^" appropriate for C, "" switches function names off.

command! -nargs=0 ChangeLog call <SID>ChangeLog()
function! s:ChangeLog()
  " Maintainer's name, try to guess when undefined
  if !exists('g:changelog_maintainer_name')
    echoerr 'changelog_maintainer_name not defined!  guessing...'
    let node=substitute(system('hostname -f'), "\n", '', 'g')
    let usrinfo=system('grep ^`id -un`: /etc/passwd')
    let login=matchstr(usrinfo,'^\w\+')
    let t=matchend(usrinfo,'\w\+:[^:]\+:\d\+:\d\+:')
    let name=matchstr(usrinfo,'[^:]\+',t)
    let g:changelog_maintainer_name=name.' <'.login.'@'.node.'>'
  endif
  " Find current function name
  let l=line('.')
  let c=col('.')
  if exists('b:changelog_function_re')
    if strlen(b:changelog_function_re) == 0
      let re=''
    else
      let re=b:changelog_function_re.'\w\+\s*[({]'
    endif
  else
    let re='^\w\+\s*[({]'
  endif
  if strlen(re) > 0 && search(re, 'bW') > 0
    let foo=matchstr(getline('.'),'\w\+\(\s*[({]\)\@=')
    call cursor(l,c)
    if strlen(foo) > 0
      let foo=' ('.foo.')'
    endif
  else
    let foo=''
  endif
  " Find and open the ChangeLog
  let f=expand('%:p:h')
  while strlen(f)>1 && !filewritable(f.'/ChangeLog')
    let f=fnamemodify(f,':h')
  endwhile
  let rf=strpart(expand('%:p'),strlen(f)+1)  " Relativize filename
  let f=f.'/ChangeLog'
  if !filewritable(f)
    echoerr "Cannot find ChangeLog in parent directories"
    return
  endif
  execute "split ".f
  " Add the entry
  call cursor(1,1)
  " FIXME: If changelog_time_format changes, this should change too
  call search('^\u\l\l \u\l\l \+\d\+ \d\d:\d\d:\d\d ', 'W')
  call cursor(line('.')-1,0)
  call append('.','')
  call append('.','      ')    " Some people may want a TAB here
  if exists('g:changelog_time_format')
    let timefmt=g:changelog_time_format
  else
    " Try to emulate date(1) output while being fairly portable (incl. Win32)
    let timefmt="%a %b %d %H:%M:%S %Z %Y"
  endif
  call append('.','    * '.rf.foo.':')    " Some people may want a TAB here
  call append('.',strftime(timefmt).'  '.g:changelog_maintainer_name)
  call cursor(line('.')+3,10000)
endfunction

