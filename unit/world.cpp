/* MegaZeux
 *
 * Copyright (C) 2021 Alice Rowan <petrifiedrowan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * Unit tests for misc. world format functionality. More comprehensive world
 * world loader testing for valid worlds is performed by the test worlds.
 */

#include "Unit.hpp"

#include "../src/world.h"
#include "../src/world_format.h"
#include "../src/io/memfile.h"
#include "../src/io/zip.h"

#include "../src/network/Scoped.hpp"

/**
 * world.c functions.
 */

struct world_magic_data
{
  const char *world_magic;
  const char *save_magic;
  const char *version_string;
  int expected;
};

UNITTEST(Magic)
{
  static const char UNKNOWN[] = "<unknown 0000h>";
  static const world_magic_data magic_data[] =
  {
    // Valid (DOS).
    { "MZX",        "MZSAV",        "1.xx",         V100   },
    { "MZ2",        "MZSV2",        "2.xx/2.51",    V200   },
    { "MZA",        "MZXSA",        "2.51s1",       V251s1 },
    { "M\x02\x09",  "MZS\x02\x09",  "2.51s2/2.61",  V251s2 },
    { "M\x02\x11",  nullptr,        "<decrypted>",  VERSION_DECRYPT }, // Only generated by world decryption.
    { "M\x02\x32",  "MZS\x02\x32",  "2.62/2.62b",   V262 },
    { "M\x02\x41",  "MZS\x02\x41",  "2.65/2.68",    V265 },
    { "M\x02\x44",  "MZS\x02\x44",  "2.68",         V268 },
    { "M\x02\x45",  "MZS\x02\x45",  "2.69",         V269 },
    { "M\x02\x46",  "MZS\x02\x46",  "2.69b",        V269b },
    { "M\x02\x48",  "MZS\x02\x48",  "2.69c",        V269c },
    { "M\x02\x49",  "MZS\x02\x49",  "2.70",         V270 },

    // Valid (port).
    { "M\x02\x50",  "MZS\x02\x50",  "2.80",   V280 },
    { "M\x02\x51",  "MZS\x02\x51",  "2.81",   V281 },
    { "M\x02\x52",  "MZS\x02\x52",  "2.82",   V282 },
    { "M\x02\x53",  "MZS\x02\x53",  "2.83",   V283 },
    { "M\x02\x54",  "MZS\x02\x54",  "2.84",   V284 },
    { "M\x02\x5A",  "MZS\x02\x5A",  "2.90",   V290 },
    { "M\x02\x5B",  "MZS\x02\x5B",  "2.91",   V291 },
    { "M\x02\x5C",  "MZS\x02\x5C",  "2.92",   V292 },
    { "M\x02\x5D",  "MZS\x02\x5D",  "2.93",   V293 },
    { "M\x02\x5E",  "MZS\x02\x5E",  "2.94",   0x025E },
    { "M\x02\x5F",  "MZS\x02\x5F",  "2.95",   0x025F },
    { "M\x03\x00",  "MZS\x03\x00",  "3.00",   0x0300 },
    { "M\x09\xFF",  "MZS\x09\xFF",  "9.255",  0x09FF },

    // Invalid.
    { "M\x01\x00",  "MZS\x01\x00",  UNKNOWN,  0 },
    { "MZB",        "MZXZB",        UNKNOWN,  0 },
    { "MZM",        "MZSZM",        UNKNOWN,  0 },
    { "MTM",        "MZDTM",        UNKNOWN,  0 },
    { "MOD",        "MOD",          UNKNOWN,  0 },
    { "NZX",        "NNNN",         UNKNOWN,  0 },
    { "\xff\x00",   "\xff\x00",     UNKNOWN,  0 },
    { "",           "",             UNKNOWN,  0 },
    { "M",          "M",            UNKNOWN,  0 },
    { "MZ",         "MZ",           UNKNOWN,  0 },
    { nullptr,      "MZS",          UNKNOWN,  0 },
    { nullptr,      "MZSV",         UNKNOWN,  0 },
    { nullptr,      "MZX",          UNKNOWN,  0 },
    { nullptr,      "MZXS",         UNKNOWN,  0 },
  };

  SECTION(world_magic)
  {
    for(const world_magic_data &d : magic_data)
    {
      if(!d.world_magic)
        continue;

      int value = world_magic(d.world_magic);
      ASSERTEQ(value, d.expected, "%s", d.world_magic);
    }
  }

  SECTION(save_magic)
  {
    for(const world_magic_data &d : magic_data)
    {
      if(!d.save_magic)
        continue;

      int value = save_magic(d.save_magic);
      ASSERTEQ(value, d.expected, "%s", d.save_magic);
    }
  }

  SECTION(get_version_string)
  {
    char buffer[16];
    for(const world_magic_data &d : magic_data)
    {
      size_t len = get_version_string(buffer, (enum mzx_version)d.expected);
      ASSERTCMP(buffer, d.version_string, "");
      ASSERTEQ(len, strlen(buffer), "return value should be length: %s", buffer);
    }
  }
}

/**
 * world_format.h functions.
 */

struct world_assign_file_input
{
  const char *filename;
  uint16_t method;
};

struct world_assign_file_result
{
  const char *filename;
  uint32_t file_id;
  uint8_t board_id;
  uint8_t robot_id;
};

static void init_test_archive(struct zip_archive &zp,
 struct zip_file_header **fha, ScopedPtr<uint8_t[]> *fh_ptrs,
 const world_assign_file_input *input, size_t NUM_FILES)
{
  /* This function only needs these zip_archive fields. */
  zp.files = fha;
  zp.num_files = NUM_FILES;

  for(size_t i = 0; i < NUM_FILES; i++)
  {
    const world_assign_file_input &d = input[i];
    size_t name_len = strlen(d.filename);

    fh_ptrs[i] = new uint8_t[sizeof(zip_file_header) + name_len]{};
    fha[i] = reinterpret_cast<zip_file_header *>(fh_ptrs[i].get());

    fha[i]->method = d.method;
    fha[i]->offset = (i << 8);
    fha[i]->file_name_length = name_len;
    memcpy(fha[i]->file_name, d.filename, name_len + 1);
  }
}

static void verify_test_archive(const struct zip_archive &zp,
 const world_assign_file_result *expected, size_t NUM_FILES)
{
  for(size_t i = 0; i < NUM_FILES; i++)
  {
    const world_assign_file_result &d = expected[i];
    const zip_file_header *fh = zp.files[i];

    ASSERTCMP(fh->file_name, d.filename, "%s", d.filename);
    ASSERTEQ(fh->mzx_file_id, d.file_id, "%s", d.filename);
    ASSERTEQ(fh->mzx_board_id, d.board_id, "%s", d.filename);
    ASSERTEQ(fh->mzx_robot_id, d.robot_id, "%s", d.filename);
  }
}

/**
 * Make sure world_assign_file_ids correctly assigns certain files their
 * corresponding world loading IDs and sorts them as expected. Also make
 * sure invalid compression methods are rejected a file ID.
 */
UNITTEST(world_assign_file_ids)
{
  static const size_t MAX_FILES = 256;

  ScopedPtr<uint8_t[]> fh_ptrs[MAX_FILES];
  struct zip_file_header *fha[MAX_FILES];
  struct zip_archive zp{};

  SECTION(World)
  {
    static const world_assign_file_input input[] =
    {
      // Typical valid files.
      { "world",    ZIP_M_NONE },
      { "gr",       ZIP_M_NONE },
      { "sfx",      ZIP_M_DEFLATE },  // Custom SFX only.
      { "chars",    ZIP_M_DEFLATE },
      { "pal",      ZIP_M_NONE },
      { "palidx",   ZIP_M_NONE },     // SMZX 3 only.
      { "vco",      ZIP_M_DEFLATE },
      { "vch",      ZIP_M_DEFLATE },
      { "palint",   ZIP_M_NONE },     // Save only.
      { "spr",      ZIP_M_DEFLATE },  // Save only.
      { "counter",  ZIP_M_DEFLATE },  // Save only.
      { "string",   ZIP_M_DEFLATE },  // Save only.
      { "b00",      ZIP_M_NONE },
      { "b01",      ZIP_M_NONE },
      { "b01bid",   ZIP_M_DEFLATE },
      { "b01bpr",   ZIP_M_DEFLATE },
      { "b01bco",   ZIP_M_DEFLATE },
      { "b01uid",   ZIP_M_DEFLATE },
      { "b01upr",   ZIP_M_DEFLATE },
      { "b01uco",   ZIP_M_DEFLATE },
      { "b01och",   ZIP_M_DEFLATE },
      { "b01oco",   ZIP_M_DEFLATE },
      { "b01r01",   ZIP_M_NONE },
      { "b01rFF",   ZIP_M_NONE },
      { "b01sc01",  ZIP_M_NONE },
      { "b01scFF",  ZIP_M_NONE },
      { "b01se01",  ZIP_M_NONE },
      { "b01seFF",  ZIP_M_NONE },
      // Filenames are case-insensitive.
      { "B02",      ZIP_M_NONE },
      { "B02BID",   ZIP_M_DEFLATE },
      { "B02bPr",   ZIP_M_DEFLATE },
      { "B02SE7f",  ZIP_M_NONE },
      // Unusual compression methods.
      { "b03",      ZIP_M_SHRUNK },
      { "b04",      ZIP_M_REDUCED_1 },
      { "b05",      ZIP_M_REDUCED_2 },
      { "b06",      ZIP_M_REDUCED_3 },
      { "b07",      ZIP_M_REDUCED_4 },
      { "b08",      ZIP_M_IMPLODED },
      { "b09",      ZIP_M_DEFLATE64 },
      // Invalid names.
      { "worlddd",  ZIP_M_NONE },
      { "",         ZIP_M_NONE },
      { "a",        ZIP_M_NONE },
      { "bb",       ZIP_M_NONE },
      { "ccc",      ZIP_M_NONE },
      { "dddd",     ZIP_M_NONE },
      { "eeeee",    ZIP_M_NONE },
      { "ffffff",   ZIP_M_NONE },
      { "ggggggg",  ZIP_M_NONE },
      { "hhhhhhhh", ZIP_M_NONE },
      { "iiiiiiiii",ZIP_M_NONE },
      { "b01obj",   ZIP_M_DEFLATE },
      { "bFFbi",    ZIP_M_NONE },
      { "countr",   ZIP_M_NONE },
      { "bFFse001", ZIP_M_NONE },
      // tolower() hack shouldn't promote control codes to numbers.
      { "b00r\x11", ZIP_M_NONE },
    };

    static const world_assign_file_result expected[] =
    {
      // Invalid files get filtered to the start for now.
      { "b03",      FILE_ID_NONE,                 0,  0 },
      { "b04",      FILE_ID_NONE,                 0,  0 },
      { "b05",      FILE_ID_NONE,                 0,  0 },
      { "b06",      FILE_ID_NONE,                 0,  0 },
      { "b07",      FILE_ID_NONE,                 0,  0 },
      { "b08",      FILE_ID_NONE,                 0,  0 },
      { "b09",      FILE_ID_NONE,                 0,  0 },
      { "worlddd",  FILE_ID_NONE,                 0,  0 },
      { "",         FILE_ID_NONE,                 0,  0 },
      { "a",        FILE_ID_NONE,                 0,  0 },
      { "bb",       FILE_ID_NONE,                 0,  0 },
      { "ccc",      FILE_ID_NONE,                 0,  0 },
      { "dddd",     FILE_ID_NONE,                 0,  0 },
      { "eeeee",    FILE_ID_NONE,                 0,  0 },
      { "ffffff",   FILE_ID_NONE,                 0,  0 },
      { "ggggggg",  FILE_ID_NONE,                 0,  0 },
      { "hhhhhhhh", FILE_ID_NONE,                 0,  0 },
      { "iiiiiiiii",FILE_ID_NONE,                 0,  0 },
      { "b01obj",   FILE_ID_NONE,                 0,  0 },
      { "bFFbi",    FILE_ID_NONE,                 0,  0 },
      { "countr",   FILE_ID_NONE,                 0,  0 },
      { "bFFse001", FILE_ID_NONE,                 0,  0 },
      { "b00r\x11", FILE_ID_NONE,                 0,  0 },
      // Valid files.
      { "world",    FILE_ID_WORLD_INFO,           0,  0 },
      { "gr",       FILE_ID_WORLD_GLOBAL_ROBOT,   0,  0 },
      { "sfx",      FILE_ID_WORLD_SFX,            0,  0 },
      { "chars",    FILE_ID_WORLD_CHARS,          0,  0 },
      { "pal",      FILE_ID_WORLD_PAL,            0,  0 },
      { "palidx",   FILE_ID_WORLD_PAL_INDEX,      0,  0 },
      { "vco",      FILE_ID_WORLD_VCO,            0,  0 },
      { "vch",      FILE_ID_WORLD_VCH,            0,  0 },
      { "palint",   FILE_ID_WORLD_PAL_INTENSITY,  0,  0 },
      { "spr",      FILE_ID_WORLD_SPRITES,        0,  0 },
      { "counter",  FILE_ID_WORLD_COUNTERS,       0,  0 },
      { "string",   FILE_ID_WORLD_STRINGS,        0,  0 },
      { "b00",      FILE_ID_BOARD_INFO,           0,  0 },
      { "b01",      FILE_ID_BOARD_INFO,           1,  0 },
      { "b01bid",   FILE_ID_BOARD_BID,            1,  0 },
      { "b01bpr",   FILE_ID_BOARD_BPR,            1,  0 },
      { "b01bco",   FILE_ID_BOARD_BCO,            1,  0 },
      { "b01uid",   FILE_ID_BOARD_UID,            1,  0 },
      { "b01upr",   FILE_ID_BOARD_UPR,            1,  0 },
      { "b01uco",   FILE_ID_BOARD_UCO,            1,  0 },
      { "b01och",   FILE_ID_BOARD_OCH,            1,  0 },
      { "b01oco",   FILE_ID_BOARD_OCO,            1,  0 },
      { "b01r01",   FILE_ID_ROBOT,                1,  1 },
      { "b01rFF",   FILE_ID_ROBOT,                1,  255 },
      { "b01sc01",  FILE_ID_SCROLL,               1,  1 },
      { "b01scFF",  FILE_ID_SCROLL,               1,  255 },
      { "b01se01",  FILE_ID_SENSOR,               1,  1 },
      { "b01seFF",  FILE_ID_SENSOR,               1,  255 },
      { "B02",      FILE_ID_BOARD_INFO,           2,  0 },
      { "B02BID",   FILE_ID_BOARD_BID,            2,  0 },
      { "B02bPr",   FILE_ID_BOARD_BPR,            2,  0 },
      { "B02SE7f",  FILE_ID_SENSOR,               2,  127 },
    };

    static const size_t NUM_FILES = arraysize(input);
    static_assert(NUM_FILES < MAX_FILES, "too many files!");
    samesize(input, expected);

    init_test_archive(zp, fha, fh_ptrs, input, NUM_FILES);
    world_assign_file_ids(&zp, true);
    verify_test_archive(zp, expected, NUM_FILES);
  }

  SECTION(Board)
  {
    static const world_assign_file_input input[] =
    {
      // Valid files.
      { "b00",      ZIP_M_NONE },
      { "b00bid",   ZIP_M_NONE },
      { "b00bpr",   ZIP_M_NONE },
      { "b00bco",   ZIP_M_NONE },
      { "b00uid",   ZIP_M_NONE },
      { "b00upr",   ZIP_M_NONE },
      { "b00uco",   ZIP_M_NONE },
      { "b00och",   ZIP_M_NONE },
      { "b00oco",   ZIP_M_NONE },
      { "b00r01",   ZIP_M_NONE },
      { "b00rFF",   ZIP_M_NONE },
      { "b00sc01",  ZIP_M_NONE },
      { "b00scFF",  ZIP_M_NONE },
      { "b00se01",  ZIP_M_NONE },
      { "b00seFF",  ZIP_M_NONE },
      // This shorthand for robots is valid for board/MZM mode file ID assignment.
      { "r02",      ZIP_M_NONE },
      { "rFE",      ZIP_M_NONE },
      // World files are ignored in board files.
      { "world",    ZIP_M_NONE },
      { "counter",  ZIP_M_NONE },
      // Boards >0 are ignored in board files.
      { "b01",      ZIP_M_NONE },
      { "bFFr7F",   ZIP_M_NONE },
      // Valid shorthand robot after invalid board ID should still be 0.
      { "r03",      ZIP_M_NONE },
      // Future file that is currently invalid.
      { "b00obj",   ZIP_M_DEFLATE },
    };
    static const world_assign_file_result expected[] =
    {
      // Invalid files (filtered to the front for now).
      { "world",    FILE_ID_NONE,       0, 0 },
      { "counter",  FILE_ID_NONE,       0, 0 },
      { "b01",      FILE_ID_NONE,       0, 0 },
      { "bFFr7F",   FILE_ID_NONE,       0, 0 },
      { "b00obj",   FILE_ID_NONE,       0, 0 },
      // Valid files.
      { "b00",      FILE_ID_BOARD_INFO, 0, 0 },
      { "b00bid",   FILE_ID_BOARD_BID,  0, 0 },
      { "b00bpr",   FILE_ID_BOARD_BPR,  0, 0 },
      { "b00bco",   FILE_ID_BOARD_BCO,  0, 0 },
      { "b00uid",   FILE_ID_BOARD_UID,  0, 0 },
      { "b00upr",   FILE_ID_BOARD_UPR,  0, 0 },
      { "b00uco",   FILE_ID_BOARD_UCO,  0, 0 },
      { "b00och",   FILE_ID_BOARD_OCH,  0, 0 },
      { "b00oco",   FILE_ID_BOARD_OCO,  0, 0 },
      { "b00r01",   FILE_ID_ROBOT,      0, 1 },
      { "r02",      FILE_ID_ROBOT,      0, 2 },
      { "r03",      FILE_ID_ROBOT,      0, 3 },
      { "rFE",      FILE_ID_ROBOT,      0, 254 },
      { "b00rFF",   FILE_ID_ROBOT,      0, 255 },
      { "b00sc01",  FILE_ID_SCROLL,     0, 1 },
      { "b00scFF",  FILE_ID_SCROLL,     0, 255 },
      { "b00se01",  FILE_ID_SENSOR,     0, 1 },
      { "b00seFF",  FILE_ID_SENSOR,     0, 255 },
    };

    static const size_t NUM_FILES = arraysize(input);
    static_assert(NUM_FILES < MAX_FILES, "too many files!");
    samesize(input, expected);

    init_test_archive(zp, fha, fh_ptrs, input, NUM_FILES);
    world_assign_file_ids(&zp, false);
    verify_test_archive(zp, expected, NUM_FILES);
  }
}

UNITTEST(Properties)
{
  uint8_t buffer[32];
  struct memfile mf;
  struct memfile prop;

  mfopen_wr(buffer, sizeof(buffer), &mf);
  memset(buffer, '\xff', sizeof(buffer));

  SECTION(save_prop_eof)
  {
    static const uint8_t expected[] = { '\0', '\0' };
    save_prop_eof(&mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");
  }

  SECTION(save_prop_c)
  {
    static const uint8_t expected[] = { 'A', 'b', 0x01, '\0', '\0', '\0', 'X' };
    save_prop_c(0x6241, 'X', &mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");
  }

  SECTION(save_prop_w)
  {
    static const uint8_t expected[] = { 'A', 'b', 0x02, '\0', '\0', '\0', 0x34, 0x12 };
    save_prop_w(0x6241, 0x1234, &mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");
  }

  SECTION(save_prop_d)
  {
    static const uint8_t expected[] = { 'A', 'b', 0x04, '\0', '\0', '\0', 0x78, 0x56, 0x34, 0x12 };
    save_prop_d(0x6241, 0x12345678, &mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");
  }

  SECTION(save_prop_a)
  {
    const char *str = "value! 1234567890";
    static const uint8_t expected[] =
    {
      'A', 'b', 0x11, '\0', '\0', '\0',
      'v', 'a', 'l', 'u', 'e', '!', ' ',
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
    };

    save_prop_a(0x6241, str, strlen(str), 1, &mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");

    mfseek(&mf, 0, SEEK_SET);

    static const uint8_t arr[4][2] =
    {
      { 1, 2, },
      { 3, 4, },
      { 5, 6, },
      { 7, 8, },
    };
    static const uint8_t expected2[] =
    {
      'A', 'b', 0x08, '\0', '\0', '\0',
      1, 2, 3, 4, 5, 6, 7, 8
    };
    save_prop_a(0x6241, arr, 2, 4, &mf);
    ASSERTMEM(buffer, expected2, sizeof(expected2), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected2), "");
  }

  SECTION(save_prop_s)
  {
    const char *str = "string_2_save.";
    static const uint8_t expected[] =
    {
      'A', 'b', 0x0e, '\0', '\0', '\0',
      's', 't', 'r', 'i', 'n', 'g', '_', '2', '_', 's', 'a', 'v', 'e', '.',
    };

    save_prop_s(0x6241, str, &mf);
    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");
  }

  SECTION(save_prop_v)
  {
    const char *str = "value! 1234567890";
    size_t len = strlen(str);
    static const uint8_t expected[] =
    {
      'A', 'b', 0x11, '\0', '\0', '\0',
      'v', 'a', 'l', 'u', 'e', '!', ' ',
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
    };

    save_prop_v(0x6241, len, &prop, &mf);
    size_t ret = mfwrite(str, len, 1, &prop);
    ASSERTEQ(ret, 1, "");

    ASSERTMEM(buffer, expected, sizeof(expected), "");
    ASSERTEQ((size_t)mftell(&mf), sizeof(expected), "");

    // The following tests won't be writing past the property header,
    // but to protect this against checks/asserts...
    ScopedPtr<uint8_t[]> bigbuffer = new uint8_t[0x20000];
    mfopen_wr(bigbuffer.get(), 0x20000, &mf);

    static const uint8_t expected2[] = { 0x01, 0x00, 0xff, 0x7f, '\0', '\0' };
    save_prop_v(0x0001, 0x7fff, &prop, &mf);
    ASSERTMEM(bigbuffer.get(), expected2, sizeof(expected2), "");
    ASSERTEQ(mftell(&mf), PROP_HEADER_SIZE + 0x7fff, "");

    mfseek(&mf, 0, SEEK_SET);

    static const uint8_t expected3[] = { 0x01, 0x00, 0xff, 0xff, 0x01, '\0' };
    save_prop_v(0x0001, 0x1ffff, &prop, &mf);
    ASSERTMEM(bigbuffer.get(), expected3, sizeof(expected3), "");
    ASSERTEQ(mftell(&mf), PROP_HEADER_SIZE + 0x1ffff, "");
  }

  SECTION(next_prop)
  {
    static const uint8_t input[] =
    {
      0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 'A', 'B', 'C', 'D', 'E',
      0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF,
      0x00, 0x00,
    };
    boolean ret;
    int ident;
    int length;

    mfopen(input, sizeof(input), &mf);
    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret, "next_prop (1)");
    ASSERTEQ(ident, 0x0001, "ident (1)");
    ASSERTEQ(length, 0x00000005, "length (1)");
    mfread(buffer, length, 1, &prop);
    ASSERTMEM(buffer, "ABCDE", 5, "value (1)");

    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret, "next_prop (2)");
    ASSERTEQ(ident, 0x0002, "ident (2)");
    ASSERTEQ(length, 0x00000001, "length (2)");
    mfread(buffer, length, 1, &prop);
    ASSERTMEM(buffer, "\xff", 1, "value (2)");

    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret == false, "next_prop (EOF)");

    // Currently doesn't actually read the EOF, do it manually.
    uint16_t value = mfgetw(&mf);
    ASSERTEQ(value, 0x0000, "ident (EOF)");
    ASSERTEQ((size_t)mftell(&mf), sizeof(input), "final pos");

    // Truncated header.
    mfopen(input, PROP_HEADER_SIZE - 2, &mf);
    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret == false, "next_prop (truncated header)");

    // Truncated value.
    mfopen(input, PROP_HEADER_SIZE + 2, &mf);
    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret == false, "next_prop (truncated value)");

    // Absurd length.
    static const uint8_t input2[] =
    {
      0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 'A', 'B', 'C', 'D', 'E',
    };
    mfopen(input2, sizeof(input2), &mf);
    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret == false, "next_prop (large length)");
    ASSERTEQ((size_t)mftell(&mf), PROP_HEADER_SIZE, "final pos (large length)");
  }

  SECTION(load_prop_int)
  {
    struct load_prop_int_data
    {
      int ident;
      int length;
      int value;
    };

    static const uint8_t input[] =
    {
      0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 'a',
      0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 'b', 'b',
      0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 'c', 'c', 'c',
      0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 'd', 'd', 'd', 'd',
      0x05, 0x05, 0x05, 0x00, 0x00, 0x00, 'e', 'e', 'e', 'e', 'e',
      0x00, 0x00,
    };
    static const struct load_prop_int_data expected[] =
    {
      { 0xff00, 0, 0 },
      { 0x0101, 1, 'a' },
      { 0x0202, 2, ('b' << 8) | 'b' },
      { 0x0303, 3, 0 },
      { 0x0404, 4, ('d' << 24) | ('d' << 16) | ('d' << 8) | 'd', },
      { 0x0505, 5, 0 },
    };
    boolean ret;
    int ident;
    int length;
    int value;

    mfopen(input, sizeof(input), &mf);

    for(const load_prop_int_data &d : expected)
    {
      ret = next_prop(&prop, &ident, &length, &mf);
      ASSERT(ret, "%04x", d.ident);

      ASSERTEQ(ident, d.ident, "%04x", d.ident);
      ASSERTEQ(length, d.length, "%04x", d.ident);

      value = load_prop_int(length, &prop);
      ASSERTEQ(value, d.value, "%04x", d.ident);
    }

    ret = next_prop(&prop, &ident, &length, &mf);
    ASSERT(ret == false, "EOF");
    value = mfgetw(&mf);
    ASSERTEQ(value, 0x0000, "ident (EOF)");
  }
}
