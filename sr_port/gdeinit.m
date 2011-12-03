;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
;	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
gdeinit: ;set up local variables and arrays
GDEINIT

	s renpref=""
	s log=0,logfile="GDELOG.LOG",BOL=""
	s ZERO=$c(0),ONE=$c(1),TRUE=ONE,FALSE=ZERO,TAB=$c(9)
	s endian("VAX")=FALSE,glo("VMS")=1024
	s endian("AXP")=FALSE,glo("VMS")=1024,glo("OSF1")=1024
	s endian("x86")=FALSE,glo("SCO")=384,glo("UWIN")=1024,glo("Linux")=1024
	s endian("SEQUOIA_SERIES_400")=TRUE,glo("VAX")=1024
	s endian("HP-PA")=TRUE,glo("HP-UX")=1024
	s endian("SPARC")=TRUE,glo("SUN/OS_V4.x")=800,glo("Solaris")=1024
	s endian("MIPS")=TRUE,glo("A25")=1024
	s endian("B30")=TRUE,glo("NONSTOP-UX")=1024
	s endian("B32")=TRUE,glo("NONSTOP-UX")=1024
	s endian("MC-680x0")=TRUE,glo("SYS_V/68_R3V6")=1024,glo("TOPIX")=1024
	s endian("RS6000")=TRUE,glo("AIX")=1024
	s endian("S390")=TRUE,glo("OS390")=1024
	; The following line is for support of AIX V3.2.5 only and can (and should)
	; be removed (along with this comment) as soon as we drop support for
	; AIX V3.2.5.  This change is needed to correspond to the change in
	; release_name.h.  C9801-000344
	s glo("AIX325")=glo("AIX")
	s HEX(0)=1
	f x=1:1:8 s HEX(x)=HEX(x-1)*16 i x#2=0 s TWO(x*4)=HEX(x)
	s TWO(26)=TWO(24)*4
	s TWO(31)=TWO(32)*.5
	s lower="abcdefghijklmnopqrstuvwxyz",upper="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	s endian=endian($p($zver," ",4))
	s ver=$p($zver," ",3)
	s defglo=glo(ver)
	s comline=$zcmdline
	d UNIX:ver'="VMS"
	d VMS:ver="VMS"
	d syntabi
;
	s SIZEOF("am_offset")=308
	s SIZEOF("file_spec")=256
	s SIZEOF("gd_header")=16
	s SIZEOF("gd_contents")=44
	s SIZEOF("gd_map")=12
	s SIZEOF("gd_region")=316
	s SIZEOF("gd_segment")=320
	s SIZEOF("mident")=8
	s SIZEOF("rec_hdr")=3
	s SIZEOF("dsk_blk")=512
	s SIZEOF("max_str")=32767
	s MAXNAMLN=SIZEOF("mident"),MAXREGLN=16,MAXSEGLN=16
;
; tokens are used for error reporting only
	s tokens("TKIDENT")="identifier"
	s tokens("TKNUMLIT")="number"
	s tokens("TKEOL")="end-of-line"
	s tokens("""")="TKSTRLIT",tokens("TKSTRLIT")="string literal"
	s tokens("@")="TKAMPER",tokens("TKAMPER")="ampersand"
	s tokens("*")="TKASTER",tokens("TKASTER")="asterisk"
	s tokens(":")="TKCOLON",tokens("TKCOLON")="colon"
	s tokens(",")="TKCOMMA",tokens("TKCOMMA")="comma"
	s tokens("$")="TKDOLLAR",tokens("TKDOLLAR")="dollar sign"
	s tokens("=")="TKEQUAL",tokens("TKEQUAL")="equal sign"
	s tokens("<")="TKLANGLE",tokens("TKLANGLE")="left angle bracket"
	s tokens("[")="TKLBRACK",tokens("TKLBRACK")="left bracket"
	s tokens("(")="TKLPAREN",tokens("TKLPAREN")="left parenthesis"
	s tokens("-")="TKDASH",tokens("TKDASH")="dash"
	s tokens("%")="TKPCT",tokens("TKPCT")="percent sign"
	s tokens(".")="TKPERIOD",tokens("TKPERIOD")="period"
	s tokens(")")="TKRPAREN",tokens("TKRPAREN")="right parenthesis"
	s tokens("]")="TKRBRACK",tokens("TKRBRACK")="right bracket"
	s tokens(">")="TKRANGLE",tokens("TKRANGLE")="right angle bracket"
	s tokens(";")="TKSCOLON",tokens("TKSCOLON")="semicolon"
	s tokens("/")="TKSLASH",tokens("TKSLASH")="slash"
	s tokens("_")="TKUSCORE",tokens("TKUSCORE")="underscore"
	s tokens("!")="TKEXCLAM",tokens("TKEXCLAM")="exclamation point"
	s tokens("TKOTHER")="other"
; maximums and mimimums
; region
	s minreg("ALLOCATION")=10,minreg("BEFORE_IMAGE")=0,minreg("COLLATION_DEFAULT")=0,minreg("EXTENSION")=0
	s minreg("JOURNAL")=0,minreg("KEY_SIZE")=3,minreg("NULL_SUBSCRIPTS")=0; ,minreg("STOP_ENABLED")=0
	s minreg("RECORD_SIZE")=SIZEOF("rec_hdr")+4
	s maxreg("ALLOCATION")=TWO(24),maxreg("BEFORE_IMAGE")=1,maxreg("BUFFER_SIZE")=2000
	s maxreg("COLLATION_DEFAULT")=255,maxreg("EXTENSION")=HEX(4)-1
	s maxreg("JOURNAL")=1,maxreg("KEY_SIZE")=255,maxreg("NULL_SUBSCRIPTS")=1; ,maxreg("STOP_ENABLED")=1
	s maxreg("RECORD_SIZE")=SIZEOF("max_str")
; segments
; bg
	s minseg("BG","ALLOCATION")=10,minseg("BG","BLOCK_SIZE")=SIZEOF("dsk_blk"),minseg("BG","EXTENSION_COUNT")=0
	s minseg("BG","GLOBAL_BUFFER_COUNT")=64,minseg("BG","LOCK_SPACE")=10,minseg("BG","RESERVED_BYTES")=0
	s maxseg("BG","ALLOCATION")=TWO(26),(maxseg("BG","BLOCK_SIZE"),maxseg("BG","RESERVED_BYTES"))=HEX(4)-SIZEOF("dsk_blk")
	s maxseg("BG","EXTENSION_COUNT")=HEX(4)-1,maxseg("BG","GLOBAL_BUFFER_COUNT")=65536,maxseg("BG","LOCK_SPACE")=1000
; mm
	s minseg("MM","ALLOCATION")=10,minseg("MM","BLOCK_SIZE")=SIZEOF("dsk_blk"),minseg("MM","DEFER")=0
	s minseg("MM","LOCK_SPACE")=10,minseg("MM","EXTENSION_COUNT")=0,minseg("MM","RESERVED_BYTES")=0
	s maxseg("MM","ALLOCATION")=TWO(26),(maxseg("MM","BLOCK_SIZE"),maxseg("BG","RESERVED_BYTES"))=HEX(4)-SIZEOF("dsk_blk")
	s maxseg("MM","DEFER")=86400,maxseg("MM","LOCK_SPACE")=1000,maxseg("MM","EXTENSION_COUNT")=HEX(4)-1
	q

;-----------------------------------------------------------------------------------------------------------------------------------

; gde command language syntax table
syntabi:
	s syntab("ADD","NAME")=""
	s syntab("ADD","NAME","REGION")="REQUIRED"
	s syntab("ADD","NAME","REGION","TYPE")="TREGION"
	s syntab("ADD","REGION")=""
	s syntab("ADD","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("ADD","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("ADD","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("ADD","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("ADD","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("ADD","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("ADD","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	;s syntab("ADD","REGION","JOURNAL","STOP_ENABLED")="NEGATABLE"
	s syntab("ADD","REGION","KEY_SIZE")="REQUIRED"
	s syntab("ADD","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","REGION","NULL_SUBSCRIPTS")="NEGATABLE"
	s syntab("ADD","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("ADD","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT")=""
	s syntab("ADD","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("ADD","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("ADD","SEGMENT","ACCESS_METHOD","TYPE","VALUES")=accmeth
	s syntab("ADD","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("ADD","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","DEFER")="NEGATABLE"
	s syntab("ADD","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("ADD","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("ADD","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("ADD","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("ADD","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("ADD","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("ADD","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","NAME")=""
	s syntab("CHANGE","NAME","REGION")="REQUIRED"
	s syntab("CHANGE","NAME","REGION","TYPE")="TREGION"
	s syntab("CHANGE","REGION")=""
	s syntab("CHANGE","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("CHANGE","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("CHANGE","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("CHANGE","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("CHANGE","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("CHANGE","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	;s syntab("CHANGE","REGION","JOURNAL","STOP_ENABLED")="NEGATABLE"
	s syntab("CHANGE","REGION","KEY_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","NULL_SUBSCRIPTS")="NEGATABLE"
	s syntab("CHANGE","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT")=""
	s syntab("CHANGE","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("CHANGE","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("CHANGE","SEGMENT","ACCESS_METHOD","TYPE","VALUES")=accmeth
	s syntab("CHANGE","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("CHANGE","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","DEFER")="NEGATABLE"
	s syntab("CHANGE","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("CHANGE","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("CHANGE","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("CHANGE","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("CHANGE","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("CHANGE","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION")=""
	s syntab("TEMPLATE","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("TEMPLATE","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("TEMPLATE","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("TEMPLATE","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("TEMPLATE","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("TEMPLATE","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	;s syntab("TEMPLATE","REGION","JOURNAL","STOP_ENABLED")="NEGATABLE"
	s syntab("TEMPLATE","REGION","KEY_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","NULL_SUBSCRIPTS")="NEGATABLE"
	s syntab("TEMPLATE","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT")=""
	s syntab("TEMPLATE","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("TEMPLATE","SEGMENT","ACCESS_METHOD","TYPE","VALUES")=accmeth
	s syntab("TEMPLATE","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","DEFER")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("TEMPLATE","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("DELETE","NAME")=""
	s syntab("DELETE","REGION")=""
	s syntab("DELETE","SEGMENT")=""
	s syntab("EXIT")=""
	s syntab("HELP")=""
	s syntab("LOCKS","REGION")="REQUIRED"
	s syntab("LOCKS","REGION","TYPE")="TREGION"
	s syntab("LOG","OFF")=""
	s syntab("LOG","ON")="OPTIONAL"
	s syntab("LOG","ON","TYPE")="TFSPEC"
	s syntab("SETGD","FILE")="REQUIRED"
	s syntab("SETGD","FILE","TYPE")="TFSPEC"
	s syntab("SETGD","QUIT")=""
	s syntab("QUIT")=""
	s syntab("RENAME","NAME")=""
	s syntab("RENAME","REGION")=""
	s syntab("RENAME","SEGMENT")=""
	s syntab("SHOW")=""
	s syntab("SHOW","ALL")=""
	s syntab("SHOW","TEMPLATES")=""
	s syntab("SHOW","MAP")=""
	s syntab("SHOW","MAP","REGION")="REQUIRED"
	s syntab("SHOW","MAP","REGION","TYPE")="TREGION"
	s syntab("SHOW","NAME")=""
	s syntab("SHOW","REGION")=""
	s syntab("SHOW","SEGMENT")=""
	s syntab("SPAWN")=""
	s syntab("VERIFY","ALL")=""
	s syntab("VERIFY","MAP")=""
	s syntab("VERIFY","NAME")=""
	s syntab("VERIFY","REGION")=""
	s syntab("VERIFY","SEGMENT")=""
	s syntab("VERIFY","TEMPLATE")=""
	q
VMS
	s SIZEOF("blk_hdr")=7
	s endian=FALSE
	s hdrlab="GTCGBLDIR007"		; must be concurrently maintained in gbldirnam.h!!!
	s tfile="GTM$GBLDIR"
	s accmeth="\BG\MM\USER"
	s helpfile="GTM$HELP:GDE.HLB"
	s defdb="MUMPS"
	s defgld="MUMPS.GLD",defgldext=".GLD"
	s defreg="$DEFAULT"
	s defseg="$DEFAULT"
	s dbfilpar=".1AN.1""-"".1""_"".1"":"".1""$"".1""["".1""]"".1""<"".1"">"".1""."".1"";"""
	s filexfm="$tr(filespec,lower,upper)"
	s sep="TKSLASH"
	s nommbi=1		; this is used in gdeverif and should be removed along with the code when support is added
	q

UNIX:
	s SIZEOF("blk_hdr")=8
	s hdrlab="GTCGBDUNX003"		; must be concurrently maintained in gbldirnam.h!!!
	s tfile="$gtmgbldir"
	s accmeth="\BG\MM"
	s helpfile="$gtm_dist/gdehelp.gld"
	s defdb="mumps.dat"
	s defgld="mumps.gld",defgldext="*.gld"
	s defreg="DEFAULT"
	s defseg="DEFAULT"
	s dbfilpar="1E"
	s filexfm="filespec"
	s sep="TKDASH"
	s nommbi=0		; this is used in gdeverif and should be removed along with the code when support is added
	q
