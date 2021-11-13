#include "type_trait.hpp"

#include "../../lua_env/enums/enums.hpp"



namespace elona
{

TraitDB the_trait_db;
const constexpr char* data::DatabaseTraits<TraitDB>::type_id;



TraitData TraitDB::convert(const lua::ConfigTable& data, const std::string& id)
{
    DATA_INTEGER_ID();
    DATA_ENUM(trait_type, int, TraitTypeTable, 0 /* feat */);
    DATA_REQ(min, int);
    DATA_REQ(max, int);

    return TraitData{
        data::InstanceId{id},
        integer_id,
        trait_type,
        min,
        max,
    };
}

} // namespace elona
