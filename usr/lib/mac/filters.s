#
# file: filters.s
# Copyright 1987 Apple Computers, Inc.
#
# Example assembler wrapper routines to allow UNIX C routines to be called
# from the Macintosh ROMs.

	text

#
# Control Manager
#

# Action procedure for use when mouse was pressed in an indicator:

# void IMyAction()

	global	aIMyAction
aIMyAction:
	jmp	IMyAction	# No parameters, so no work to do
				# Wrapper routine is not needed in this case

# Action procedure for use when mouse was not pressed in an indicator:

# void		MyAction(theControl,partCode)
# ControlHandle	theControl;
# int		partCode;

	global	aMyAction
aMyAction:
	mov.w	4(%a7),-(%a7)
	clr.w	-(%a7)
	mov.l	10(%a7),-(%a7)
	jsr	MyAction
	mov.l	8(%a7),%a1
	add.l	&18,%a7
	jmp	(%a1)

# Control definition function:

# long		MyControl(varCode,theControl,message,param)
# int		varCode;
# ControlHandle	theControl;
# int		message;
# long		param;

	global	aMyControl
aMyControl:
	mov.l	4(%a7),-(%a7)
	mov.w	12(%a7),-(%a7)
	clr.w	-(%a7)
	mov.l	18(%a7),-(%a7)
	mov.w	26(%a7),-(%a7)
	clr.w	-(%a7)
	jsr	MyControl
	mov.l	16(%a7),%a1
	add.l	&32,%a7
	mov.l	%d0,(%a7)
	jmp	(%a1)

#
# Dialog Manager
#

# User item procedure:

# void		MyItem(theWindow,itemNo)
# WindowPtr	theWindow;
# int		itemNo;

	global	aMyItem
aMyItem:
	mov.w	4(%a7),-(%a7)
	clr.w	-(%a7)
	mov.l	10(%a7),-(%a7)
	jsr	MyItem
	mov.l	8(%a7),%a1
	add.l	&18,%a7
	jmp	(%a1)

# Sound procedure:

# void		MySound(soundNo)
# int		soundNo;

	global	aMySound
aMySound:
	mov.w	4(%a7),-(%a7)
	clr.w	-(%a7)
	jsr	MySound
	mov.l	4(%a7),%a1
	add.l	&10,%a7
	jmp	(%a1)

# ModalDialog filter:

# char		MyFilter(theDialog,theEvent,itemHit)
# DialogPtr	theDialog;
# EventRecord	*theEvent;
# short		*itemHit;

	global	aMyFilter
aMyFilter:
	mov.l	4(%a7),-(%a7)
	mov.l	12(%a7),-(%a7)
	mov.l	20(%a7),-(%a7)
	jsr	MyFilter
	mov.l	12(%a7),%a1
	add.l	&28,%a7
	mov.b	%d0,(%a7)
	jmp	(%a1)

#
# Menu Manager
#

# Menu Definition Procedure:

# void		MyMenu(message,theMenu,menuRect,hitPt,whichItem)
# int		message;
# MenuHandle	theMenu;
# Rect		*menuRect;
# Point		hitPt;
# short		*whichItem;

	global	aMyMenu
aMyMenu:
	mov.l	4(%a7),-(%a7)
	mov.l	12(%a7),-(%a7)
	mov.l	20(%a7),-(%a7)
	mov.l	28(%a7),-(%a7)
	mov.w	36(%a7),-(%a7)
	clr.w	-(%a7)
	jsr	MyMenu
	mov.l	20(%a7),%a1
	add.l	&42,%a7
	jmp	(%a1)

#
# StdFile Package
#

# Dialog hook function:

# int		MyDlg(item,theDialog)
# int		item;
# DialogPtr	*theDialog;

	global	aMyDlg
aMyDlg:
	mov.l	4(%a7),-(%a7)
	mov.w	12(%a7),-(%a7)
	clr.w	-(%a7)
	jsr	MyDlg
	mov.l	8(%a7),%a1
	add.l	&18,%a7
	mov.w	%d0,(%a7)
	jmp	(%a1)

# File filter function:

# char		MyFileFilter(paramBlock)
# ParmBlkPtr	paramBlock;

	global	aMyFileFilter
aMyFileFilter:
	mov.l	4(%a7),-(%a7)
	jsr	MyFileFilter
	mov.l	4(%a7),%a1
	add.l	&12,%a7
	mov.b	%d0,(%a7)
	jmp	(%a1)

#
# TextEdit
#

# Word break function:

# char		MyWordBreak(text,charPos)
# Ptr		text;
# int		charPos;

	global	aMyWordBreak
aMyWordBreak:
	mov.w	4(%a7),-(%a7)
	clr.w	-(%a7)
	mov.l	10(%a7),-(%a7)
	jsr	MyWordBreak
	mov.l	8(%a7),%a1
	add.l	&18,%a7
	mov.b	%d0,(%a7)
	jmp	(%a1)

# Click loop routine:

# char		MyClikLoop()

	global	aMyClikLoop
aMyClikLoop:
	jsr	MyClikLoop
	mov.b	%d0,5(%a7)
	rts

#
# Window Manager
#

# Window definition procedure:

# long		MyWindow(varCode,theWindow,message,param)
# int		varCode;
# WindowPtr	theWindow;
# int		message;
# long		param;

	global	aMyWindow
aMyWindow:
	mov.l	4(%a7),-(%a7)
	mov.w	12(%a7),-(%a7)
	clr.w	-(%a7)
	mov.l	18(%a7),-(%a7)
	mov.w	26(%a7),-(%a7)
	clr.w	-(%a7)
	jsr	MyWindow
	mov.l	16(%a7),%a1
	add.l	&32,%a7
	mov.l	%d0,(%a7)
	jmp	(%a1)

