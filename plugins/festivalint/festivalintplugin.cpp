/***************************************************** vim:set ts=4 sw=4 sts=4:
  festivalintplugin.cpp
  Generating the factories so festival (interactive) can be used as plug in.
  -------------------
  Copyright : (C) 2004 Gary Cramblitt
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>
  Current Maintainer: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 ***************************************************************************/

// $Id$

#include <kgenericfactory.h>

#include "festivalintconf.h"
#include "festivalintproc.h"

typedef K_TYPELIST_2( FestivalIntProc, FestivalIntConf ) Festival;
K_EXPORT_COMPONENT_FACTORY( libfestivalintplugin, KGenericFactory<Festival>("plugin_FestivalInt") );
