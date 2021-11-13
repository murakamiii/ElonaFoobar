#pragma once
#include "../../draw.hpp"
#include "../../pic_loader/extent.hpp"
#include "../base_database.hpp"

namespace elona
{

struct CharaChipData
{
    data::InstanceId id;
    int integer_id;
    Extent rect;
    CharaChip chip;
    optional<fs::path> filepath;
};



// Used only as an intermediary between registry and initialize_chara_chips().
ELONA_DEFINE_LUA_DB(CharaChipDB, CharaChipData, "core.chara_chip")

} // namespace elona
