/*	@(#)usertab.c	UniPlus V.2.1.4	(UniSoft 1.1)*/
/*
 * (C) Copyright 1984 UniSoft Corp. Version V.0.0
 *
 *	To make a new tabNNN file for nroff you must do 3 things
 *
 *	1) Take a copy of this file as XXXXX.c and modify it to
 *	   describe your output device. Change the table values
 *	   and change extname to give the name of the tabNNN file.
 *
 *	2) Build userterms using 'cc -o userterms userterms.o XXXXX.c'
 *
 *	3) Run userterms which will create your tabNNNN file
 *
 *		Note: this file as is will produce the 5520 tabs file
 */

char *extname="tab5520";

#define INCH 240

struct {
	int b[9];
	char *c[16];
	char *d[256-32];
	char *z
		} extdata[] = {
/*bset*/	0,
/*breset*/	0177420,
/*Hor*/		INCH/10,
/*Vert*/	INCH/6,
/*Newline*/	INCH/6,
/*Char*/	INCH/10,
/*Em*/		INCH/10,
/*Halfline*/	INCH/12,
/*Adj*/		INCH/10,
/*twinit*/	"\0334\033>\033]W\033]L",
/*twrest*/	"\0334\033>\033]W\033]L",
/*twnl*/	"\015\n",
/*hlr*/		"\033]S\033XA\033]W",
/*hlf*/		"\033]S\033ZA\033]W",
/*flr*/		"\0339",
/*bdon*/	"",
/*bdoff*/	"",
/*iton*/	"",
/*itoff*/	"",
/*ploton*/	"",
/*plotoff*/	"",
/*up*/		"",
/*down*/	"",
/*right*/	"",
/*left*/	"",
/*codetab*/
/*
	For Prestige Elite /Prestige Renown thimble #803-020004-392-A
	The letters in the shift-out set are used for capital Greeks.
	All greeks are represented by their troff symbols.
	Tom Strong	16 March 1982
*/
"\001 ",	/*space*/
"\001!",	/*!*/
"\001\"",	/*"*/
"\001#",	/*#*/
"\001$",	/*$*/
"\001%",	/*%*/
"\001&",	/*&*/
"\001'",	/*' close*/
"\001(",	/*(*/
"\001)",	/*)*/
"\001\033]Q\033ZA*\033XA\033]W",	/***/
"\001+",	/*+*/
"\001,",	/*,*/
"\001-",	/*- hyphen*/
"\001.",	/*.*/
"\001/",	/*/*/
"\2010",	/*0*/
"\2011",	/*1*/
"\2012",	/*2*/
"\2013",	/*3*/
"\2014",	/*4*/
"\2015",	/*5*/
"\2016",	/*6*/
"\2017",	/*7*/
"\2018",	/*8*/
"\2019",	/*9*/
"\001:",	/*:*/
"\001;",	/*;*/
"\001<",	/*<*/
"\001=",	/*=*/
"\001>",	/*>*/
"\001?",	/*?*/
"\001@",	/*@*/
"\201A",	/*A*/
"\201B",	/*B*/
"\201C",	/*C*/
"\201D",	/*D*/
"\201E",	/*E*/
"\201F",	/*F*/
"\201G",	/*G*/
"\201H",	/*H*/
"\201I",	/*I*/
"\201J",	/*J*/
"\201K",	/*K*/
"\201L",	/*L*/
"\201M",	/*M*/
"\201N",	/*N*/
"\201O",	/*O*/
"\201P",	/*P*/
"\201Q",	/*Q*/
"\201R",	/*R*/
"\201S",	/*S*/
"\201T",	/*T*/
"\201U",	/*U*/
"\201V",	/*V*/
"\201W",	/*W*/
"\201X",	/*X*/
"\201Y",	/*Y*/
"\201Z",	/*Z*/
"\001[",	/*[*/
"\001\\",	/*\*/
"\001]",	/*]*/
"\001^",	/*^*/
"\001_",	/*_ dash*/
"\001`",	/*` open*/
"\201a",	/*a*/
"\201b",	/*b*/
"\201c",	/*c*/
"\201d",	/*d*/
"\201e",	/*e*/
"\201f",	/*f*/
"\201g",	/*g*/
"\201h",	/*h*/
"\201i",	/*i*/
"\201j",	/*j*/
"\201k",	/*k*/
"\201l",	/*l*/
"\201m",	/*m*/
"\201n",	/*n*/
"\201o",	/*o*/
"\201p",	/*p*/
"\201q",	/*q*/
"\201r",	/*r*/
"\201s",	/*s*/
"\201t",	/*t*/
"\201u",	/*u*/
"\201v",	/*v*/
"\201w",	/*w*/
"\201x",	/*x*/
"\201y",	/*y*/
"\201z",	/*z*/
"\001{",	/*{*/
"\001|",	/*|*/
"\001}",	/*}*/
"\001~",	/*~*/
"\000\0",	/*nar sp*/
"\001-",	/*hyphen*/
"\001O\b+",	/*bullet*/
"\002\[]",	/*square*/
"\001-",	/*3/4 em*/
"\001_",	/*rule*/
"\0031/4",	/*1/4*/
"\0031/2",	/*1/2*/
"\0033/4",	/*3/4*/
"\001-",	/*minus*/
"\202fi",	/*fi*/
"\202fl",	/*fl*/
"\202ff",	/*ff*/
"\203ffi",	/*ffi*/
"\203ffl",	/*ffl*/
"\001\033]S\033XAo\033ZA\033]W",	/*degree*/
"\001-\b|",	/*dagger*/
"\001 ",	/*section*/
"\001'",	/*foot mark*/
"\001'",	/*acute accent*/
"\001`",	/*grave accent*/
"\001_",	/*underrule*/
"\001/",	/*slash (longer)*/
"\000\0",	/*half narrow space*/
"\001 ",	/*unpaddable space*/
"\201a",	/*alpha*/
"\201b",	/*beta*/
"\201g",	/*gamma*/
"\201d",	/*delta*/
"\201e",	/*epsilon*/
"\201z",	/*zeta*/
"\201y",	/*eta*/
"\201h",	/*theta*/
"\201i",	/*iota*/
"\201k",	/*kappa*/
"\201l",	/*lambda*/
"\201m",	/*mu*/
"\201n",	/*nu*/
"\201c",	/*xi*/
"\201o",	/*omicron*/
"\201p",	/*pi*/
"\201r",	/*rho*/
"\201s",	/*sigma*/
"\201t",	/*tau*/
"\201v",	/*upsilon*/
"\201f",	/*phi*/
"\201x",	/*chi*/
"\201q",	/*psi*/
"\201w",	/*omega*/
"\201\016G\017",	/*Gamma*/
"\201\016D\017",	/*Delta*/
"\201\016H\017",	/*Theta*/
"\201\016L\017",	/*Lambda*/
"\201\016C\017",	/*Xi*/
"\201\016P\017",	/*Pi*/
"\201\016S\017",	/*Sigma*/
"\201\016T\017",	/*Tau*/
"\201\016U\017",	/*Upsilon*/
"\201\016F\017",	/*Phi*/
"\201\016Q\017",	/*Psi*/
"\201\016W\017",	/*Omega*/
"\001 ",	/*square root*/
"\000",		/*terminal sigma*/
"\001 ",	/*root en*/
"\001>\b_",	/*>=*/
"\001<\b_",	/*<=*/
"\001=\b_",	/*identically equal*/
"\001-",	/*equation minus*/
"\001=\b~",	/*approx =*/
"\001~",	/*approximates*/
"\001=\b/",	/*not equal*/
"\002->",	/*right arrow*/
"\002<-",	/*left arrow*/
"\001|\b^",	/*up arrow*/
"\001 ",	/*down arrow*/
"\001=",	/*equation equal*/
"\001x",	/*multiply*/
"\001/",	/*divide*/
"\001+\b_",	/*plus-minus*/
"\001U",	/*cup (union)*/
"\001 ",	/*cap (intersection)*/
"\001 ",	/*subset of*/
"\001 ",	/*superset of*/
"\001 ",	/*improper subset*/
"\001 ",	/*improper superset*/
"\002oo",	/*infinity*/
"\001 ",	/*partial derivative*/
"\001 ",	/*gradient*/
"\001 ",	/*not*/
"\001 ",	/*integral sign*/
"\002oc",	/*proportional to*/
"\001 ",	/*empty set*/
"\001 ",	/*member of*/
"\001+",	/*equation plus*/
"\001\033]S\033XAr\033ZA\033]W",	/*registered*/
"\001\033]S\033XAc\033ZA\033]W",	/*copyright*/
"\001|",	/*box rule */
"\001c\b/",	/*cent sign*/
"\001=\b|",	/*dbl dagger*/
"\002*>",	/*right hand*/
"\002<*",	/*left hand*/
"\001*",	/*math * */
"\001 ",	/*bell system sign*/
"\001|",	/*or (was star)*/
"\001O",	/*circle*/
"\001|",	/*left top (of big curly)*/
"\001|",	/*left bottom*/
"\001|",	/*right top*/
"\001|",	/*right bot*/
"\001|",	/*left center of big curly bracket*/
"\001|",	/*right center of big curly bracket*/
"\001|",	/*bold vertical*/
"\001|",	/*left floor (left bot of big sq bract)*/
"\001|",	/*right floor (rb of ")*/
"\001|",	/*left ceiling (lt of ")*/
"\001|"};	/*right ceiling (rt of ")*/
