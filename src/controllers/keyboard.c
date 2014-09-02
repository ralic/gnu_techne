/* Copyright (C) 2009 Papavasileiou Dimitris
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lua.h>
#include <lauxlib.h>
#include <ctype.h>
#include <string.h>

#include "techne.h"
#include "input.h"
#include "keyboard.h"

@implementation Keyboard

-(void) input
{
    GdkEvent *event;

    for (event = [Input first] ; event ; event = [Input next]) {
        assert (event);

        if (event->type == GDK_KEY_PRESS ||
            event->type == GDK_KEY_RELEASE) {
            char *name;
            unsigned int k;

            t_pushuserdata (_L, 1, self);

            k = event->key.keyval;
            name = gdk_keyval_name (k);

            if (k > 255 || !isalnum(k)) {
                char *c, new[strlen(name) + 1];

                /* Make a temporary copy of the name and down-case it. */

                strcpy(new, name);

                for (c = new ; *c ; c += 1) {
                    if (isupper(*c)) {
                        *c = tolower(*c);
                    }
                }

                lua_pushstring (_L, new);
            } else {
                lua_pushstring (_L, name);
            }

            if (event->type == GDK_KEY_PRESS) {
                t_callhook (_L, self->buttonpress, 2, 0);
            } else {
                t_callhook (_L, self->buttonrelease, 2, 0);
            }
        }
    }

    [super input];
}

@end
