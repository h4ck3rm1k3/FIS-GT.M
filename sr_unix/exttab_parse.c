/****************************************************************
 *								*
 *	Copyright 2001, 2004 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"

/* gcc/LinuxIA32 needs stdio before varargs until stdio.h removed from error.h */
/* gcc/Linux390 needs varargs before stdio */
#ifdef EARLY_VARARGS
#include <varargs.h>
#endif
#include "gtm_stdio.h"
#include <errno.h>
#include "gtm_stdlib.h"
#ifndef EARLY_VARARGS
#include <varargs.h>
#endif

#include "gtm_ctype.h"
#include "gtm_stat.h"
#include "copy.h"
#include "gtmxc_types.h"
#include "rtnhdr.h"
#include "fgncal.h"
#include "gtmci.h"
#include "eintr_wrappers.h"
#include "error.h"

#define	CR			0x0A		/* Carriage return */
#define	NUM_TABS_FOR_GTMERRSTR	2
#define	MAX_SRC_LINE		1024
#define	MAX_NAM_LEN 		32
#define	MAXIMUM_STARS		2
#define	SPACE_BLOCK_SIZE	1024
#define	TABLEN			8
#define	POINTER_SIZE		6

static 	int	ext_source_line_num;
static  int	ext_source_line_len;
static  int	ext_source_column;
static  char	ext_source_line[MAX_SRC_LINE];
static  char	*ext_table_file_name;
static  bool	star_found;

static  void 	ext_stx_error();
static 	int 	scan_array_bound(char **b,int curr_type);
const static int parm_space_needed[] =
{
	0,
	0,
	sizeof(void *),
	sizeof(xc_long_t),
	sizeof(xc_ulong_t),
	sizeof(xc_float_t),
	sizeof(xc_double_t),
	sizeof(xc_long_t *) + sizeof(xc_long_t),
	sizeof(xc_ulong_t *) + sizeof(xc_ulong_t),
	sizeof(xc_string_t *) + sizeof(xc_string_t),
	sizeof(xc_float_t *) + sizeof(xc_float_t),
	sizeof(xc_char_t *),
	sizeof(xc_char_t **) + sizeof(xc_char_t *),
	sizeof(xc_double_t *) + sizeof(xc_double_t),
	sizeof(xc_pointertofunc_t),
	sizeof(xc_pointertofunc_t *) + sizeof(xc_pointertofunc_t)
};

/* manage local get_memory'ed space (the space is never returned) */
static void	*get_memory(int n)
{
	void		*x;
	static void	*heap_base = 0;
	static int	space_remaining = 0;

	if (0 == n)
		return NULL;
	/* round up to 64 bits bytes */
	n = ROUND_UP(n, 8);
	if (space_remaining < n)
	{
		if (SPACE_BLOCK_SIZE < n)
			return (void *)malloc(n);
		else
		{
			heap_base = (void *)malloc(SPACE_BLOCK_SIZE);
			space_remaining = SPACE_BLOCK_SIZE;
		}
	}
	assert(space_remaining >= n);
	x = heap_base;
	heap_base = (char *)heap_base + n;
	space_remaining -= n;
	return x;
}

/* skip white space */
static char	*scan_space(char *c)
{
	for ( ;ISSPACE(*c); c++, ext_source_column++)
		;
	return c;
}

/* if this is an identifier (alphameric and underscore), then
   return the address after the end of the identifier.
   Otherwise, return zero
  */
static char	*scan_ident(char *c)
{
	char	*b;

	b = c;
	for (  ;ISALNUM(*b)  ||  ('_' == *b); b++, ext_source_column++)
		;
	return (b == c) ? 0 : b;
}

static char	*scan_labelref(char *c)
{
	char	*b = c;
	for ( ;(ISALNUM(*b) || '_' == *b || '^' == *b); b++,ext_source_column++)
		;
	return (b == c) ? 0 : b;
}

static enum xc_types	scan_keyword (char **c)
{
	const static struct
	{
		char		nam[MAX_NAM_LEN];
		enum xc_types	typ[MAXIMUM_STARS + 1]; /* one entry for each level of indirection eg [1] is type* */
	} xctab[] =
	{
	/*	typename		type		type *		type **			*/

		{"void",		xc_void,	xc_notfound,	xc_notfound		},

		{"gtm_long_t",		xc_long,	xc_long_star,	xc_notfound		},
		{"xc_long_t",		xc_long,	xc_long_star,	xc_notfound		},
		{"long",		xc_long,	xc_long_star,	xc_notfound		},

		{"gtm_ulong_t",		xc_ulong,	xc_ulong_star,	xc_notfound		},
		{"xc_ulong_t",		xc_ulong,	xc_ulong_star,	xc_notfound		},
		{"ulong",		xc_ulong,	xc_ulong_star,	xc_notfound		},

		{"gtm_status_t",	xc_status,	xc_notfound,	xc_notfound		},
		{"xc_status_t",		xc_status,	xc_notfound,	xc_notfound		},

		{"gtm_char_t",		xc_notfound,	xc_char_star,	xc_char_starstar	},
		{"xc_char_t",		xc_notfound,	xc_char_star,	xc_char_starstar	},
		{"char",		xc_notfound,	xc_char_star,	xc_char_starstar	},

		{"gtm_string_t",	xc_notfound,	xc_string_star,	xc_notfound		},
		{"xc_string_t",		xc_notfound,	xc_string_star,	xc_notfound		},
		{"string",		xc_notfound,	xc_string_star,	xc_notfound		},

		{"gtm_float_t",		xc_float,	xc_float_star,	xc_notfound		},
		{"xc_float_t",		xc_float,	xc_float_star,	xc_notfound		},
		{"float",		xc_float,	xc_float_star,	xc_notfound		},

		{"gtm_double_t",	xc_double,	xc_double_star,	xc_notfound		},
		{"xc_double_t",		xc_double,	xc_double_star,	xc_notfound		},
		{"double",		xc_double,	xc_double_star,	xc_notfound		},

		{"gtm_pointertofunc_t", xc_pointertofunc, xc_pointertofunc_star, xc_notfound	},
		{"xc_pointertofunc_t", 	xc_pointertofunc, xc_pointertofunc_star, xc_notfound	}
	};

	char	*b = *c;
	char	*d;
	int	len, i, star_count;

	b = scan_space(b);
	d = scan_ident(b);
	if (!d)
		return xc_notfound;
	len = d - b;
	for (i = 0 ;  i < sizeof(xctab) / sizeof(xctab[0]) ;  i++)
	{
		if ((0 == memcmp(xctab[i].nam, b, len)) && ('\0' ==  xctab[i].nam[len]))
		{
			/* got name */
			/* scan stars */
			for (star_count = 0;(MAXIMUM_STARS >= star_count);star_count++, d++)
			{
				d = scan_space(d);
				if ('*' != *d)
					break;
				star_found = TRUE;
			}
			assert(star_count <= MAXIMUM_STARS);
			*c = scan_space(d);
			return xctab[i].typ[star_count];
		}
	}
	return xc_notfound;
}

static 	int scan_array_bound(char **b,int curr_type)
{
	char 		number[MAX_DIGITS_IN_INT];
	char		*c;
	char 		*line;
	int 		index;
	error_def	(ERR_ZCCSQRBR);
	error_def	(ERR_ZCPREALLNUMEX);

	const static int default_pre_alloc_value[] =
	{
		0, /* Unknown Type */
		0, /* void */
		0, /* status */
		0, /* long */
		0, /* unsigned long */
		0, /* float */
		0, /* double */
		1, /* Pointer to long */
		1, /* Pointer to unsigned long */
		1, /* Pointer to string */
		1, /* Pointer to float */
		100, /* Pointer to char */
		1, /* Pointer to pointer of char */
		1, /* Pointer to double */
		1, /* Pointer to function */
		1  /* Pointer to pointer to function */
	};

	c = *b;
	/* already found '[' */
	for (index=0, c++; ']' != *c;c++)
	{
		if ('\0' != *c)
		{
			if (ISDIGIT((int)*c))
				number[index++] = *c;
			else
				ext_stx_error(ERR_ZCPREALLNUMEX, ext_table_file_name);
		} else
			ext_stx_error(ERR_ZCCSQRBR, ext_table_file_name);
	}
	c++; /* skip ']' */
	*b = c;
	if (0 == index)
		return default_pre_alloc_value[curr_type];
	number[index] = 0;
	return ATOI(number);
}

static char	*read_table (char *b,int l,FILE *f)
{
	char	*t;

	ext_source_column = 0;
	FGETS(b, l, f, t);
	if (NULL == t)
	{
		if (0 != ferror(f))
		{
			int fclose_res;
			FCLOSE(f, fclose_res);
			/* Expand error message */
			rts_error(VARLSTCNT(1) errno);
		} else
			assert(feof(f) != 0);
	} else
	{
		/* unfortunately, fgets does not strip the NL, we will do it for it */
		for (ext_source_line_len = 0; *t ;  t++)
		{
			ext_source_line[ext_source_line_len++] = *t;
			if (CR == *t)
			{
				*t = 0;
				ext_source_line_num += 1;
				break;
			}
		}
	}
	return t;
}

/*  utility routine to store static mstr's which will remain for the duration of the process
 *   These mstr's will have a trailing null attached so that UNIX system routines can operate
 *   on them directly as const char *'s
 */
static void	put_mstr (mstr *src,mstr *dst)
{
	char	*cp;
	int	n;

	dst->len = n = src->len;
	assert(n >= 0);
	dst->addr = cp = (char *)get_memory(n + 1);
	if (0 < n)
		memcpy(dst->addr, src->addr, dst->len);
	dst->addr[n] = 0;
	return;
}

/* utility to convert an array of bool's to a bit mask */
static uint4	array_to_mask (bool ar[MAXIMUM_PARAMETERS],int n)
{
	uint4	mask = 0;
	int	i;

	for (i = n - 1;0 <= i;i--)
	{
		assert((ar[i] & ~1) == 0);
		mask = (mask << 1) | ar[i];
	}
	return mask;
}

/* Note: need condition handler to clean-up allocated structures and close intput file in the event of an error */
struct extcall_package_list	*exttab_parse (mval *package)
{
	int		parameter_alloc_values[MAXIMUM_PARAMETERS], parameter_count, ret_pre_alloc_val, i, fclose_res;
	bool		is_input[MAXIMUM_PARAMETERS], is_output[MAXIMUM_PARAMETERS], got_status;
	mstr		callnam, rtnnam;
	void_ptr_t	pakhandle;
	enum xc_types	ret_tok, parameter_types[MAXIMUM_PARAMETERS], pr;
	char		str_buffer[MAX_TABLINE_LEN], *tbp, *end;
	FILE		*ext_table_file_handle;
	struct extcall_package_list	*pak;
	struct extcall_entry_list	*entry_ptr;

	error_def(ERR_ZCRTENOTF);
	error_def(ERR_ZCALLTABLE);
	error_def(ERR_ZCUSRRTN);
	error_def(ERR_ZCCTENV);
	error_def(ERR_ZCCTOPN);
	error_def(ERR_ZCCTNULLF);
	error_def(ERR_ZCUNAVAIL);
	error_def(ERR_ZCENTNAME);
	error_def(ERR_ZCCOLON);
	error_def(ERR_ZCRTNTYP);
	error_def(ERR_ZCRCALLNAME);
	error_def(ERR_ZCUNTYPE);
	error_def(ERR_ZCMLTSTATUS);
	error_def(ERR_ZCRPARMNAME);
	error_def(ERR_ZCPREALLVALPAR);
	error_def(ERR_ZCPREALLVALINV);
	/* First, construct package name environment variable */
	memcpy(str_buffer, PACKAGE_ENV_PREFIX, sizeof(PACKAGE_ENV_PREFIX));
	tbp = &str_buffer[sizeof(PACKAGE_ENV_PREFIX) - 1];
	if (package->str.len)
	{
		/* guaranteed by compiler */
		assert(package->str.len < MAX_NAME_LENGTH - sizeof(PACKAGE_ENV_PREFIX) - 1);
		*tbp++ = '_';
		memcpy(tbp, package->str.addr, package->str.len);
		tbp += package->str.len;
	}
	*tbp = 0;
	/* Now we have the environment name, lookup file name */
	ext_table_file_name = GETENV(str_buffer);
	if (NULL == ext_table_file_name)
	{
		/* Environment variable for the package not found */
		rts_error(VARLSTCNT(4) ERR_ZCCTENV, 2, LEN_AND_STR(str_buffer));
	}
	ext_table_file_handle = Fopen(ext_table_file_name, "r");
	if (NULL == ext_table_file_handle)
	{
		/* Package's external call table could not be found */
		rts_error(VARLSTCNT(4) ERR_ZCCTOPN, 2, LEN_AND_STR(ext_table_file_name));
	}
	ext_source_line_num = 0;
	/* pick-up name of shareable library */
	tbp = read_table(LIT_AND_LEN(str_buffer), ext_table_file_handle);
	if (NULL == tbp)
	{
		/* External call table is a null file */
		rts_error(VARLSTCNT(4) ERR_ZCCTNULLF, 2, package->str.len, package->str.addr);
	}
	pakhandle = fgn_getpak(str_buffer, INFO);
	if (NULL == pakhandle)
	{
		/* Unable to obtain handle to the shared library */
		rts_error(VARLSTCNT(4) ERR_ZCUNAVAIL, 2, package->str.len, package->str.addr);
	}
	pak = get_memory(sizeof(*pak));
	pak->first_entry = 0;
	put_mstr(&package->str, &pak->package_name);
	pak->package_handle = pakhandle;
	/* At this point, we have a valid package, pointed to by pak */
#ifdef DEBUG_EXTCALL
	FPRINTF(stderr, "GT.M external call opened package name: %s\n", pak->package_name.addr);
#endif
	for (;;)
	{
		star_found = FALSE;
		tbp = read_table(LIT_AND_LEN(str_buffer), ext_table_file_handle);
		if (NULL == tbp)
			break;
		tbp = scan_space(str_buffer);
		/* empty line? */
		if (!*tbp)
			continue;
		/* No, must be entryref */
		end = scan_ident(tbp);
		if (!end)
			ext_stx_error(ERR_ZCENTNAME, ext_table_file_name);
		if ('^' == *end)
		{
			end++;
			end = scan_ident(end);
			if (!end)
				ext_stx_error(ERR_ZCENTNAME, ext_table_file_name);
		}
		rtnnam.addr = tbp;
		rtnnam.len = end - tbp;
		tbp = scan_space(end);
		if (':' != *tbp++)
			ext_stx_error(ERR_ZCCOLON, ext_table_file_name);
		/* get return type */
		ret_tok = scan_keyword(&tbp);
		/* check for legal return type */
		switch (ret_tok)
		{
		case xc_status:
		case xc_void:
		case xc_long:
		case xc_ulong:
		case xc_char_star:
		case xc_float_star:
		case xc_string_star:
		case xc_long_star:
		case xc_ulong_star:
		case xc_double_star:
		case xc_char_starstar:
		case xc_pointertofunc:
		case xc_pointertofunc_star:
			break;
		default:
			ext_stx_error(ERR_ZCRTNTYP, ext_table_file_name);
		}
		got_status = (ret_tok == xc_status);
		/*  get call name */
		if ('[' == *tbp)
		{
			if (star_found)
				ret_pre_alloc_val = scan_array_bound(&tbp,ret_tok);
			else
				ext_stx_error(ERR_ZCPREALLVALPAR, ext_table_file_name);
			/* We should allow the pre-allocated value upto to the maximum string size (MAX_STRLEN) plus 1 for the
			 * extra terminating NULL. Negative values would have been caught by scan_array_bound() above */
			if (ret_pre_alloc_val > MAX_STRLEN + 1)
				ext_stx_error(ERR_ZCPREALLVALINV, ext_table_file_name);
		} else
			ret_pre_alloc_val = -1;
		end = scan_ident(tbp);
		if (!end)
			ext_stx_error(ERR_ZCRCALLNAME, ext_table_file_name);
		callnam.addr = tbp;
		callnam.len = end - tbp;
		tbp = scan_space(end);
		tbp = scan_space(tbp);
		for (parameter_count = 0;(MAXIMUM_PARAMETERS > parameter_count) && (')' != *tbp); parameter_count++)
		{
			star_found = FALSE;
			/* must have comma if this is not the first parameter, otherwise '(' */
			if (((0 == parameter_count)?'(':',') != *tbp++)
				ext_stx_error(ERR_ZCRPARMNAME, ext_table_file_name);
			tbp = scan_space(tbp);
			/* special case: () is ok */
			if ((0 == parameter_count) && (*tbp == ')'))
				break;
			/* looking for an I, an O or an IO */
			is_input[parameter_count] = is_output[parameter_count] = FALSE;
			if ('I' == *tbp)
			{
				is_input[parameter_count] = TRUE;
				tbp++;
			}
			if ('O' == *tbp)
			{
				is_output[parameter_count] = TRUE;
				tbp++;
			}
			if (((FALSE == is_input[parameter_count]) && (FALSE == is_output[parameter_count]))
				||(':' != *tbp++))
			{
				ext_stx_error(ERR_ZCRCALLNAME, ext_table_file_name);
			}
			/* scanned colon--now get type */
			pr = scan_keyword(&tbp);
			if (xc_notfound == pr)
				ext_stx_error(ERR_ZCUNTYPE, ext_table_file_name);
			if (xc_status == pr)
			{
				/* Only one type "status" allowed per call */
				if (got_status)
					ext_stx_error(ERR_ZCMLTSTATUS, ext_table_file_name);
				else
					got_status = TRUE;
			}
			parameter_types[parameter_count] = pr;
			if ('[' == *tbp)
			{
				if (star_found && !is_input[parameter_count])
					parameter_alloc_values[parameter_count] = scan_array_bound(&tbp, pr);
				else
					ext_stx_error(ERR_ZCPREALLVALPAR, ext_table_file_name);
				/* We should allow the pre-allocated value upto to the maximum string size (MAX_STRLEN) plus 1 for
				 * the extra terminating NULL. Negative values would have been caught by scan_array_bound() above */
				if (parameter_alloc_values[parameter_count] > MAX_STRLEN + 1)
					ext_stx_error(ERR_ZCPREALLVALINV, ext_table_file_name);
			} else
				parameter_alloc_values[parameter_count] = -1;
			tbp = scan_space(tbp);
		}
		entry_ptr = get_memory(sizeof(*entry_ptr));
		entry_ptr->next_entry = pak->first_entry;
		pak->first_entry = entry_ptr;
		entry_ptr->return_type = ret_tok;
		entry_ptr->ret_pre_alloc_val = ret_pre_alloc_val;
		entry_ptr->argcnt = parameter_count;
		entry_ptr->input_mask = array_to_mask(is_input, parameter_count);
		entry_ptr->output_mask = array_to_mask(is_output, parameter_count);
		entry_ptr->parms = get_memory(parameter_count * sizeof(entry_ptr->parms[0]));
		entry_ptr->param_pre_alloc_size = get_memory(parameter_count * sizeof(int));
		entry_ptr->parmblk_size = sizeof(void *) * parameter_count + sizeof(int);
		for (i = 0 ;  i < parameter_count ;  i++)
		{
			entry_ptr->parms[i] = parameter_types[i];
			entry_ptr->parmblk_size += parm_space_needed[parameter_types[i]];
			entry_ptr->param_pre_alloc_size[i] = parameter_alloc_values[i];
		}
		put_mstr(&rtnnam, &entry_ptr->entry_name);
		put_mstr(&callnam, &entry_ptr->call_name);

		/* the reason for passing INFO severity is that PROFILE has several routines listed in
		 * the external call table that are not in the shared library. PROFILE folks would
		 * rather see info/warning messages for such routines at shared library open time,
		 * than error out. These unimplemented routines, they say were not being called from
		 * the application and wouldn't cause any application failures. If we fail to open
		 * the shared libary, or we fail to locate a routine that is called from the
		 * application, we issue rts_error message (in extab_parse.c) */
		entry_ptr->fcn = fgn_getrtn(pak->package_handle, &entry_ptr->call_name, INFO);
#ifdef DEBUG_EXTCALL
		FPRINTF(stderr, "   package entry point: %s, address: %x\n", entry_ptr->entry_name.addr, entry_ptr->fcn);
#endif
	}
	FCLOSE(ext_table_file_handle, fclose_res);
	return pak;
}

callin_entry_list*	citab_parse (void)
{
	int			parameter_count, i, fclose_res;
	uint4			inp_mask, out_mask, mask;
	mstr			labref, callnam;
	enum xc_types		ret_tok, parameter_types[MAXIMUM_PARAMETERS], pr;
	char			str_buffer[MAX_TABLINE_LEN], *tbp, *end;
	FILE			*ext_table_file_handle;
	callin_entry_list	*entry_ptr, *save_entry_ptr = 0;

	error_def(ERR_CITABENV);
	error_def(ERR_CITABOPN);
	error_def(ERR_CIENTNAME);
	error_def(ERR_COLON);
	error_def(ERR_CIRTNTYP);
	error_def(ERR_CIRCALLNAME);
	error_def(ERR_CIDIRECTIVE);
	error_def(ERR_CIRPARMNAME);
	error_def(ERR_CIPARTYPE);
	error_def(ERR_CIUNTYPE);
	error_def(ERR_SYSCALL);
	error_def(ERR_CIMAXPARAM);

	ext_table_file_name = GETENV(CALLIN_ENV_NAME);
	if (!ext_table_file_name) /* environment variable not set */
		rts_error(VARLSTCNT(4) ERR_CITABENV, 2, LEN_AND_STR(CALLIN_ENV_NAME));

	ext_table_file_handle = Fopen(ext_table_file_name, "r");
	if (!ext_table_file_handle) /* call-in table not found */
		rts_error(VARLSTCNT(11) ERR_CITABOPN, 2, LEN_AND_STR(ext_table_file_name),
					ERR_SYSCALL, 5, LEN_AND_LIT("fopen"), CALLFROM, errno);
	ext_source_line_num = 0;
	while (read_table(LIT_AND_LEN(str_buffer), ext_table_file_handle))
	{
		if (!*(tbp = scan_space(str_buffer)))
			continue;
		if (!(end = scan_ident(tbp)))
			ext_stx_error(ERR_CIRCALLNAME, ext_table_file_name);
		callnam.addr = tbp;
		callnam.len = end - tbp;
		tbp = scan_space(end);
		if (':' != *tbp++)
			ext_stx_error(ERR_COLON, ext_table_file_name);
		ret_tok = scan_keyword(&tbp); /* return type */
		switch (ret_tok) /* return type valid ? */
		{
		case xc_void:
		case xc_char_star:
		case xc_long_star:
		case xc_ulong_star:
		case xc_float_star:
		case xc_double_star:
		case xc_string_star:
			break;
		default:
			ext_stx_error(ERR_CIRTNTYP, ext_table_file_name);
		}
		labref.addr = tbp;
		if ((end = scan_labelref(tbp)))
			labref.len = end - tbp;
		else
			ext_stx_error(ERR_CIENTNAME, ext_table_file_name);
		tbp = scan_space(end);
		inp_mask = out_mask = 0;
		for (parameter_count = 0; (*tbp && ')' != *tbp); parameter_count++)
		{
			if (MAXIMUM_PARAMETERS <= parameter_count)
				ext_stx_error(ERR_CIMAXPARAM, ext_table_file_name);
			/* must have comma if this is not the first parameter, otherwise '(' */
			if (((0 == parameter_count)?'(':',') != *tbp++)
				ext_stx_error(ERR_CIRPARMNAME, ext_table_file_name);
			tbp = scan_space(tbp);
			if ((0 == parameter_count) && (*tbp == ')')) /* special case () */
				break;
			/* looking for an I, a O or an IO */
			mask = (1 << parameter_count);
			inp_mask |= ('I' == *tbp) ? (tbp++, mask) : 0;
			out_mask |= ('O' == *tbp) ? (tbp++, mask) : 0;
			if ((!(inp_mask & mask) && !(out_mask & mask)) || (':' != *tbp++))
				ext_stx_error(ERR_CIDIRECTIVE, ext_table_file_name);
			switch ((pr = scan_keyword(&tbp))) /* valid param type? */
			{
			case xc_long:
			case xc_ulong:
			case xc_float:
			case xc_double:
				if (out_mask & mask)
					ext_stx_error(ERR_CIPARTYPE, ext_table_file_name);
				/* fall-thru */
			case xc_char_star:
			case xc_long_star:
			case xc_ulong_star:
			case xc_float_star:
			case xc_double_star:
			case xc_string_star:
				break;
			default:
				ext_stx_error(ERR_CIUNTYPE, ext_table_file_name);
			}
			parameter_types[parameter_count] = pr;
			tbp = scan_space(tbp);
		}
		if (!*tbp)
			ext_stx_error(ERR_CIRPARMNAME, ext_table_file_name);
		entry_ptr = get_memory(sizeof(callin_entry_list));
		entry_ptr->next_entry = save_entry_ptr;
		save_entry_ptr = entry_ptr;
		entry_ptr->return_type = ret_tok;
		entry_ptr->argcnt = parameter_count;
		entry_ptr->input_mask = inp_mask;
		entry_ptr->output_mask = out_mask;
		entry_ptr->parms = get_memory(parameter_count * sizeof(entry_ptr->parms[0]));
		for (i = 0 ;  i < parameter_count ;  i++)
			entry_ptr->parms[i] = parameter_types[i];
		put_mstr(&labref, &entry_ptr->label_ref);
		put_mstr(&callnam, &entry_ptr->call_name);
	}
	FCLOSE(ext_table_file_handle, fclose_res);
	return entry_ptr;
}

static void ext_stx_error(va_alist)
va_dcl
{
	va_list	args, sav_arg;
	char	*ext_table_name;
	char	buf[MAX_SRC_LINE], *b;
	int	num_tabs, num_spaces;
	int	in_error;
	error_def(ERR_EXTSRCLIN);
	error_def(ERR_EXTSRCLOC);

	va_start(args);
	in_error = va_arg(args, int);
	ext_table_name = va_arg(args, char *);

	num_tabs = ext_source_column/TABLEN;
	num_spaces = ext_source_column%TABLEN;

	b = &buf[0];
	memset(buf, '\t', num_tabs+2);
	b += num_tabs+NUM_TABS_FOR_GTMERRSTR;
	memset(b, ' ', num_spaces);
	b += num_spaces;
	memcpy(b, "^-----", POINTER_SIZE);
	b += POINTER_SIZE;
	*b = 0;

	dec_err(VARLSTCNT(6) ERR_EXTSRCLIN, 4, ext_source_line_len, ext_source_line, b - &buf[0], &buf[0]);
	dec_err(VARLSTCNT(6) ERR_EXTSRCLOC, 4, ext_source_column, ext_source_line_num, LEN_AND_STR(ext_table_name));
	rts_error(VARLSTCNT(1) in_error);
}
