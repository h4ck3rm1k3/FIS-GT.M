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

#include "mdef.h"

#include "gtm_string.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gbldirnam.h"
#include "hashdef.h"
#include "iosize.h"
#include "probe.h"
#include "dpgbldir.h"
#ifdef UNIX
#include "gtmio.h"
#elif defined(VMS)
#include <fab.h>
#else
#error unsupported platform
#endif
#include "dpgbldir_sysops.h"

GBLREF gd_addr		*gd_header;

/*+
Function:       ZGBLDIR

		This function searches the list of global directory names for
		the specified names.  If not found, it adds the new name to the
		list and calls GD_LOAD.  A pointer to the global directory
		structure is returned, and the name entry is pointed at it.
		The global directory pointer is then returned to the caller.

Syntax:         gd_addr *zgbldir(mval *v)

Prototype:      ?

Return:         *gd_addr -- a pointer to the global directory structure

Arguments:      mval *v	-- an mval that contains the name of the global
		directory to be accessed.  The name may require translation.

Side Effects:   NONE

Notes:          NONE
-*/

typedef struct gdr_name_struct
{
	mstr		name;
	struct gdr_name	*link;
	gd_addr		*gd_ptr;
}gdr_name;

static	gdr_name	*gdr_name_head;

gd_addr *zgbldir(mval *v)
{
	gd_addr		*gd_ptr;
	gdr_name	*name;
	mstr		temp_mstr, *tran_name;

	for (name = gdr_name_head;  name;  name = (gdr_name *)name->link)
		if (v->str.len == name->name.len && !memcmp(v->str.addr, name->name.addr, v->str.len))
			return name->gd_ptr;
	if (!v->str.len)
	{
		temp_mstr.addr = DEF_GDR;
		temp_mstr.len = sizeof(DEF_GDR) - 1;
		tran_name = get_name(&temp_mstr);
	} else
		tran_name = get_name(&v->str);
	gd_ptr = gd_load(tran_name);
	name = (gdr_name *)malloc(sizeof(gdr_name));
	if (name->name.len = v->str.len)	/* Note embedded assignment */
	{
		name->name.addr = (char *)malloc(v->str.len);
		memcpy(name->name.addr, v->str.addr, v->str.len);
	}
	/* free up memory allocated for mstr and its addr field in get_name */
	assert(tran_name->len);
	free(tran_name->addr);
	free(tran_name);

	if (gdr_name_head)
		name->link = (struct gdr_name *)gdr_name_head;
	else
		name->link = 0;
	gdr_name_head = name;
	gdr_name_head->gd_ptr = gd_ptr;
	return gd_ptr;
}

/*+
Function:       GD_LOAD

Syntax:		gd_addr *gd_load(mstr *gd_name)

		Open a global directory file and verify that it is a valid GD.
		Determine if it has already been opened.  If not, setup and
		initialize the GT.M structures used to access the GD based on
		the information in the file, enter in the linked list of global
		directories and return a pointer to it.  If already opened, return
		a pointer to it.

Prototype:      ?

Return:         gd_addr *	(all errors are signalled)

Arguments:	gd_name is the name of the file to be opened

Side Effects:   None
Notes:          A) While checking may be done earlier for duplicate names,
		unique identification of files can require OS specific
		operations useable only after the file is open, so checks
		must be done within this function for duplicate files.
-*/

LITREF char gde_labels[GDE_LABEL_NUM][GDE_LABEL_SIZE];

static gd_addr	*gd_addr_head;

gd_addr *gd_load(mstr *v)
{
	void		*file_ptr;	/* This is a temporary structure as the file open and manipulations are currently stubs */
	header_struct	*header, temp_head;
	gd_addr		*table, *gd_addr_ptr;
	gd_binding	*map, *map_top;
	gd_region	*reg, *reg_top;
	uint4		t_offset, size;
	short		i;
	error_def(ERR_GDINVALID);

	file_ptr = open_gd_file(v);

	for (gd_addr_ptr = gd_addr_head;  gd_addr_ptr;  gd_addr_ptr = gd_addr_ptr->link)
	{	/* if already open then return old structure */
		if (comp_gd_addr(gd_addr_ptr, file_ptr))
		{
			close_gd_file(file_ptr);
			return gd_addr_ptr;
		}
	}
	file_read(file_ptr, sizeof(header_struct), (uchar_ptr_t)&temp_head, 1);		/* Read in header and verify is valid GD */
	for (i = 0;  i < GDE_LABEL_NUM;  i++)
		if (!memcmp(temp_head.label, gde_labels[i], GDE_LABEL_SIZE - 1))
			break;
	if (GDE_LABEL_NUM == i)
	{
		close_gd_file(file_ptr);
		rts_error(VARLSTCNT(4) ERR_GDINVALID, 2, v->len, v->addr);
	}
	size = LEGAL_IO_SIZE(temp_head.filesize);
	header = (header_struct *)malloc(size);
	file_read(file_ptr, size, (uchar_ptr_t)header, 1);			/* Read in body of file */
	table = (gd_addr *)((char *)header + sizeof(header_struct));
	table->local_locks = (struct gd_region_struct *)((unsigned)table->local_locks + (unsigned)table);
	table->maps = (struct gd_binding_struct *)((unsigned)table->maps + (unsigned)table);
	table->regions = (struct gd_region_struct *)((unsigned)table->regions + (unsigned)table);
	table->segments = (struct gd_segment_struct *)((unsigned)table->segments + (unsigned)table);
	table->end = (unsigned)table->end + (unsigned)table;
	for (map = table->maps, map_top = map + table->n_maps;  map < map_top;  map++)
	{
		t_offset = map->reg.offset;
		map->reg.addr = (gd_region *)((char *)table + t_offset);
	}

	for (reg = table->regions, reg_top = reg + table->n_regions;  reg < reg_top;  reg++)
	{
		t_offset = reg->dyn.offset;
		reg->dyn.addr = (gd_segment *)((char *)table + t_offset);
	}
	table->link = gd_addr_head;
	gd_addr_head = table;
	fill_gd_addr_id(gd_addr_head, file_ptr);
	close_gd_file(file_ptr);
	table->tab_ptr = (htab_desc *)malloc(sizeof(htab_desc));
	ht_init(table->tab_ptr, 0);
	return table;
}

/*+
Function:       GET_NEXT_GDR

		This function returns the next entry in the list of open
		global directories.  If the input parameter is zero, the
		first entry is returned, otherwise the next entry in the
		list is returned.  If the input parameter is not a member
		of the list, then zero will be returned.

Syntax:         gd_addr *get_next_gdr(gd_addr *prev)

Prototype:      ?

Return:         *gd_addr -- a pointer to the global directory structure

Arguments:      The previous global directory accessed;

Side Effects:   NONE

Notes:          NONE
-*/

gd_addr *get_next_gdr(gd_addr *prev)
{
	gd_addr	*ptr;

	if (!prev)
		return gd_addr_head;

	for (ptr = gd_addr_head;  ptr && ptr != prev;  ptr = ptr->link)
		if (!GTM_PROBE(sizeof(*ptr), ptr, READ)) /* Called from secshr, have to check access to memory */
			return NULL;
	if (ptr && GTM_PROBE(sizeof(*ptr), ptr, READ))
		return ptr->link;
	return NULL;
}

/* Maintain list of regions for GTCM_SERVER */

void cm_add_gdr_ptr(gd_region *greg)
{
	gd_addr	*ga;

	ga = (gd_addr *)malloc(sizeof(gd_addr));
	ga->end = 0;	/* signifies a GT.CM gd_addr */
	ga->regions = greg;
	ga->n_regions = 1;
	ga->link = gd_addr_head;
	gd_addr_head = ga;
	return;
}

void cm_del_gdr_ptr(gd_region *greg)
{
	gd_addr	*ga1, *ga2;

	for (ga1 = ga2 = gd_addr_head;  ga1;  ga1 = ga1->link)
	{
		if (ga1->regions == greg)
		{
			if (ga1 == gd_addr_head)
				gd_addr_head = ga1->link;
			else
				ga2->link = ga1->link;
			free(ga1);
			break;
		}
		ga2 = ga1;
	}
	return;
}

boolean_t get_first_gdr_name(gd_addr *current_gd_header, mstr *log_nam)
{
	gdr_name	*name;

	for (name = gdr_name_head;  name;  name = (gdr_name *)name->link)
	{
		if (name->gd_ptr == current_gd_header)
		{
			*log_nam = name->name;
			return (TRUE);
		}
	}
	return FALSE;
}

void gd_rundown(void)		/* Wipe out the global directory structures */
{
	gd_addr		*gda_cur, *gda_next;
	gdr_name	*gdn_cur, *gdn_next;

	for (gda_cur = gd_addr_head;  NULL != gda_cur; gda_cur = gda_next)
	{
		gda_next = gda_cur->link;
		if (gda_cur->end)
		{
			gd_ht_kill(gda_cur->tab_ptr, TRUE);
			free(gda_cur->id);		/* free up gd_id malloced in gd_load()/fill_gd_addr_id() */
			free(gda_cur->tab_ptr);		/* free up hashtable malloced in gd_load() */
			free((char *)gda_cur - sizeof(header_struct));	/* free up global directory itself */
		} else
			free(gda_cur);	/* GT.CM gd_addr and hence header_struct wasn't malloced in cm_add_gdr_ptr */
	}
	gd_header = gd_addr_head = (gd_addr *)NULL;
	for (gdn_cur = gdr_name_head; NULL != gdn_cur; gdn_cur = gdn_next)
	{
		gdn_next = (gdr_name *)gdn_cur->link;
		if (gdn_cur->name.len)
			free(gdn_cur->name.addr);
		free(gdn_cur);
	}
	gdr_name_head = (gdr_name *)NULL;
}

void gd_ht_kill(htab_desc *table, boolean_t contents)		/* wipe out the hash table corresponding to a gld */
{
	ht_entry	*ent, *top;

	if (contents)
	{
		for (ent = table->base, top = ent + table->size; ent < top; ent++)
		{
			if (ent->ptr)
			{
				if (NULL != ((gv_namehead *)ent->ptr)->alt_hist)
					free(((gv_namehead *)ent->ptr)->alt_hist);	/* can be NULL for GT.CM client */
				free(ent->ptr);
			}
		}
	}
	free((char *)table->base);
	/* We don't do a free(table) in this generic routine because it is called both by GT.M and GT.CM
	 * and GT.CM retains the table for reuse while GT.M doesn't. GT.M fgncal_rundown() takes care of
	 * this by freeing it up explicitly (after a call to ht_kill) in gd_rundown() [dpgbldir.c]
	 */
	return;
}
