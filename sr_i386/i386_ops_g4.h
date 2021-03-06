/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* Opcodes for group 4 (one-byte) determined by bits 5,4,3 of ModR/M byte: */
I386_OP(INC,Eb,0)
I386_OP(DEC,Eb,1)
I386_OP(ILLEGAL_GROUP_4_OP,2,2)
I386_OP(ILLEGAL_GROUP_4_OP,3,3)
I386_OP(ILLEGAL_GROUP_4_OP,4,4)
I386_OP(ILLEGAL_GROUP_4_OP,5,5)
I386_OP(ILLEGAL_GROUP_4_OP,6,6)
I386_OP(ILLEGAL_GROUP_4_OP,7,7)
