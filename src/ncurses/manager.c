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
/**
 * @file manager.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_manager.h
 *
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <locale.h>
#include "glib-extra.h"
#include "setting.h"
#include "manager.h"
#include "packet/dissectors/packet_sip.h"
#include "capture/capture.h"
#include "ncurses/windows/auth_validate_win.h"
#include "ncurses/windows/call_list_win.h"
#include "ncurses/windows/call_flow_win.h"
#include "ncurses/windows/call_raw_win.h"
#include "ncurses/windows/filter_win.h"
#include "ncurses/windows/msg_diff_win.h"
#include "ncurses/windows/column_select_win.h"
#include "ncurses/windows/stats_win.h"
#include "ncurses/windows/save_win.h"
#include "ncurses/windows/settings_win.h"

/**
 * @brief Active windows list
 *
 * This list contains the created list of windows
 * and pointer to their main functions.
 */
static GPtrArray *windows;

gboolean
ncurses_init(GError **error)
{
    gshort bg, fg;
    const gchar *term;

    // Set Locale
    setlocale(LC_CTYPE, "");

    // Initialize curses
    if (!initscr()) {
        g_set_error(error,
                    NCURSES_ERROR,
                    NCURSES_ERROR_INIT,
                    "Unable to initialize ncurses mode.");
        return FALSE;
    }

    // Check if user wants a black background
    if (setting_has_value(SETTING_BACKGROUND, "dark")) {
        assume_default_colors(COLOR_WHITE, COLOR_BLACK);
    } else {
        use_default_colors();
    }
    // Enable Colors
    start_color();
    cbreak();

    // Dont write user input on screen
    noecho();
    // Hide the cursor
    curs_set(0);
    // Only delay ESC Sequences 25 ms (we dont want Escape sequences)
    ESCDELAY = 25;

    // Redefine some keys
    term = getenv("TERM");
    if (term
        && (!strcmp(term, "xterm") || !strcmp(term, "xterm-color") || !strcmp(term, "vt220"))) {
        define_key("\033[H", KEY_HOME);
        define_key("\033[F", KEY_END);
        define_key("\033OP", KEY_F(1));
        define_key("\033OQ", KEY_F(2));
        define_key("\033OR", KEY_F(3));
        define_key("\033OS", KEY_F(4));
        define_key("\033[11~", KEY_F(1));
        define_key("\033[12~", KEY_F(2));
        define_key("\033[13~", KEY_F(3));
        define_key("\033[14~", KEY_F(4));
        define_key("\033[17;2~", KEY_F(18));
    }

    if (setting_has_value(SETTING_BACKGROUND, "dark")) {
        fg = COLOR_WHITE;
        bg = COLOR_BLACK;
    } else {
        fg = COLOR_DEFAULT;
        bg = COLOR_DEFAULT;
    }

    // Initialize colorpairs
    init_pair(CP_CYAN_ON_DEF, COLOR_CYAN, bg);
    init_pair(CP_YELLOW_ON_DEF, COLOR_YELLOW, bg);
    init_pair(CP_MAGENTA_ON_DEF, COLOR_MAGENTA, bg);
    init_pair(CP_GREEN_ON_DEF, COLOR_GREEN, bg);
    init_pair(CP_RED_ON_DEF, COLOR_RED, bg);
    init_pair(CP_BLUE_ON_DEF, COLOR_BLUE, bg);
    init_pair(CP_WHITE_ON_DEF, COLOR_WHITE, bg);
    init_pair(CP_DEF_ON_CYAN, fg, COLOR_CYAN);
    init_pair(CP_DEF_ON_BLUE, fg, COLOR_BLUE);
    init_pair(CP_WHITE_ON_BLUE, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_BLACK_ON_CYAN, COLOR_BLACK, COLOR_CYAN);
    init_pair(CP_WHITE_ON_CYAN, COLOR_WHITE, COLOR_CYAN);
    init_pair(CP_YELLOW_ON_CYAN, COLOR_YELLOW, COLOR_CYAN);
    init_pair(CP_BLUE_ON_CYAN, COLOR_BLUE, COLOR_CYAN);
    init_pair(CP_BLUE_ON_WHITE, COLOR_BLUE, COLOR_WHITE);
    init_pair(CP_CYAN_ON_WHITE, COLOR_CYAN, COLOR_WHITE);
    init_pair(CP_CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

    // Initialize windows stack
    windows = g_ptr_array_new();

    return TRUE;
}

void
ncurses_deinit()
{
    // Clear screen before leaving
    refresh();
    // End ncurses mode
    endwin();
}

Window *
ncurses_create_window(enum WindowTypes type)
{
    // Find the panel of given type and create it
    return window_create(ncurses_find_by_type(type));
}

void
ncurses_destroy_window(Window *window)
{
    // Remove from the window list
    g_ptr_array_remove(windows, window);
    // Deallocate window memory
    window_destroy(window);
}

static gboolean
ncurses_window_panel_cmp(Window *window, PANEL *panel)
{
    return window->panel == panel;
}

static gboolean
ncurses_window_type_cmp(Window *window, gpointer type)
{
    return window->type == GPOINTER_TO_UINT(type);
}

Window *
ncurses_find_by_panel(PANEL *panel)
{
    guint index;
    if (g_ptr_array_find_with_equal_func(
        windows, panel,
        (GEqualFunc) ncurses_window_panel_cmp, &index)) {
        return g_ptr_array_index(windows, index);
    }

    return NULL;
}

Window *
ncurses_find_by_type(enum WindowTypes type)
{
    guint index;
    if (g_ptr_array_find_with_equal_func(
        windows, GUINT_TO_POINTER(type),
        (GEqualFunc) ncurses_window_type_cmp, &index)) {
        return g_ptr_array_index(windows, index);
    }

    Window *window = NULL;

    switch (type) {
        case WINDOW_CALL_LIST:
            window = call_list_win_new();
            break;
        case WINDOW_COLUMN_SELECT:
            window = column_select_win_new();
            break;
        case WINDOW_STATS:
            window = stats_win_new();
            break;
        case WINDOW_CALL_FLOW:
            window = call_flow_win_new();
            break;
        case WINDOW_CALL_RAW:
            window = call_raw_win_new();
            break;
        case WINDOW_FILTER:
            window = filter_win_new();
            break;
        case WINDOW_MSG_DIFF:
            window = msg_diff_win_new();
            break;
        case WINDOW_SAVE:
            window = save_win_new();
            break;
        case WINDOW_SETTINGS:
            window = settings_win_new();
            break;
        case WINDOW_AUTH_VALIDATE:
            window = auth_validate_win_new();
            break;
        default:
            break;
    }

    if (window != NULL) {
        g_ptr_array_add(windows, window);
    }

    return window;
}

int
ncurses_wait_for_input()
{
    Window *ui;
    WINDOW *win;
    PANEL *panel;

    // While there are still panels
    while ((panel = panel_below(NULL))) {

        // Get panel interface structure
        ui = ncurses_find_by_panel(panel);

        // Set character input timeout 200 ms
        halfdelay(REFRESHTHSECS);

        // Avoid parsing any packet while UI is being drawn
        capture_lock(capture_manager());
        // Query the interface if it needs to be redrawn
        if (window_redraw(ui)) {
            // Redraw this panel
            if (window_draw(ui) != 0) {
                ncurses_destroy_window(ui);
                capture_unlock(capture_manager());
                continue;
            }
        }
        capture_unlock(capture_manager());

        // Update panel stack
        update_panels();
        doupdate();

        // Get topmost panel
        panel = panel_below(NULL);

        // Enable key input on current panel
        win = panel_window(panel);
        keypad(win, TRUE);

        // Get pressed key
        int c = wgetch(win);

        // Timeout, no key pressed
        if (c == ERR)
            continue;

        capture_lock(capture_manager());
        // Handle received key
        int hld = KEY_NOT_HANDLED;
        while (hld != KEY_HANDLED) {
            // Check if current panel has custom bindings for that key
            hld = window_handle_key(ui, c);

            if (hld == KEY_HANDLED) {
                // Panel handled this key
                continue;
            } else if (hld == KEY_PROPAGATED) {
                // Destroy current panel
                ncurses_destroy_window(ui);
                // Try to handle this key with the previous panel
                ui = ncurses_find_by_panel(panel_below(NULL));
            } else if (hld == KEY_DESTROY) {
                ncurses_destroy_window(ui);
                break;
            } else {
                // Key not handled by UI nor propagated. Use default handler
                hld = ncurses_default_keyhandler(ui, c);
            }
        }
        capture_unlock(capture_manager());
    }

    return 0;
}

int
ncurses_default_keyhandler(Window *window, int key)
{
    enum KeybindingAction action = ACTION_UNKNOWN;

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RESIZE_SCREEN:
                ncurses_resize_panels();
                break;
            case ACTION_TOGGLE_SYNTAX:
                setting_toggle(SETTING_SYNTAX);
                break;
            case ACTION_TOGGLE_HINT:
                setting_toggle(SETTING_ALTKEY_HINT);
                break;
            case ACTION_CYCLE_COLOR:
                setting_toggle(SETTING_COLORMODE);
                break;
            case ACTION_SHOW_ALIAS:
                setting_toggle(SETTING_DISPLAY_ALIAS);
                break;
            case ACTION_SHOW_SETTINGS:
                ncurses_create_window(WINDOW_SETTINGS);
                break;
            case ACTION_TOGGLE_PAUSE:
                // Pause/Resume capture
                capture_manager()->paused = !capture_manager()->paused;
                break;
            case ACTION_SHOW_HELP:
                window_help(window);
                break;
            case ACTION_PREV_SCREEN:
                ncurses_destroy_window(window);
                break;
            default:
                // Parse next action
                continue;
        }
        // Default handler has handled the key
        break;
    }

    // Consider the key handled at this point
    return KEY_HANDLED;
}

void
ncurses_resize_panels()
{
    PANEL *panel = NULL;

    // While there are still panels
    while ((panel = panel_below(panel))) {
        // Invoke resize for this panel
        window_resize(ncurses_find_by_panel(panel));
    }
}

void
title_foot_box(PANEL *panel)
{
    int height, width;
    WINDOW *win = panel_window(panel);

    // Sanity check
    if (!win)
        return;

    // Get window size
    getmaxyx(win, height, width);
    box(win, 0, 0);
    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwhline(win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 2, width - 1, ACS_RTEE);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 2);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);

}

int
draw_message(WINDOW *win, Message *msg)
{
    return draw_message_pos(win, msg, 0);
}

int
draw_message_pos(WINDOW *win, Message *msg, int starting)
{
    int height, width, line, column;
    const char *cur_line, *payload, *method = NULL;
    int syntax = setting_enabled(SETTING_SYNTAX);
    const char *nonascii = setting_get_value(SETTING_CR_NON_ASCII);

    // Default text format
    int attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
    if (syntax)
        wattrset(win, attrs);

    // Get window of main panel
    getmaxyx(win, height, width);

    // Get message method (if request)
    if (msg_is_request(msg)) {
        method = packet_sip_method_str(msg->packet);
    }

    // Get packet payload
    cur_line = payload = (const char *) msg_get_payload(msg);

    // Print msg payload
    line = starting;
    column = 0;
    for (guint i = 0; i < strlen(payload); i++) {
        // If syntax highlighting is enabled
        if (syntax) {
            // First line highlight
            if (line == starting) {
                // Request syntax
                if (i == 0 && strncmp(cur_line, "SIP/2.0", 7))
                    attrs = A_BOLD | COLOR_PAIR(CP_YELLOW_ON_DEF);

                // Response syntax
                if (i == 8 && !strncmp(cur_line, "SIP/2.0", 7))
                    attrs = A_BOLD | COLOR_PAIR(CP_RED_ON_DEF);

                // SIP URI syntax
                if (method && i == strlen(method) + 1) {
                    attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                }
            } else {

                // Header syntax
                if (strchr(cur_line, ':') && payload + i < strchr(cur_line, ':'))
                    attrs = A_NORMAL | COLOR_PAIR(CP_GREEN_ON_DEF);

                // Call-ID Header syntax
                if (!strncasecmp(cur_line, "Call-ID:", 8) && column > 8)
                    attrs = A_BOLD | COLOR_PAIR(CP_MAGENTA_ON_DEF);

                // CSeq Heaedr syntax
                if (!strncasecmp(cur_line, "CSeq:", 5) && column > 5 && !isdigit(payload[i]))
                    attrs = A_NORMAL | COLOR_PAIR(CP_YELLOW_ON_DEF);

                // tag and branch syntax
                if (i > 0 && payload[i - 1] == ';') {
                    // Highlight branch if requested
                    if (setting_enabled(SETTING_SYNTAX_BRANCH)) {
                        if (!strncasecmp(payload + i, "branch", 6)) {
                            attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                        }
                    }
                    // Highlight tag if requested
                    if (setting_enabled(SETTING_SYNTAX_TAG)) {
                        if (!strncasecmp(payload + i, "tag", 3)) {
                            if (!strncasecmp(cur_line, "From:", 5)) {
                                attrs = A_BOLD | COLOR_PAIR(CP_DEFAULT);
                            } else {
                                attrs = A_BOLD | COLOR_PAIR(CP_GREEN_ON_DEF);
                            }
                        }
                    }
                }

                // SDP syntax
                if (strcspn(cur_line, "=") == 1)
                    attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
            }

            // Remove previous syntax
            if (strcspn(payload + i, " \n;<>") == 0) {
                wattroff(win, attrs);
                attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
            }

            // Syntax hightlight text!
            wattron(win, attrs);
        }

        // Dont print this characters
        if (payload[i] == '\r')
            continue;

        // Store where the line begins
        if (payload[i] == '\n')
            cur_line = payload + i + 1;

        // Move to the next line if line is filled or a we reach a line break
        if (column > width || payload[i] == '\n') {
            line++;
            column = 0;
            continue;
        }

        // Put next character in position
        if (isascii(payload[i])) {
            mvwaddch(win, line, column++, payload[i]);
        } else {
            mvwaddch(win, line, column++, *nonascii);
        }

        // Stop if we've reached the bottom of the window
        if (line == height)
            break;
    }

    // Disable syntax when leaving
    if (syntax)
        wattroff(win, attrs);

    // Redraw raw win
    wnoutrefresh(win);

    return line - starting;
}
