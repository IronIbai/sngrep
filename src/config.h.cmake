/**************************************************************************
**
** sngrep - SIP Messages flow viewer
**
** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#ifndef SNGREP_CONFIG_H
#define SNGREP_CONFIG_H

#define PACKAGE "@PROJECT_NAME@"
#define VERSION "@PROJECT_VERSION@"

#cmakedefine PROJECT_NAME
#cmakedefine PROJECT_VERSION
#cmakedefine PROJECT_VERSION_MAJOR
#cmakedefine PROJECT_VERSION_MINOR
#cmakedefine PROJECT_VERSION_PATCH

#cmakedefine USE_HEP

#cmakedefine USE_IPV6

#cmakedefine WITH_SSL


#endif
