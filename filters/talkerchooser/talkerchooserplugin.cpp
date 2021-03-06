/***************************************************** vim:set ts=4 sw=4 sts=4:
  Generating the factories so Talker Chooser Filter can be used as plug in.
  -------------------
  Copyright:
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

 // KDE includes.
#include <KPluginFactory>
#include <KPluginLoader>

// KTTS includes.
#include "filterproc.h"

#include "talkerchooserconf.h"
#include "talkerchooserproc.h"

K_PLUGIN_FACTORY(TalkerChooserPluginFactory, registerPlugin<TalkerChooserProc>(); registerPlugin<TalkerChooserConf>();)
K_EXPORT_PLUGIN(TalkerChooserPluginFactory("jovie"))
