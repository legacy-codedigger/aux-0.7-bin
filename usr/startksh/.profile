set -a
CDPATH=:/usr/start
EDITOR=vi 
EXINIT="set wm=10"
FCEDIT=vi
HISTSIZE=60
PATH=$PATH:~:.
PS1="!$ "
SHELL=/bin/ksh
TERM=`tset - -AIQ `
set +a
if test -f ~/.kshrc
then 
	. ~/.kshrc
	ENV=~/.kshrc
fi
date
