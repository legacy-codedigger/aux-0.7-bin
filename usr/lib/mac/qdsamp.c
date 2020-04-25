/*
 * Copyright 1987 Apple Computer, Inc.
 * $Header: qdsamp.c,v 1.2 87/01/21 17:54:13 rpd Exp $
 *
 * $Log:	qdsamp.c,v $
 * Revision 1.2  87/01/21  17:54:13  rpd
 * removed initMac call
 * 
 * Revision 1.1  87/01/15  22:15:11  rpd
 * Initial revision
 * 
 */

/*
    File: qdsamp.c

    This is a version of the old QDSamp program that shows QuickDraw off
a little bit.  This version will compile and run from MPW as well.
*/

#include "types.h"
#include "quickdraw.h"
#include "fonts.h"
#include <stdio.h>

/* space for six (6) thirty-two-by-thirty-two (32x32) icons */
long	icons[6][32];

void InitIcons ()
{
    /* icon #0 */
    StuffHex(&icons[0][ 0],"0000000007FFFFE00800001008000010");
    StuffHex(&icons[0][ 4],"08FFFF10090000900955509009000090");
    StuffHex(&icons[0][ 8],"09550090090000900950009009000090");
    StuffHex(&icons[0][12],"09500090090000900954009009000090");
    StuffHex(&icons[0][16],"0900009008FFFF100800001008000010");
    StuffHex(&icons[0][20],"08000010080000100800FF1008000010");
    StuffHex(&icons[0][24],"08000010080000100800001007FFFFE0");
    StuffHex(&icons[0][28],"04000020040000200400002007FFFFE0");

    /* icon #1 */
    StuffHex(&icons[1][ 0],"FC1F83FC84108204B41682F4B416BAF4");
    StuffHex(&icons[1][ 4],"8770EEF495D2820484108FFFFC1F8801");
    StuffHex(&icons[1][ 8],"4000080160000FFF2000000420000004");
    StuffHex(&icons[1][12],"FC00007E84000042B41FE05AB790205A");
    StuffHex(&icons[1][16],"84F7A0429417A04A8417A042FC10207E");
    StuffHex(&icons[1][20],"007FF8040040080C00400808007FF80C");
    StuffHex(&icons[1][24],"FC000004840003FFB4000201B4000281");
    StuffHex(&icons[1][28],"85C00E01977FF9FF84000082FC0000FE");

    /* icon #2 */
    StuffHex(&icons[2][ 0],"00000404000000080000021000004020");
    StuffHex(&icons[2][ 4],"0000110000000400000F801500106380");
    StuffHex(&icons[2][ 8],"00201C00002000200020049001FC0808");
    StuffHex(&icons[2][12],"01FC108401FC000007FF00801FFFC000");
    StuffHex(&icons[2][16],"1FFFC0003FFFE0003FFFE0007FFF7000");
    StuffHex(&icons[2][20],"7FFFF0007FFFB0007FFFB0007FFFB000");
    StuffHex(&icons[2][24],"7FFFF0007FFF70003FFFE0003FFDE000");
    StuffHex(&icons[2][28],"1FFFC0001FFFC00007FF000000F80000");

    /* icon #3 */
    StuffHex(&icons[3][ 0],"00000000000000000000000000000000");
    StuffHex(&icons[3][ 4],"07FFFF00040001000500050004000100");
    StuffHex(&icons[3][ 8],"0516C50004000100051B450004000100");
    StuffHex(&icons[3][12],"051DC50004000100051F450004000100");
    StuffHex(&icons[3][16],"051DC500040001000500050004000100");
    StuffHex(&icons[3][20],"FFFFFFFF90000049FFFFFFF990000049");
    StuffHex(&icons[3][24],"9FFFFFC980000009FFFFFFF98000000F");
    StuffHex(&icons[3][28],"800000089000000880000008FFFFFFF8");

    /* icon #4 */
    StuffHex(&icons[4][ 0],"00063000001F4800003F800000000000");
    StuffHex(&icons[4][ 4],"017FFE000208210003FF7D0004010080");
    StuffHex(&icons[4][ 8],"05FFF780080000400E7F3E4010410220");
    StuffHex(&icons[4][12],"1389F1E0200800103FFFFF9040000008");
    StuffHex(&icons[4][16],"78fff8f880000404fff8fffc80088004");
    StuffHex(&icons[4][20],"80088004800880c480088324fff8cc2c");
    StuffHex(&icons[4][24],"00001320000014e00000102000001120");
    StuffHex(&icons[4][28],"00000c20000003400000008000000000");

    /* icon #5 */
    StuffHex(&icons[5][ 0],"03fffff8060000380c000078180000e8");
    StuffHex(&icons[5][ 4],"3fffffc8200000883ffffe882e6cc688");
    StuffHex(&icons[5][ 8],"53932e88fffffe8880001e8881e01e88");
    StuffHex(&icons[5][12],"81201e8881e01e8880001e8883301e88");
    StuffHex(&icons[5][16],"81e01e8880001c8880001e88fffff288");
    StuffHex(&icons[5][20],"281e028828120288281e028828000288");
    StuffHex(&icons[5][24],"28330288281e02882800028828000298");
    StuffHex(&icons[5][28],"2fffffb8200000f03fffffe03fffffc0");
}

void DrawIcon(whichIcon, h, v)
	short whichIcon;
	short h;
	short v;
{
	Rect	srcRect, dstRect;
	BitMap	srcBits;

	srcBits.baseAddr = (Ptr) &icons[whichIcon][0];
	srcBits.rowBytes = 4;
	SetRect(&srcBits.bounds,0,0,32,32);
	srcRect = srcBits.bounds;
	dstRect = srcRect;
	OffsetRect(&dstRect,h,v);
	CopyBits(&srcBits,&qd.thePort->portBits,
		&srcRect,&dstRect,srcOr,(RgnHandle) 0);
}

#define	STYLES	30
#define FONTS	110

main()
{
    Rect	theRect;
    GrafPort	myGrafPort;
    PolyHandle  myPoly;
    RgnHandle   myRgn;
    short	i;

    InitGraf(&qd.thePort);
    InitFonts();

    OpenPort(&myGrafPort);

    theRect = qd.thePort->portRect;
    EraseRect(&theRect);

    MoveTo(140,14);
    DrawString("Look what you can draw with UNIX QuickDraw !");
    MoveTo(0,18);
    LineTo(639,18);
    MoveTo(0,20);
    LineTo(639,20);

    /* horizontal lines */

    MoveTo(5,174);
    LineTo(634,174);

    MoveTo(5,328);
    LineTo(634,328);

    /* vertical lines */
    MoveTo(210,25);
    LineTo(210,465);

    MoveTo(430,25);
    LineTo(430,465);

    /* text */

    MoveTo(80,54);
    DrawString("Text");

    TextFace(bold);
    MoveTo(STYLES,75);
    DrawString("Bold");

    TextFace(italic);
    MoveTo(STYLES,90);
    DrawString("Italic");

    TextFace(underline);
    MoveTo(STYLES,105);
    DrawString("Underline");

    TextFace(outline);
    MoveTo(STYLES,120);
    DrawString("Outline");

    TextFace(shadow);
    MoveTo(STYLES,135);
    DrawString("Shadow");

    TextFace(normal);
    TextFont(newYork);
    MoveTo(FONTS,80);
    DrawString("New York");

    TextFont(geneva);
    MoveTo(FONTS,100);
    DrawString("Geneva");

    TextFont(systemFont);
    MoveTo(FONTS,120);
    DrawString("Chicago");

    TextFont(monaco);
    MoveTo(FONTS,140);
    DrawString("Monaco");

    TextFont(systemFont);

    /* lines */

    MoveTo(280,54);
    DrawString("Lines");

    MoveTo(240,55); Line(160,40);

    PenSize(3,2);
    MoveTo(240,65); Line(160,40);

    PenSize(6,4);
    MoveTo(240,75); Line(160,40);

    PenSize(12,8);
    PenPat(qd.gray);
    MoveTo(240,91); Line(160,40);

    PenSize(15,10);
    PenPat(qd.dkGray);
    MoveTo(240,110); Line(160,40);
    PenNormal();

    /* rectangles */
    
    MoveTo(480,54);
    DrawString("Rectangles");

    SetRect (&theRect,450,60,510,90);
    FrameRect(&theRect);

    OffsetRect(&theRect,15,15);
    PenSize(3,2);
    EraseRect(&theRect);
    FrameRect(&theRect);

    OffsetRect(&theRect,15,15);
    PaintRect(&theRect);

    OffsetRect(&theRect,15,15);
    PenNormal();
    FillRect(&theRect, qd.gray);
    FrameRect(&theRect);

    OffsetRect(&theRect,15,15);
    FillRect(&theRect, qd.dkGray);
    FrameRect(&theRect);

    /* RoundRect */

    MoveTo(70,208);
    DrawString("RoundRects");

    SetRect (&theRect,30,210,90,240);
    FrameRoundRect(&theRect,30,20);

    OffsetRect(&theRect,15,15);
    PenSize(3,2);
    EraseRoundRect(&theRect,30,20);
    FrameRoundRect(&theRect,30,20);

    OffsetRect(&theRect,15,15);
    PaintRoundRect(&theRect,30,20);

    OffsetRect(&theRect,15,15);
    PenNormal();
    FillRoundRect(&theRect,30,20,qd.gray);
    FrameRoundRect(&theRect,30,20);

    OffsetRect(&theRect,15,15);
    FillRoundRect(&theRect,30,20,qd.dkGray);
    FrameRoundRect(&theRect,30,20);

    /* icons */
    InitIcons ();

    MoveTo(280,208);
    DrawString("Icons");

    DrawIcon(0,249,220);
    DrawIcon(1,304,220);
    DrawIcon(2,359,220);
    DrawIcon(3,249,270);
    DrawIcon(4,304,270);
    DrawIcon(5,359,270);
    
    /* arcs */

    MoveTo(480,208);
    DrawString("Arcs");

    SetRect(&theRect,450,213,580,303);
    FillArc(&theRect,135,65,qd.ltGray);
    FillArc(&theRect,200,130,qd.dkGray);
    FillArc(&theRect,330,75,qd.gray);
    OffsetRect(&theRect,20,0);
    PaintArc(&theRect,45,90);
    FillArc(&theRect,45,90,qd.black);

    /* polygons */

    MoveTo(80,362);
    DrawString("Polygons");

    myPoly = OpenPoly();
    MoveTo(30,390);
    LineTo(30,380);
    LineTo(50,365);
    LineTo(90,365);
    LineTo(80,380);
    LineTo(95,390);
    LineTo(30,390);
    ClosePoly();

    FramePoly(myPoly);

    OffsetPoly(myPoly,15,15);
    PenSize(3,2);
    ErasePoly(myPoly);
    FramePoly(myPoly);

    OffsetPoly(myPoly,15,15);
    PaintPoly(myPoly);

    OffsetPoly(myPoly,15,15);
    PenNormal ();
    FillPoly(myPoly,qd.gray);
    FramePoly(myPoly);

    OffsetPoly(myPoly,15,15);
    FillPoly(myPoly,qd.dkGray);
    FramePoly(myPoly);

    /* regions */
    MoveTo(280,362);
    DrawString("Clipping");
    myRgn = NewRgn ();
    OpenRgn ();
    ShowPen ();
    SetRect(&theRect,220,370,420,450);
    FrameRoundRect(&theRect,24,16);

    MoveTo(235,435);
    LineTo(285,385);
    LineTo(335,435);
    LineTo(235,435);

    SetRect(&theRect,325,377,405,425);
    FrameOval(&theRect);
    HidePen ();
    CloseRgn(myRgn);

    SetClip(myRgn);
    for (i=0;i<6;i++) {
		MoveTo(220,380+12*i);
		DrawString("Arbitrary clipping region");
    }

    ClipRect(&(qd.thePort->portRect));
    DisposeRgn(myRgn);

    /* ovals */

    MoveTo(480,362);
    DrawString("Ovals");

    SetRect (&theRect,450,364,510,394);
    FrameOval(&theRect);

    OffsetRect(&theRect,15,15);
    PenSize(3,2);
    EraseOval(&theRect);
    FrameOval(&theRect);

    OffsetRect(&theRect,15,15);
    PaintOval(&theRect);

    OffsetRect(&theRect,15,15);
    PenNormal();
    FillOval(&theRect, qd.gray);
    FrameOval(&theRect);

    OffsetRect(&theRect,15,15);
    FillOval(&theRect, qd.dkGray);
    FrameOval(&theRect);

    ClosePort(&myGrafPort);
}
