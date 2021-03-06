#################################################################
#								#
#	Copyright 2001 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

#	PAGE	,132
	.title	opp_hardret.s

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"
	.INCLUDE	"g_msf.si"

	.sbttl	opp_hardret
#	PAGE	+
	.DATA
.extern	dollar_truth
.extern	frame_pointer

	.text
.extern	op_hardret

# PUBLIC	opp_hardret
ENTRY opp_hardret
	putframe
	addl	$4,%esp
	call	op_hardret
	getframe
	ret
# opp_hardret ENDP

# END
