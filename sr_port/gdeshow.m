;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
;	Copyright 2001 Sanchez Computer Associates, Inc.	;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
show:	;implement the verb: SHOW
ALL
	d TEMPLATE,ALLNAME,ALLREGIO,ALLSEGME,MAP
	q
NAME
	i '$d(nams(NAME)) zm gdeerr("OBJNOTFND"):"Name":NAME q
	d n2
	i log s BOL="!" u @uself w BOL d n2 w ! u @useio s BOL=""
	q
n2:	d namehd w !,BOL,?x(1),NAME,?x(2),nams(NAME)
	q
ALLNAME
	d SHOWNAM^GDEMAP
	d n1
	i log s BOL="!" u @uself w BOL d n1 w ! u @useio s BOL=""
	q
n1:	d namehd s s="#"
	f  s s=$o(nams(s)) q:'$l(s)  w !,BOL,?x(1),s,?x(2),nams(s)
	q
REGION
	i '$d(regs(REGION)) zm gdeerr("OBJNOTFND"):"Region":REGION q
	d r2
	i log s BOL="!" u @uself w BOL d r2 w ! u @useio s BOL=""
	q
r2:	d regionhd s s=REGION d onereg
	i regs(s,"JOURNAL") d jnlhd d onejnl
	q
ALLREGIO
	d r1
	i log s BOL="!" u @uself w BOL d r1 w ! u @useio s BOL=""
	q
r1:	d regionhd s jnl=0,s=""
	f  s s=$o(regs(s)) q:'$l(s)  d onereg i regs(s,"JOURNAL") s jnl=1
	i jnl d jnlhd s s="" f  s s=$o(regs(s)) q:'$l(s)  i regs(s,"JOURNAL") d onejnl
	q
onereg:
	w !,BOL,?x(1),s,?x(2),regs(s,"DYNAMIC_SEGMENT"),?x(3),$j(regs(s,"COLLATION_DEFAULT"),4)
	w ?x(4),$j(regs(s,"RECORD_SIZE"),5),?x(5),$j(regs(s,"KEY_SIZE"),5)
	w ?x(6),$s(regs(s,"NULL_SUBSCRIPTS"):"Y",1:"N"),?x(7),$s(regs(s,"JOURNAL"):"Y",1:"N")
	q
onejnl:
	w !,BOL,?x(1),s,?x(2),$s($l(regs(s,"FILE_NAME")):regs(s,"FILE_NAME"),1:"<based on DB file-spec>")
	i $x'<(x(3)-1) w !,BOL
	w ?x(3),$s(regs(s,"BEFORE_IMAGE"):"Y",1:"N"),?x(4),$j(regs(s,"BUFFER_SIZE"),5)
	w ?x(5),$j(regs(s,"ALLOCATION"),10),?x(6),$j(regs(s,"EXTENSION"),5)
	;w ?x(7),$s(regs(s,"STOP_ENABLED"):"Y",1:"N")
	w !,BOL
	q
SEGMENT
	i '$d(segs(SEGMENT)) zm gdeerr("OBJNOTFND"):"Segment":SEGMENT q
	d s2
	i log s BOL="!" u @uself w BOL d s2 w ! u @useio s BOL=""
	q
s2:	d seghd s s=SEGMENT s am=segs(s,"ACCESS_METHOD") d oneseg
	q
ALLSEGME
	d s1
	i log s BOL="!" u @uself w BOL d s1 w ! u @useio s BOL=""
	q
s1:	d seghd s s=""
	f  s s=$o(segs(s)) q:'$l(s)  s am=segs(s,"ACCESS_METHOD") d oneseg
	q
oneseg:
	w !,BOL,?x(1),s,?x(2),segs(s,"FILE_NAME")
	i $x'<(x(3)-1) w !,BOL
	w ?x(3),segs(s,"ACCESS_METHOD")
	i am="USER" q
	w ?x(4),$s(segs(s,"FILE_TYPE")="DYNAMIC":"DYN",1:"STA")
	w ?x(5),$j(segs(s,"BLOCK_SIZE"),5),?x(6),$j(segs(s,"ALLOCATION"),10),?x(7),$j(segs(s,"EXTENSION_COUNT"),5)
	d @am
	q
BG	w ?x(8),"GLOB=",$j(segs(s,"GLOBAL_BUFFER_COUNT"),4)
	w !,BOL,?x(8),"LOCK=",$j(segs(s,"LOCK_SPACE"),4)
	w !,BOL,?x(8),"RES =",$j(segs(s,"RESERVED_BYTES"),4)
	q
MM	w ?x(8),$s(segs(s,"DEFER"):"DEFER",1:"NODEFER")
	w !,BOL,?x(8),"LOCK=",$j(segs(s,"LOCK_SPACE"),4)
	w !,BOL,?x(8),"RES =",$j(segs(s,"RESERVED_BYTES"),4)
	q
MAP
	n map
	i '$d(mapreg) n mapreg s mapreg=""
	e  i '$d(regs(mapreg)) zm gdeerr("OBJNOTFND"):"Region":mapreg q
	d SHOWMAKE^GDEMAP
	d m1
	i log s BOL="!" u @uself w BOL d m1 w ! u @useio s BOL=""
	q
m1:	n l1,s1,s2
	d maphd
	s s1=$o(map("$"))
	i s1'="%" s map("%")=map("$"),s1="%"
	f  s s2=s1,s1=$o(map(s2)) q:'$l(s1)  d onemap(s1,s2)
	d onemap("...",s2)
	i $d(nams("#")) s s2="LOCAL LOCKS",map(s2)=nams("#") d onemap("",s2) k map(s2)
	q
onemap:(s1,s2)
	i $l(mapreg),mapreg'=map(s2) q
	s l1=$l(s1)
	i $l(s2)=l1,$e(s1,l1)=0,$e(s2,l1)=")",$e(s1,1,l1-1)=$e(s2,1,l1-1) q
	w !,BOL,?x(1),$tr(s2,")","0"),?x(2),$tr(s1,")","0"),?x(3),map(s2)
	i '$d(regs(map(s2),"DYNAMIC_SEGMENT")) w ?x(4),"NONE",?x(5),"NONE" q
	s j=regs(map(s2),"DYNAMIC_SEGMENT") w ?x(4),j
	i '$d(segs(j,"ACCESS_METHOD")) w ?x(5),"NONE"
	e  s s=segs(j,"FILE_NAME") i $l(s)<(79-x(5)) w ?x(5),s
	e  w !,BOL,?$s(71-$l(s)>1:79-$l(s),1:1),s
	q
TEMPLATE
	d t1
	i log s BOL="!" u @uself w BOL d t1 w ! u @useio s BOL=""
	q
t1:	d tmpreghd
	w !,BOL,?x(1),"<default>",?x(3),$j(tmpreg("COLLATION_DEFAULT"),4)
	w ?x(4),$j(tmpreg("RECORD_SIZE"),5),?x(5),$j(tmpreg("KEY_SIZE"),5)
	w ?x(6),$s(tmpreg("NULL_SUBSCRIPTS"):"Y",1:"N")
	w ?x(7),$s(tmpreg("JOURNAL"):"Y",1:"N")
	i tmpreg("JOURNAL") d tmpjnlhd,tmpjnlbd
	d tmpseghd
	w !,BOL,?x(1),"<default>",?x(2),$s(tmpacc="BG":"  *",1:""),?x(3),"BG"
	w ?x(4),$s(tmpseg("BG","FILE_TYPE")="DYNAMIC":"DYN",1:"STA"),?x(5),$j(tmpseg("BG","BLOCK_SIZE"),5)
	w ?x(6),$j(tmpseg("BG","ALLOCATION"),10),?x(7),$j(tmpseg("BG","EXTENSION_COUNT"),5)
	w ?x(8),"GLOB =",$j(tmpseg("BG","GLOBAL_BUFFER_COUNT"),3)
	w !,BOL,?x(8),"LOCK =",$j(tmpseg("BG","LOCK_SPACE"),3)
	w !,BOL,?x(1),"<default>",?x(2),$s(tmpacc="MM":"   *",1:""),?x(3),"MM"
	w ?x(4),$s(tmpseg("MM","FILE_TYPE")="DYNAMIC":"DYN",1:"STA"),?x(5),$j(tmpseg("MM","BLOCK_SIZE"),5)
	w ?x(6),$j(tmpseg("MM","ALLOCATION"),10),?x(7),$j(tmpseg("MM","EXTENSION_COUNT"),5)
	w ?x(8),$s(tmpseg("MM","DEFER"):"DEFER",1:"NODEFER")
	w !,BOL,?x(8),"LOCK =",$j(tmpseg("MM","LOCK_SPACE"),3)
	q
tmpjnlbd:
	w !,BOL,?x(1),"<default>",?x(2),$s($l(tmpreg("FILE_NAME")):tmpreg("FILE_NAME"),1:"<based on DB file-spec>")
	i $x'<(x(3)-1) w !,BOL
	w ?x(3),$s(tmpreg("BEFORE_IMAGE"):"Y",1:"N"),?x(4),$j(tmpreg("BUFFER_SIZE"),5)
	w ?x(5),$j(tmpreg("ALLOCATION"),10),?x(6),$j(tmpreg("EXTENSION"),5)
	;w ?x(7),$s(tmpreg("STOP_ENABLED"):"Y",1:"N")
	w !,BOL
	q

;-----------------------------------------------------------------------------------------------------------------------------------

namehd:
	s x(0)=9,x(1)=1,x(2)=18
	w !,BOL,!,BOL,?x(0),"*** NAMES ***",!,BOL,?x(1),"Global",?x(2),"Region"
	w !,BOL,?x(1),$tr($j("",39)," ","-")
	q
regionhd:
	s x(0)=32,x(1)=1,x(2)=18,x(3)=44,x(4)=49,x(5)=55,x(6)=61,x(7)=67
	w !,BOL,!,BOL,?x(0),"*** REGIONS ***"
	w !,BOL,?x(2),"Dynamic",?x(3),$j("Def",4),?x(4),$j("Rec",5),?x(5),$j("Key",5),?x(6),"Null"
	w !,BOL,?x(1),"Region",?x(2),"Segment",?x(3),$j("Coll",4),?x(4),$j("Size",5),?x(5),$j("Size",5)
	w ?x(6),"Subs",?x(7),"Journaling"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
jnlhd:
	s x(0)=26,x(1)=1,x(2)=18,x(3)=44,x(4)=51,x(5)=57,x(6)=68,x(7)=74
	w !,BOL,!,BOL,?x(0),"*** JOURNALING INFORMATION ***"
	w !,BOL,?x(1),"Region",?x(2),"Jnl File (def ext: .mjl)"
	w ?x(3),"Before",?x(4),$j("Buff",5),?x(5),$j("Alloc",10),?x(6),"Exten" ;?x(7),"Stop"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
seghd:
	s x(0)=32,x(1)=1,x(2)=18,x(3)=38,x(4)=42,x(5)=46,x(6)=52,x(7)=63,x(8)=69
	w !,BOL,!,BOL,?x(0),"*** SEGMENTS ***"
	w !,BOL,?x(1),"Segment",?x(2),"File (def ext: .dat)",?x(3),"Acc",?x(4),"Typ",?x(5),"Block",?x(6),$j("Alloc",10)
	w ?x(7),"Exten",?x(8),"Options"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
maphd:
	s x="*** MAP"_$s($l(mapreg):" for region "_mapreg,1:"")_" ***"
	s x(0)=80-$l(x)*.5,x(1)=1,x(2)=14,x(3)=29,x(4)=46,x(5)=63
	w !,BOL,!,BOL,?x(0),x
	w !,BOL,?x(1),"  -  - Names -  -",?x(5),"File"
	w !,BOL,?x(1),"From",?x(2),"Up to",?x(3),"Region",?x(4),"Segment",?x(5),"(def ext: .dat)"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
tmpreghd:
	s x(0)=31,x(1)=1,x(2)=19,x(3)=44,x(4)=49,x(5)=55,x(6)=61,x(7)=67
	w !,BOL,!,BOL,?x(0),"*** TEMPLATES ***"
	w !,BOL,?x(3),$j("Def",4),?x(4),$j("Rec",5),?x(5),$j("Key",5),?x(6),"Null"
	w !,BOL,?x(1),"Region",?x(3),$j("Coll",4),?x(4),$j("Size",5),?x(5),$j("Size",5),?x(6),"Subs",?x(7),"Journaling"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
tmpjnlhd:
	s x(0)=26,x(1)=1,x(2)=18,x(3)=44,x(4)=51,x(5)=57,x(6)=68,x(7)=74
	w !,BOL,?x(2),"Jnl File (def ext: .mjl)"
	w ?x(3),"Before",?x(4),$j("Buff",5),?x(5),$j("Alloc",10),?x(6),"Exten" ;?x(7),"Stop"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
tmpseghd:
	s x(0)=32,x(1)=1,x(2)=18,x(3)=38,x(4)=42,x(5)=46,x(6)=52,x(7)=63,x(8)=69
	w !,BOL,!,BOL,?x(1),"Segment",?x(2),"Active",?x(3),"Acc",?x(4),"Typ",?x(5),"Block",?x(6),$j("Alloc",10)
	w ?x(7),"Exten",?x(8),"Options"
	w !,BOL,?x(1),$tr($j("",78)," ","-")
	q
