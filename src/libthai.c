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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcitx/ime.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/module.h>
#include <fcitx/hook.h>
#include <libintl.h>
#include <thai/thinp.h>
#include <iconv.h>

#include "config.h"
#include "libthai-internal.h"
#include "thaikb.h"

static void *FcitxLibThaiCreate(FcitxInstance *instance);
static void FcitxLibThaiDestroy(void *arg);
static void FcitxLibThaiReloadConfig(void *arg);
boolean FcitxLibThaiInit(void *arg); /**< FcitxIMInit */
void FcitxLibThaiResetIM(void *arg); /**< FcitxIMResetIM */
INPUT_RETURN_VALUE FcitxLibThaiDoInput(void *arg, FcitxKeySym, unsigned int); /**< FcitxIMDoInput */
INPUT_RETURN_VALUE FcitxLibThaiGetCandWords(void *arg); /**< FcitxIMGetCandWords */
CONFIG_DEFINE_LOAD_AND_SAVE(LibThai, FcitxLibThaiConfig, "fcitx-libthai")
DECLARE_ADDFUNCTIONS(LibThai)

FCITX_DEFINE_PLUGIN(fcitx_libthai, ime2, FcitxIMClass2) = {
    FcitxLibThaiCreate,
    FcitxLibThaiDestroy,
    FcitxLibThaiReloadConfig,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static void*
FcitxLibThaiCreate(FcitxInstance *instance)
{
    FcitxLibThai *libthai = fcitx_utils_new(FcitxLibThai);
    bindtextdomain("fcitx-libthai", LOCALEDIR);
    libthai->owner = instance;
    libthai->conv = iconv_open("TIS-620", "UTF-8");

    if (libthai->conv == (iconv_t)-1)
    {
        free(libthai);
        return NULL;
    }
    if (!LibThaiLoadConfig(&libthai->config)) {
        free(libthai);
        return NULL;
    }

    FcitxIMIFace iface;
    memset( &iface, 0, sizeof(FcitxIMIFace));
    iface.Init = FcitxLibThaiInit;
    iface.ResetIM = FcitxLibThaiResetIM;
    iface.DoInput = FcitxLibThaiDoInput;
    iface.GetCandWords = FcitxLibThaiGetCandWords;
    FcitxInstanceRegisterIMv2( instance, libthai, "libthai", _("Thai"), "thai", iface, 10, "th");
    FcitxLibThaiAddFunctions(instance);
    return libthai;
}

static void
FcitxLibThaiDestroy(void *arg)
{
    FcitxLibThai *libthai = (FcitxLibThai*)arg;
    free(libthai);
}

static void
FcitxLibThaiReloadConfig(void *arg)
{
    FcitxLibThai *libthai = FcitxLibThai*)arg;
    LibThaiLoadConfig(&libthai->config); (
}

    boolean FcitxLibThaiInit(void* arg)
{
    return true;
}

static boolean
is_context_lost_key (FcitxKeySym keyval)
{
    return ((keyval & 0xFF00) == 0xFF00) &&
           (keyval == FcitxKey_BackSpace ||
            keyval == FcitxKey_Tab ||
            keyval == FcitxKey_Linefeed ||
            keyval == FcitxKey_Clear ||
            keyval == FcitxKey_Return ||
            keyval == FcitxKey_Pause ||
            keyval == FcitxKey_Scroll_Lock ||
            keyval == FcitxKey_Sys_Req ||
            keyval == FcitxKey_Escape ||
            keyval == FcitxKey_Delete ||
            /* IsCursorkey */
            (FcitxKey_Home <= keyval && keyval <= FcitxKey_Begin) ||
            /* IsKeypadKey, non-chars only */
            (FcitxKey_KP_Space <= keyval && keyval <= FcitxKey_KP_Delete) ||
            /* IsMiscFunctionKey */
            (FcitxKey_Select <= keyval && keyval <= FcitxKey_Break) ||
            /* IsFunctionKey */
            (FcitxKey_F1 <= keyval && keyval <= FcitxKey_F35));
}

static boolean
is_context_intact_key (FcitxKeySym keyval)
{
    return (((keyval & 0xFF00) == 0xFF00) &&
            ( /* IsModifierKey */
                (FcitxKey_Shift_L <= keyval && keyval <= FcitxKey_Hyper_R) ||
                (keyval == FcitxKey_Mode_switch) ||
                (keyval == FcitxKey_Num_Lock))) ||
           (((keyval & 0xFE00) == 0xFE00) &&
            (FcitxKey_ISO_Lock <= keyval && keyval <= FcitxKey_ISO_Last_Group_Lock));
}

boolean is_client_support_surrounding (FcitxLibThai *libthai)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(libthai->owner);
    return ic && (ic->contextCaps & CAPACITY_SURROUNDING_TEXT);
}

static thchar_t
FcitxLibThaiGetPrevChar (FcitxLibThai *libthai)
{

    thchar_t c;

    do {
        if (!is_client_support_surrounding ((libthai)))
        {
            break;
        }
        char *surrounding;
        uint     cursor_pos;
        uint     anchor_pos;
        const char *s;


        if (!FcitxInstanceGetSurroundingText ((libthai->owner),
                                              &surrounding, &cursor_pos, &anchor_pos))
        {
            break;
        }
        char    *tis_text = fcitx_utils_malloc0(sizeof(char)*(cursor_pos+1));
        boolean flag = false;
        s = surrounding;
        cursor_pos = fcitx_utf8_get_nth_char (s, cursor_pos) - s;
        while (*s)
        {
            const gchar *t;
            char *inbuf = s;
            char *outbuf = tis_text;
            size_t inbytesleft = cursor_pos;
            size_t outbytesleft = cursor_pos;
            iconv (libthai->conv, &inbuf, &inbytesleft, &outbuf,
                   &outbytesleft);
            if (inbytesleft == 0)
            {
                flag = true;
                break;
            }
            uint32_t c;
            t = fcitx_utf8_get_char ( s, &c);
            cursor_pos -= (t - s);
            s = t;
        }
        if (flag)
        {
            int char_index;

            char_index = fcitx_utf8_strnlen (s, cursor_pos);
            c = tis_text[char_index - 1];
        }
        free(tis_text);
        return c;

    } while(0);
    /* retrieve from the fallback buffer */
    return libthai->char_buff[libthai->buff_tail - 1];
}

INPUT_RETURN_VALUE FcitxLibThaiDoInput(void* arg, FcitxKeySym  sym, unsigned int state)
{
    FcitxLibThai *libthai = (FcitxLibThai*)arg;
    FcitxInputState* input = FcitxInstanceGetInputState( libthai->owner);
    struct thcell_t context_cell;
    struct thinpconv_t conv;
    int    shift_lv;
    tischar_t new_char;

    if (state & (FcitxKeyState_Ctrl | FcitxKeyState_Alt)
            || is_context_lost_key (sym))
    {
        FcitxLibThaiResetIM(libthai);
        return IRV_TO_PROCESS;
    }
    if (0 == sym || is_context_intact_key (sym))
    {
        return IRV_TO_PROCESS;
    }

    shift_lv = !(state & (FcitxKeyState_Shift | FcitxKeyState_ScrollLock)) ? 0
               : ((state & FcitxKeyState_ScrollLock) ? 2 : 1);
    new_char = thai_map_keycode (libthai->config.kb_map, FcitxInputStateGetKeyCode( input), shift_lv);
    if (0 == new_char)
        return IRV_TO_PROCESS;

    /* No correction -> just reject or commit */
    if (!libthai->config.do_correct)
    {
        thchar_t prev_char = ibus_libthai_engine_get_prev_char (libthai_engine);

        if (!th_isaccept (prev_char, new_char, libthai->config.isc_mode))
            goto reject_char;

        return ibus_libthai_engine_commit_chars (libthai_engine, &new_char, 1);
    }

    ibus_libthai_engine_get_prev_cell (libthai_engine, &context_cell);
    if (!th_validate_leveled (context_cell, new_char, &conv,
                              libthai->config.isc_mode))
    {
        goto reject_char;
    }

    if (conv.offset < 0)
    {
        /* Can't correct context, just fall back to rejection */
        if (!is_client_support_surrounding (engine))
            goto reject_char;

        ibus_engine_delete_surrounding_text (engine, conv.offset, -conv.offset);
    }
    ibus_libthai_engine_forget_prev_chars (libthai_engine);
    ibus_libthai_engine_remember_prev_chars (libthai_engine, new_char);

    return ibus_libthai_engine_commit_chars (libthai_engine,
            conv.conv,
            strlen((char *) conv.conv));

reject_char:
    /* gdk_beep (); */
    return TRUE;

}

void FcitxLibThaiResetIM(void* arg)
{
    FcitxLibThai *libthai = (FcitxLibThai*)arg;
    memset (libthai->char_buff, 0, FALLBACK_BUFF_SIZE);
    libthai->buff_tail = 0;
}

INPUT_RETURN_VALUE FcitxLibThaiGetCandWords(void* arg)
{

}

#include "fcitx-libthai-addfunctions.h"



