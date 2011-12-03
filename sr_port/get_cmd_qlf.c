/****************************************************************
 *								*
 *	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"

#include "cmd_qlf.h"
#include "cli.h"

GBLREF command_qualifier glb_cmd_qlf;
GBLDEF list_params lst_param;

void get_cmd_qlf(command_qualifier *qualif)
{
	static readonly char upper[] = "UPPER";
	static readonly char lower[] = "LOWER";
	mstr *s;
	int temp_int;
	unsigned short len;
	unsigned char inbuf[255];

	qualif->qlf = glb_cmd_qlf.qlf;
	qualif->object_file.mvtype = qualif->list_file.mvtype = qualif->ceprep_file.mvtype = 0;
	if (cli_present("OBJECT") == CLI_PRESENT)
	{
		qualif->qlf |= CQ_OBJECT;
		qualif->object_file.mvtype = MV_STR;
		s = &qualif->object_file.str;
		len = s->len;
		if (cli_get_str("OBJECT", s->addr, &len) == FALSE)
		{
			s->len = 0;
			if (glb_cmd_qlf.object_file.mvtype == MV_STR  &&  glb_cmd_qlf.object_file.str.len > 0)
			{
				s->len = glb_cmd_qlf.object_file.str.len;
				memcpy(s->addr, glb_cmd_qlf.object_file.str.addr,
					s->len);
			}
		} else
			s->len = len;
	}
	else if (cli_negated("OBJECT") == TRUE)
		qualif->qlf &= ~CQ_OBJECT;


	if (cli_present("CROSS_REFERENCE") == CLI_PRESENT)
		qualif->qlf |= CQ_CROSS_REFERENCE;
	else if (cli_negated("CROSS_REFERENCE") == TRUE)
		qualif->qlf &= ~CQ_CROSS_REFERENCE;

	if (cli_negated("IGNORE") == TRUE)
		qualif->qlf &= ~CQ_IGNORE;
	else
		qualif->qlf |= CQ_IGNORE;

	if (cli_present("DEBUG") == CLI_PRESENT)
		qualif->qlf |= CQ_DEBUG;
	else if (cli_negated("DEBUG") == TRUE)
		qualif->qlf &= ~CQ_DEBUG;

	if (cli_negated("LINE_ENTRY") == TRUE)
		qualif->qlf &= ~CQ_LINE_ENTRY;

	if (cli_negated("INLINE_LITERALS") == TRUE)
		qualif->qlf &= ~CQ_INLINE_LITERALS;

#ifdef DEBUG
	if (cli_present("MACHINE_CODE") == CLI_PRESENT)
		qualif->qlf |= CQ_MACHINE_CODE;
	else if (cli_negated("MACHINE_CODE") == TRUE)
		qualif->qlf &= ~CQ_MACHINE_CODE;
#else
	qualif->qlf &= ~CQ_MACHINE_CODE;
#endif

	if (cli_negated("WARNINGS") == TRUE)
		qualif->qlf &= ~CQ_WARNINGS;
	else
		qualif->qlf |= CQ_WARNINGS;


	if (cli_negated("LIST") == TRUE)
		qualif->qlf &= (~CQ_LIST & ~CQ_MACHINE_CODE);
	else if (cli_present("LIST") == CLI_PRESENT)
	{	qualif->qlf |= CQ_LIST;
		qualif->list_file.mvtype = MV_STR;
		s = &qualif->list_file.str;
		len = s->len;
		if (cli_get_str("LIST", s->addr, &len) == FALSE)
		{
			s->len = 0;
			if (glb_cmd_qlf.list_file.mvtype == MV_STR  &&  glb_cmd_qlf.list_file.str.len > 0)
			{	s->len = glb_cmd_qlf.list_file.str.len;
				memcpy(s->addr, glb_cmd_qlf.list_file.str.addr,
					s->len);
			}
		} else
			s->len = len;
	}
	else if (!(qualif->qlf & CQ_LIST))
	{	qualif->qlf &= ~CQ_MACHINE_CODE;
	}
	if (cli_get_int("LENGTH",&temp_int) == FALSE)
		temp_int = 66;
	lst_param.lines_per_page = temp_int;
	if (cli_get_int("SPACE",&temp_int) == FALSE || temp_int <= 0 || temp_int >= lst_param.lines_per_page)
		temp_int = 1;
	lst_param.space = temp_int;

	if (cli_present("LABELS") == CLI_PRESENT)
	{
		len = sizeof(inbuf);
		if (cli_get_str("LABELS", (char *)&inbuf[0], &len))
		{
			if (len == sizeof(upper) - 1)
			{
				if (!memcmp(upper, &inbuf[0], len))
					qualif->qlf &= ~CQ_LOWER_LABELS;
				else if (!memcmp(lower, &inbuf[0], len))
					qualif->qlf |= CQ_LOWER_LABELS;
			}
		}
	}

	if (cli_present("CE_PREPROCESS") == CLI_PRESENT)
        {
		qualif->qlf |= CQ_CE_PREPROCESS;
		qualif->ceprep_file.mvtype = MV_STR;
		s = &qualif->ceprep_file.str;
		len = s->len;
		if (cli_get_str("CE_PREPROCESS", s->addr, &len) == FALSE)
		{
			s->len = 0;
			if (glb_cmd_qlf.ceprep_file.mvtype == MV_STR  &&  glb_cmd_qlf.ceprep_file.str.len > 0)
			{
				s->len = glb_cmd_qlf.ceprep_file.str.len;
				memcpy(s->addr, glb_cmd_qlf.ceprep_file.str.addr, s->len);
			}
		} else
			s->len = len;
	}
	else if (cli_negated("CE_PREPROCESS") == TRUE)
		qualif->qlf &= ~CQ_CE_PREPROCESS;
}
