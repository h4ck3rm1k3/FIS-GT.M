void DO_MAXSTR_BUFF_FINI 
{ 
  if (maxstr_buff[maxstr_stack_level].addr) { 
    free(maxstr_buff[maxstr_stack_level].addr); 
    maxstr_buff[maxstr_stack_level].addr = NULL; 
  } 
  maxstr_buff[maxstr_stack_level].len = 0; maxstr_stack_level--; REVERT;
}

void DO_ALLOC_XFORM_BUFF(str) 
{
  if ((str)->len > max_lcl_coll_xform_bufsiz) { 
    if (0 == max_lcl_coll_xform_bufsiz) { 
      assert(NULL == lcl_coll_xform_buff); 
      max_lcl_coll_xform_bufsiz = MAX_STRBUFF_INIT; 
    } 
    else 
      { 
	assert(NULL != lcl_coll_xform_buff); 
	free(lcl_coll_xform_buff); 
      } 
    while ((str)->len > max_lcl_coll_xform_bufsiz) 
      max_lcl_coll_xform_bufsiz += max_lcl_coll_xform_bufsiz; 
    max_lcl_coll_xform_bufsiz = MIN(MAX_STRLEN, max_lcl_coll_xform_bufsiz); 
    lcl_coll_xform_buff = (char *)malloc(max_lcl_coll_xform_bufsiz); 
  }
}
