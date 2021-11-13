#include "type_fish.hpp"



namespace elona
{

FishDB the_fish_db;
const constexpr char* data::DatabaseTraits<FishDB>::type_id;



FishData FishDB::convert(const lua::ConfigTable& data, const std::string& id)
{
    DATA_INTEGER_ID();
    DATA_OPT_OR(no_generate, bool, false);
    DATA_OPT_OR(rank, int, 0);
    DATA_OPT_OR(rarity, int, 0);
    DATA_OPT_OR(difficulty, int, 0);
    DATA_OPT_OR(weight, int, 1);
    DATA_OPT_OR(value, int, 1);
    DATA_OPT_OR(item_id, int, 618);

    return FishData{
        data::InstanceId{id},
        integer_id,
        no_generate,
        rank,
        rarity,
        difficulty,
        weight,
        value,
        item_id,
    };
}

} // namespace elona
