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
change:	;implement the verb: CHANGE
NAME
	i '$d(nams(NAME)) zm gdeerr("OBJNOTFND"):"Name":NAME
	i '$d(lquals("REGION")) zm gdeerr("QUALREQD"):"Region"
	s update=1
	s nams(NAME)=lquals("REGION")
	q
REGION
	i '$d(regs(REGION)) zm gdeerr("OBJNOTFND"):"Region":REGION
	i $d(lquals("JOURNAL")),lquals("JOURNAL"),'regs(REGION,"JOURNAL"),'$d(lquals("BEFORE_IMAGE")) zm gdeerr("QUALREQD"):"Before_image"
	i '$$RQUALS^GDEVERIF(.lquals) zm gdeerr("OBJNOTCHG"):"region":REGION
	s update=1,s=""
	f  s s=$o(lquals(s)) q:'$l(s)  s regs(REGION,s)=lquals(s) i s="ALLOCATION" s regs(REGION,"EXTENSION")=lquals(s)
	q
SEGMENT
	i '$d(segs(SEGMENT)) zm gdeerr("OBJNOTFND"):"Segment":SEGMENT
	s am=$s($d(lquals("ACCESS_METHOD")):lquals("ACCESS_METHOD"),1:segs(SEGMENT,"ACCESS_METHOD"))
	i '$$SQUALS^GDEVERIF(am,.lquals) zm gdeerr("OBJNOTCHG"):"segment":SEGMENT
	s update=1,s=""
	s segs(SEGMENT,"ACCESS_METHOD")=am
	f  s s=$o(lquals(s)) q:'$l(s)  s segs(SEGMENT,s)=lquals(s)
	f  s s=$o(tmpseg(am,s)) q:'$l(s)  d
	. i '$l(tmpseg(am,s)) s segs(SEGMENT,s)="" q
	. i '$l(segs(SEGMENT,s)) s segs(SEGMENT,s)=tmpseg(am,s)
	q
