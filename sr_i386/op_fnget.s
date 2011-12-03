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
	.title	op_fnget.s

#	.386
#	.MODEL	FLAT, C

.include "linkage.si"
	.INCLUDE	"mval_def.si"

	.sbttl	op_fnget
#	PAGE	+
# ------------------------------------
# op_fnget.s
#
# Mumps $Get function
# ------------------------------------

#	edx - src. mval
#	eax - dest. mval

	.text
# PUBLIC	op_fnget
ENTRY op_fnget
	cmpl	$0,%edx
	je	l5			# if arg = 0, set type and len
	mv_if_notdefined %edx, l5

#	Copy the mval from [edx] to [eax].
	pushl	%edi
	pushl	%esi
	movl	$mval_byte_len,%ecx
	movl	%edx,%esi
	movl	%eax,%edi
	REP
	movsb
	popl	%esi
	popl	%edi
	ret

l5:	movb	$mval_m_str,mval_b_mvtype(%eax)		# string type
	movl	$0,mval_l_strlen(%eax)			# dest. str. len. = 0
	ret
# op_fnget ENDP

# END
