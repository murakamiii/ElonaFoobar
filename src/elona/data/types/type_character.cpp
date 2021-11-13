#include "type_character.hpp"

#include "../../lua_env/enums/enums.hpp"
#include "../util.hpp"

namespace elona
{

CharacterDB the_character_db;
const constexpr char* data::DatabaseTraits<CharacterDB>::type_id;



static std::vector<int> _convert_chara_flags(
    const lua::ConfigTable& data,
    const std::string& id)
{
    std::vector<int> flag_types;

    if (auto it = data.optional<sol::table>(id))
    {
        for (const auto& kvp : *it)
        {
            std::string variant_name = kvp.second.as<std::string>();
            int variant_value =
                lua::LuaEnums::CharaFlagTable.ensure_from_string(variant_name);
            flag_types.push_back(variant_value);
        }
    }

    return flag_types;
}



CharacterData CharacterDB::convert(
    const lua::ConfigTable& data,
    const std::string& id)
{
    DATA_INTEGER_ID();
    DATA_OPT_OR(ai_act_sub_freq, int, 0);
    DATA_OPT_OR(ai_calm, int, 0);
    DATA_OPT_OR(ai_dist, int, 0);
    DATA_OPT_OR(ai_heal, int, 0);
    DATA_OPT_OR(ai_move, int, 0);
    DATA_OPT_OR(can_talk, int, 0);
    DATA_ENUM(color, ColorIndex, ColorIndexTable, ColorIndex::none);
    DATA_OPT_OR(creaturepack, int, 0);
    DATA_OPT_OR(cspecialeq, int, 0);
    DATA_OPT_OR(damage_reaction_info, int, 0);
    DATA_OPT_OR(item_type, int, 0);
    DATA_OPT_OR(element_of_unarmed_attack, int, 0);
    DATA_OPT_OR(eqammo_0, int, 0);
    DATA_OPT_OR(eqammo_1, int, 0);
    DATA_OPT_OR(eqmultiweapon, int, 0);
    DATA_OPT_OR(eqrange_0, int, 0);
    DATA_OPT_OR(eqrange_1, int, 0);
    DATA_OPT_OR(eqring1, int, 0);
    DATA_OPT_OR(eqtwohand, int, 0);
    DATA_OPT_OR(eqweapon1, int, 0);
    DATA_OPT_OR(female_image, int, 0);
    DATA_OPT_OR(fixlv, int, 0);
    DATA_OPT_OR(has_random_name, bool, false);
    DATA_OPT_OR(image, int, 0);
    DATA_OPT_OR(level, int, 0);
    DATA_OPT_OR(male_image, int, 0);
    DATA_ENUM(original_relationship, int, RelationshipTable, 0);

    // Portrait
    std::string portrait_male;
    std::string portrait_female;
    {
        const auto common = data.optional<std::string>("portrait");
        if (common)
        {
            portrait_male = portrait_female = *common;
        }
        else
        {
            auto male_and_female =
                data.unordered_map<std::string, std::string>("portrait");
            portrait_male = male_and_female["male"];
            portrait_female = male_and_female["female"];
        }
    }

    DATA_OPT_OR(race, std::string, "");
    DATA_OPT_OR(class_, std::string, "");
    DATA_ENUM(sex, int, GenderTable, -1);
    DATA_OPT_OR(fltselect, int, 0);
    DATA_OPT_OR(category, int, 0);
    DATA_OPT_OR(rarity, int, 10000);
    DATA_OPT_OR(coefficient, int, 400);
    DATA_OPT(corpse_eating_callback, std::string);
    DATA_OPT(dialog_id, std::string);
    DATA_VEC(normal_actions, int);
    DATA_VEC(special_actions, int);

    const auto resistances = data::convert_id_number_table(data, "resistances");

    if (normal_actions.empty())
    {
        normal_actions = {-1};
    }

    std::vector<int> flag_types = _convert_chara_flags(data, "flags");

    // TODO: cannot set bit flags off.
    decltype(CharacterData::_flags) flags;
    for (const auto& type : flag_types)
    {
        flags[type] = true;
    }

    // TODO: validate by regex/alphanum-only
    std::string filter = data::convert_tags(data, "tags");

    return CharacterData{
        data::InstanceId{id},
        integer_id,
        normal_actions,
        special_actions,
        ai_act_sub_freq,
        ai_calm,
        ai_dist,
        ai_heal,
        ai_move,
        can_talk,
        data::InstanceId{class_},
        static_cast<ColorIndex>(color),
        creaturepack,
        cspecialeq,
        damage_reaction_info,
        item_type,
        element_of_unarmed_attack,
        eqammo_0,
        eqammo_1,
        eqmultiweapon,
        eqrange_0,
        eqrange_1,
        eqring1,
        eqtwohand,
        eqweapon1,
        female_image,
        filter,
        fixlv,
        has_random_name,
        image,
        level,
        male_image,
        original_relationship,
        portrait_male,
        portrait_female,
        data::InstanceId{race},
        sex,
        resistances,
        fltselect,
        category,
        rarity,
        coefficient,
        corpse_eating_callback,
        dialog_id,
        flags,
    };
}

} // namespace elona
