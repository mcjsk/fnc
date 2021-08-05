/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/*
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).


  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)
  
  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.
  
  *****************************************************************************
  This file implements schema-related parts of the library.
*/
#include "fossil-scm/fossil-internal.h"
#include <assert.h>


char const * fsl_schema_ckout(){
  extern char const * fsl_schema_ckout_cstr;
  return fsl_schema_ckout_cstr;
}

char const * fsl_schema_repo2(){
  extern char const * fsl_schema_repo2_cstr;
  return fsl_schema_repo2_cstr;
}

char const * fsl_schema_repo1(){
  extern char const * fsl_schema_repo1_cstr;
  return fsl_schema_repo1_cstr;
}

char const * fsl_schema_config(){
  extern char const * fsl_schema_config_cstr;
  return fsl_schema_config_cstr;
}

char const * fsl_schema_ticket_reports(){
  extern char const * fsl_schema_ticket_reports_cstr;
  return fsl_schema_ticket_reports_cstr;
}

char const * fsl_schema_ticket(){
  extern char const * fsl_schema_ticket_cstr;
  return fsl_schema_ticket_cstr;
}

char const * fsl_schema_forum(){
  extern char const * fsl_schema_forum_cstr;
  return fsl_schema_forum_cstr;
}
