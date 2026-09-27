// diag.h — test screens used to bring up the hardware, one per milestone.
// They stay in the tree so you can switch back to one when something breaks.
#pragma once

// M2: "TL", "TR", "BL", "BR" in the corners and "centre" in the middle.
// If they are in the wrong corners, flip SCREEN_ROTATE_CW in board_pins.h.
void diag_show_corners();
