/***************************************************************************
 *   Copyright (C) YEAR~YEAR by Your Name                                  *
 *   your-email@address.com                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef _FCITX_LIBTHAI_INTERNAL_H_
#define _FCITX_LIBTHAI_INTERNAL_H_

#include "libthai.h"
#include "thaikb.h"
#include <fcitx/instance.h>
#include <thai/thinp.h>

#define _(x) dgettext("fcitx-libthai", x)
#define FALLBACK_BUFF_SIZE 4
typedef unsigned char tischar_t;
typedef struct {
    FcitxGenericConfig gconfig;
    boolean do_correct;
    thstrict_t isc_mode;
    ThaiKBMap  kb_map;
} FcitxLibThaiConfig;

typedef struct {
    FcitxLibThaiConfig config;
    FcitxInstance *owner;
    tischar_t  char_buff[FALLBACK_BUFF_SIZE];
    short      buff_tail;
} FcitxLibThai;

CONFIG_BINDING_DECLARE(FcitxLibThaiConfig);

#endif
