/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
#if !defined(ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED)
#define ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED
/*
  Copyright 2013-2021 Stephan Beal (https://wanderinghorse.net).



  This program is free software; you can redistribute it and/or
  modify it under the terms of the Simplified BSD License (also
  known as the "2-Clause License" or "FreeBSD License".)

  This program is distributed in the hope that it will be useful,
  but without any warranty; without even the implied warranty of
  merchantability or fitness for a particular purpose.

  *****************************************************************************
  This file is the primary header for the public APIs. It includes
  various other header files. They are split into multiple headers
  primarily becuase my poor old netbook is beginning to choke on
  syntax-highlighting them and browsing their (large) Doxygen output.
*/

/*
   config.h MUST be included first so we can set some portability
   flags and config-dependent typedefs!
*/
#include "fossil-config.h"
#include "fossil-util.h"
#include "fossil-core.h"
#include "fossil-db.h"
#include "fossil-repo.h"
#include "fossil-checkout.h"
#include "fossil-confdb.h"
#include "fossil-hash.h"
#include "fossil-auth.h"
#include "fossil-vpath.h"

#endif
/* ORG_FOSSIL_SCM_LIBFOSSIL_H_INCLUDED */
