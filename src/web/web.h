// web.h — the web task: serves the embedded web app and a small JSON API.
//
// Runs on core 0 below the Wi-Fi and Bluetooth stacks, and never touches
// LVGL: a saved layout reaches the screen through config_mark_dirty().
// Routes: docs/architecture.md, "Web API".
#pragma once

// Start the HTTP server task on port 80. Call after net_begin().
bool web_begin();
