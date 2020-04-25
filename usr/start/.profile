set -a
CDPATH=:/usr/start
EXINIT="set wm=10"
PATH=$PATH:$HOME:.
SHELL=/bin/sh
TERM=`tset - -AIQ `
set +a
mail ()
{
	mailx $*
}
date
