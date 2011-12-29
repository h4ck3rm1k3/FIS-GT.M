#include "errorsp.h"

int DO_ESTABLISH(void (*x)()) 
{ 
  CHTRACEPOINT; 
  ctxt++; 
  CHECKHIGHBOUND(ctxt); 
  ctxt->save_active_ch = active_ch; 
  ctxt->ch_active = FALSE; 
  active_ch = ctxt; 
  ctxt->ch = x; 
  if (setjmp(ctxt->jmp) == -1) 
    { 
      REVERT; 
      return 1;  
    } 
  return 0;
}


void DO_ESTABLISH_RET(x,ret) { 
  CHTRACEPOINT; 
  ctxt++; 
  CHECKHIGHBOUND(ctxt); 
  ctxt->save_active_ch = active_ch; 
  ctxt->ch_active = FALSE; 
  active_ch = ctxt; 
  ctxt->ch = x; 
  if (setjmp(ctxt->jmp) == -1) { 
    REVERT; 
    return ret; 
  } 
}

void  DO_MAXSTR_BUFF_INIT_RET { 
  ESTABLISH_RET(gtm_maxstr_ch, -1); 
  maxstr_stack_level++; 
  assert(maxstr_stack_level < MAXSTR_STACK_SIZE); 
  maxstr_buff[maxstr_stack_level].len = MAX_STRBUFF_INIT; 
  maxstr_buff[maxstr_stack_level].addr = NULL; 
}
