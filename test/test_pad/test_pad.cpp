// test_pad.cpp — native tests for the pad reader's bounds checks.
//   pio test -e native
// Runs on the PC, not the board: pad_parse.cpp and defaults.cpp are plain C++.

#include <unity.h>

#include <fstream>
#include <sstream>
#include <string>

#include "config/config.h"
#include "config/pad_parse.h"

static uint16_t s_strokes[8192];

// Parse the pad section of `json`; returns nullptr or the problem.
static const char* parse(const std::string& json, Pad& out) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) {
        return "bad json";
    }
    StrokeBuffer buffer = {s_strokes, sizeof(s_strokes) / sizeof(s_strokes[0]), 0};
    return pad_parse(doc["pad"], out, buffer);
}

// A minimal valid file with one tile; `tile` replaces the tile's JSON.
static std::string one_tile(const std::string& tile, const std::string& root_extra = "") {
    return R"({"pad":{"v":1,"name":"T","density":0,"brightness":180,"dim":0)" + root_extra +
           R"(,"pages":[{"name":"P","tiles":[)" + tile + "]}]}}";
}
static const std::string kGoodTile =
    R"({"x":0,"y":0,"w":1,"h":1,"label":"A","icon":1,"bg":"3D8BFF","fg":"0E0F10","accent":"0E0F10","bar":false,"a":{"t":"k","m":1,"k":[6]}})";

static Pad s_pad;
static Pad s_default;

void setUp() {}
void tearDown() {}

// data/config.json (from the browser compiler) must match the layout built
// into the firmware, so a pad without a file looks the same as a fresh one.
void test_default_file_matches_builtin_layout() {
    std::ifstream file("data/config.json");
    TEST_ASSERT_TRUE_MESSAGE(file.good(), "run `node tools/make_default_config.js` first");
    std::stringstream text;
    text << file.rdbuf();

    TEST_ASSERT_NULL(parse(text.str(), s_pad));
    pad_fill_default(s_default);

    TEST_ASSERT_EQUAL_STRING(s_default.name, s_pad.name);
    TEST_ASSERT_EQUAL(s_default.density, s_pad.density);
    TEST_ASSERT_EQUAL(s_default.brightness, s_pad.brightness);
    TEST_ASSERT_EQUAL(s_default.page_count, s_pad.page_count);
    for (int p = 0; p < s_pad.page_count; p++) {
        const PadPage& a = s_default.pages[p];
        const PadPage& b = s_pad.pages[p];
        TEST_ASSERT_EQUAL_STRING(a.name, b.name);
        TEST_ASSERT_EQUAL(a.tile_count, b.tile_count);
        for (int i = 0; i < b.tile_count; i++) {
            const PadTile& x = a.tiles[i];
            const PadTile& y = b.tiles[i];
            TEST_ASSERT_EQUAL_STRING(x.label, y.label);
            TEST_ASSERT_EQUAL(x.x, y.x);
            TEST_ASSERT_EQUAL(x.y, y.y);
            TEST_ASSERT_EQUAL(x.w, y.w);
            TEST_ASSERT_EQUAL(x.h, y.h);
            TEST_ASSERT_EQUAL(x.icon, y.icon);
            TEST_ASSERT_EQUAL_HEX32(x.bg, y.bg);
            TEST_ASSERT_EQUAL_HEX32(x.fg, y.fg);
            TEST_ASSERT_EQUAL_HEX32(x.accent, y.accent);
            TEST_ASSERT_EQUAL(x.bar, y.bar);
            TEST_ASSERT_EQUAL((int)x.action.type, (int)y.action.type);
            TEST_ASSERT_EQUAL_HEX8(x.action.modifiers, y.action.modifiers);
            TEST_ASSERT_EQUAL(x.action.key_count, y.action.key_count);
            TEST_ASSERT_EQUAL_HEX8_ARRAY(x.action.keys, y.action.keys, PAD_MAX_CHORD_KEYS);
            TEST_ASSERT_EQUAL_HEX16(x.action.usage, y.action.usage);
            TEST_ASSERT_EQUAL(x.action.stroke_count, y.action.stroke_count);
            if (x.action.stroke_count) {
                TEST_ASSERT_EQUAL_HEX16_ARRAY(x.action.strokes, y.action.strokes, x.action.stroke_count);
            }
        }
    }
}

void test_good_tile() {
    TEST_ASSERT_NULL(parse(one_tile(kGoodTile), s_pad));
    TEST_ASSERT_EQUAL(1, s_pad.pages[0].tile_count);
    TEST_ASSERT_EQUAL_HEX32(0x3D8BFF, s_pad.pages[0].tiles[0].bg);
    TEST_ASSERT_EQUAL((int)ActionType::Chord, (int)s_pad.pages[0].tiles[0].action.type);
}

void test_newer_format_is_rejected() {
    std::string json = one_tile(kGoodTile);
    json.replace(json.find("\"v\":1"), 5, "\"v\":2");
    TEST_ASSERT_EQUAL_STRING("newer layout format", parse(json, s_pad));
}

void test_tile_off_the_grid() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("\"x\":0"), 5, "\"x\":4");
    tile.replace(tile.find("\"w\":1"), 5, "\"w\":2");   // 4 + 2 > 5 columns
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: off the grid", parse(one_tile(tile), s_pad));
}

void test_span_must_be_1_or_2() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("\"h\":1"), 5, "\"h\":3");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: w and h must be 1 or 2", parse(one_tile(tile), s_pad));
}

void test_label_too_long() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("\"A\""), 3, "\"0123456789012345678901234\"");   // 25 bytes
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: label longer than 24 bytes", parse(one_tile(tile), s_pad));
}

void test_bad_colour() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("3D8BFF"), 6, "3D8BFG");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: colours must be RRGGBB", parse(one_tile(tile), s_pad));
}

void test_keystrokes_parse() {
    std::string tile = kGoodTile;
    tile.replace(tile.find(R"({"t":"k","m":1,"k":[6]})"), 23, R"({"t":"s","d":10,"s":"0206000B","np":true})");
    TEST_ASSERT_NULL(parse(one_tile(tile), s_pad));
    const PadAction& a = s_pad.pages[0].tiles[0].action;
    TEST_ASSERT_EQUAL((int)ActionType::Keystrokes, (int)a.type);
    TEST_ASSERT_EQUAL(2, a.stroke_count);
    TEST_ASSERT_EQUAL_HEX16(0x0206, a.strokes[0]);
    TEST_ASSERT_EQUAL_HEX16(0x000B, a.strokes[1]);
    TEST_ASSERT_TRUE(a.numpad);
}

void test_keystrokes_must_be_whole() {
    std::string tile = kGoodTile;
    tile.replace(tile.find(R"({"t":"k","m":1,"k":[6]})"), 23, R"({"t":"s","d":10,"s":"02060"})");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: text: keystroke list is not 4 hex digits each",
                             parse(one_tile(tile), s_pad));
}

void test_delay_range() {
    std::string tile = kGoodTile;
    tile.replace(tile.find(R"({"t":"k","m":1,"k":[6]})"), 23, R"({"t":"s","d":1,"s":"0206"})");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: text: delay must be 5-100 ms", parse(one_tile(tile), s_pad));
}

void test_consumer_range() {
    std::string tile = kGoodTile;
    tile.replace(tile.find(R"({"t":"k","m":1,"k":[6]})"), 23, R"({"t":"c","u":4096})");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: media key: bad usage code", parse(one_tile(tile), s_pad));
}

void test_too_many_chord_keys() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("[6]"), 3, "[4,5,6,7,8,9,10]");
    TEST_ASSERT_EQUAL_STRING("page 1 tile 1: key chord: more than 6 keys", parse(one_tile(tile), s_pad));
}

void test_null_action_is_harmless() {
    std::string tile = kGoodTile;
    tile.replace(tile.find(R"({"t":"k","m":1,"k":[6]})"), 23, "null");
    TEST_ASSERT_NULL(parse(one_tile(tile), s_pad));
    TEST_ASSERT_EQUAL((int)ActionType::None, (int)s_pad.pages[0].tiles[0].action.type);
}

void test_compact_grid_allows_column_5() {
    std::string tile = kGoodTile;
    tile.replace(tile.find("\"x\":0"), 5, "\"x\":5");
    std::string json = one_tile(tile);
    json.replace(json.find("\"density\":0"), 11, "\"density\":1");
    TEST_ASSERT_NULL(parse(json, s_pad));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_default_file_matches_builtin_layout);
    RUN_TEST(test_good_tile);
    RUN_TEST(test_newer_format_is_rejected);
    RUN_TEST(test_tile_off_the_grid);
    RUN_TEST(test_span_must_be_1_or_2);
    RUN_TEST(test_label_too_long);
    RUN_TEST(test_bad_colour);
    RUN_TEST(test_keystrokes_parse);
    RUN_TEST(test_keystrokes_must_be_whole);
    RUN_TEST(test_delay_range);
    RUN_TEST(test_consumer_range);
    RUN_TEST(test_too_many_chord_keys);
    RUN_TEST(test_null_action_is_harmless);
    RUN_TEST(test_compact_grid_allows_column_5);
    return UNITY_END();
}
