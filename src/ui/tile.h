// tile.h — one bento tile on a page.
#pragma once

#include <lvgl.h>

#include "config/pad.h"
#include "theme.h"

// Create the tile for `t` on `page`, positioned on the grid `g`.
// The tile's user data is `&t`, so a click handler can find its action.
lv_obj_t* tile_create(lv_obj_t* page, const PadTile& t, const theme::Grid& g);

// Brief outline after a tap: `ok` when the action was sent, `error` when it
// couldn't be (Bluetooth not connected).
void tile_flash(lv_obj_t* tile, bool ok);
