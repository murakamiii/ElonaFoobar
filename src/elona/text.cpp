#include "text.hpp"

#include "../util/fileutil.hpp"
#include "../util/strutil.hpp"
#include "area.hpp"
#include "chara_db.hpp"
#include "character.hpp"
#include "data/types/type_ability.hpp"
#include "draw.hpp"
#include "elona.hpp"
#include "enchantment.hpp"
#include "fov.hpp"
#include "i18n.hpp"
#include "item.hpp"
#include "lua_env/interface.hpp"
#include "map.hpp"
#include "map_cell.hpp"
#include "random.hpp"
#include "variables.hpp"



namespace elona
{

namespace
{

using RandomNameTable =
    std::pair<std::vector<std::string>, std::vector<std::string>>;
RandomNameTable _random_name_table;



elona_vector2<std::string> _rnlist;
elona_vector1<std::string> txtbuff;



optional<std::string> _random_name_internal()
{
    auto ret = choice(_random_name_table.first);

    if (ret.empty())
    {
        return none;
    }
    if (jp)
    {
        if (rnd(8) == 0)
        {
            ret += u8"ー";
        }
    }
    if (rnd(5))
    {
        ret += choice(_random_name_table.second);
    }

    const auto length = ret.size();
    if (length < 4)
    {
        return none;
    }
    if (length < 6)
    {
        if (rnd(3))
        {
            return none;
        }
    }
    if (length < 8)
    {
        if (rnd(2))
        {
            return none;
        }
    }

    if (jp)
    {
        if (strutil::starts_with(ret, u8"ー") ||
            strutil::contains(ret, u8"ーッ"))
        {
            return none;
        }
    }

    return ret;
}

} // namespace


int p_at_m34 = 0;
int talkref = 0;



// see attack.cpp
extern int cansee;

// see world.cpp
extern elona_vector1<int> ranknorma;



std::string zentohan(const std::string& str)
{
    // TODO: Implement.
    return str;
}



std::string cnvrank(int n)
{
    if (jp)
        return std::to_string(n);

    if (n % 10 == 1 && n != 11)
        return std::to_string(n) + u8"st";
    else if (n % 10 == 2 && n != 12)
        return std::to_string(n) + u8"nd";
    else if (n % 10 == 3 && n != 13)
        return std::to_string(n) + u8"rd";
    else
        return std::to_string(n) + u8"th";
}



std::string cnvarticle(const std::string& str)
{
    return i18n::s.get("core.ui.article", str);
}



std::string cnvitemname(int id)
{
    if (jp)
        return ioriginalnameref(id);

    if (ioriginalnameref2(id) == ""s)
    {
        return ioriginalnameref(id);
    }
    return ioriginalnameref2(id) + u8" of "s + ioriginalnameref(id);
}



std::string cnven(const std::string& source)
{
    if (jp)
        return source;
    if (source.empty())
        return source;

    std::string ret = source;
    if (ret[0] == '*')
    {
        if (source.size() == 1)
            return source;
        ret[1] = std::toupper(ret[1]);
    }
    else
    {
        ret[0] = std::toupper(ret[0]);
    }
    return ret;
}



std::string cnvfix(int n)
{
    return n >= 0 ? u8"+"s + std::to_string(n) : std::to_string(n);
}



std::string cnvdate(int datetime_id, bool show_hour)
{
    std::string ret;

    int hour = datetime_id % 24;
    int day = datetime_id / 24 % 30;
    if (day == 0)
    {
        day = 30;
        datetime_id -= 720;
    }
    int month = datetime_id / 24 / 30 % 12;
    if (month == 0)
    {
        month = 12;
        datetime_id -= 8640;
    }
    int year = datetime_id / 24 / 30 / 12;

    ret = i18n::s.get("core.ui.date", year, month, day);
    if (show_hour)
    {
        ret += i18n::s.get("core.ui.date_hour", hour);
    }

    return ret;
}



std::string cnvplaytime(int datetime_id)
{
    const int h = datetime_id / 60 / 60;
    const int m = datetime_id / 60 % 60;
    const int s = datetime_id % 60;
    return i18n::s.get("core.ui.playtime", h, m, s);
}



// Get rid of the job and extract the name.
// Lomias the general vendor => Lomias
std::string sncnv(const std::string& name_with_job)
{
    return name_with_job.substr(0, name_with_job.find(' ')) + ' ';
}



std::string sngeneral(const std::string& name)
{
    return i18n::s.get("core.chara.job.general_vendor", name);
}



std::string sninn(const std::string& name)
{
    return i18n::s.get("core.chara.job.innkeeper", name);
}



std::string sntrade(const std::string& name)
{
    return i18n::s.get("core.chara.job.trader", name);
}



std::string sngoods(const std::string& name)
{
    return i18n::s.get("core.chara.job.goods_vendor", name);
}



std::string snbakery(const std::string& name)
{
    return i18n::s.get("core.chara.job.baker", name);
}



std::string snmagic(const std::string& name)
{
    return i18n::s.get("core.chara.job.magic_vendor", name);
}



std::string snarmor(const std::string& name)
{
    return i18n::s.get("core.chara.job.blacksmith", name);
}



std::string sntrainer(const std::string& name)
{
    return i18n::s.get("core.chara.job.trainer", name);
}



std::string snfish(const std::string& name)
{
    return i18n::s.get("core.chara.job.fisher", name);
}



std::string snblack(const std::string& name)
{
    return i18n::s.get("core.chara.job.blackmarket", name);
}



std::string snfood(const std::string& name)
{
    return i18n::s.get("core.chara.job.food_vendor", name);
}



void initialize_nefia_names()
{
    SDIM4(mapnamerd, 20, 2, 5);
    for (int cnt = 0; cnt < 5; cnt++)
    {
        mapnamerd(0, cnt) =
            i18n::s.get_enum("core.map.nefia.prefix.type_a", cnt);
        mapnamerd(1, cnt) =
            i18n::s.get_enum("core.map.nefia.prefix.type_b", cnt);
    }
}



std::string maplevel(int)
{
    if (game_data.current_map == mdata_t::MapId::your_home)
    {
        if (game_data.current_dungeon_level != 1)
        {
            if (game_data.current_dungeon_level > 0)
            {
                return u8"B."s + (game_data.current_dungeon_level - 1);
            }
            else
            {
                return u8"L."s + (game_data.current_dungeon_level - 2) * -1;
            }
        }
    }
    if (map_shows_floor_count_in_name())
    {
        return ""s +
            cnvrank(
                   (game_data.current_dungeon_level -
                    area_data[game_data.current_map].danger_level + 1)) +
            i18n::s.get("core.map.nefia.level");
    }

    return "";
}


std::string mapname_dungeon(int id)
{
    int suffix_id = area_data[id].type;
    std::string name = mapnamerd(
        area_data[id].dungeon_prefix,
        std::min(area_data[id].danger_level / 5, int(mapnamerd.j_size() - 1)));

    if (mdata_t::is_nefia(suffix_id))
    {
        name += i18n::s.get_enum("core.map.nefia.suffix", suffix_id);
    }
    return name;
}

std::string mapname(int id, bool description)
{
    std::string name;
    std::string desc;

    switch (static_cast<mdata_t::MapId>(area_data[id].id))
    {
    case mdata_t::MapId::quest:
        if (game_data.executing_immediate_quest_type == 1001)
        {
            name = i18n::s.get("core.map.quest.outskirts");
        }
        if (game_data.executing_immediate_quest_type == 1010 ||
            game_data.executing_immediate_quest_type == 1008)
        {
            name = i18n::s.get("core.map.quest.urban_area");
        }
        break;
    case mdata_t::MapId::random_dungeon: name = mapname_dungeon(id); break;
    default:
        auto name_opt = i18n::s.get_enum_property_optional(
            "core.map.unique", "name", area_data[id].id);
        if (name_opt)
        {
            name = *name_opt;
        }
        else
        {
            name = "";
        }

        auto desc_opt = i18n::s.get_enum_property_optional(
            "core.map.unique", "desc", area_data[id].id);
        if (desc_opt)
        {
            desc = *desc_opt;
        }
        else
        {
            desc = "";
        }
    }

    if (description)
    {
        if (area_data[id].is_hidden_in_world_map())
        {
            return "";
        }
        else if (desc != ""s)
        {
            return desc;
        }
        else if (mdata_t::is_nefia(area_data[id].type))
        {
            return i18n::s.get(
                "core.map.you_see_an_entrance",
                name,
                area_data[id].danger_level);
        }
        else
        {
            return i18n::s.get("core.map.you_see", name);
        }
    }
    else
    {
        return name;
    }
}



std::string txtbuilding(int x, int y)
{
    const auto type = bddata(0, x, y);
    if (type == 0)
    {
        return "";
    }
    return i18n::s.get(
        "core.map.you_see", i18n::s.get_enum("core.map.misc_location", type));
}



std::string txtskillchange(const Character& chara, int id, bool increase)
{
    if (auto text = i18n::s.get_enum_property_optional(
            "core.skill", increase ? "increase" : "decrease", id, chara))
    {
        return *text;
    }
    else
    {
        if (increase)
        {
            return i18n::s.get(
                "core.skill.default.increase",
                chara,
                the_ability_db.get_text(id, "name"));
        }
        else
        {
            return i18n::s.get(
                "core.skill.default.decrease",
                chara,
                the_ability_db.get_text(id, "name"));
        }
    }
}



std::string _ka(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ですか"}, {u8"ですか"}},
        {{u8"かよ", u8"か"}, {u8"かい"}},
        {{u8"かい", u8"なの"}, {u8"なの"}},
        {{u8"か…", u8"かよ…"}, {u8"なの…"}},
        {{u8"かのう", u8"であるか"}, {u8"であるか"}},
        {{u8"でござるか"}, {u8"でござりまするか"}},
        {{u8"ッスか"}, {u8"かにゃ", u8"かニャン"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _da(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"です", u8"ですね"}, {u8"ですわ", u8"です"}},
        {{u8"だぜ", u8"だ"}, {u8"ね", u8"よ"}},
        {{u8"だよ"}, {u8"だわ", u8"よ"}},
        {{u8"だ…", u8"さ…"}, {u8"よ…", u8"ね…"}},
        {{u8"じゃ", u8"でおじゃる"}, {u8"じゃ", u8"でおじゃるぞ"}},
        {{u8"でござる", u8"でござるよ"}, {u8"でござりまする"}},
        {{u8"ッス"}, {u8"みゃん", u8"ミャ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _nda(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"のです", u8"んです"}, {u8"のですわ", u8"のです"}},
        {{"", u8"んだ"}, {u8"の"}},
        {{u8"んだよ", u8"んだ"}, {u8"わ", u8"のよ"}},
        {{u8"…", u8"んだ…"}, {u8"の…", u8"わ…"}},
        {{u8"のじゃ", u8"のだぞよ"}, {u8"のじゃわ", u8"のだぞよ"}},
        {{u8"のでござる"}, {u8"のでございます"}},
        {{u8"んだッス"}, {u8"のニャ", u8"のにゃん"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _noka(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"のですか", u8"んですか"}, {u8"のですか", u8"んですか"}},
        {{u8"のか", u8"のだな"}, {u8"の", u8"のかい"}},
        {{u8"のかい", u8"の"}, {u8"の"}},
        {{u8"のか…"}, {u8"の…"}},
        {{u8"のかのう", u8"のだな"}, {u8"のかね", u8"のだな"}},
        {{u8"のでござるか"}, {u8"のでございます"}},
        {{u8"のッスか"}, {u8"にゃんか", u8"ニャン"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _kana(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"でしょうか", u8"ですか"}, {u8"かしら", u8"でしょう"}},
        {{u8"か", u8"かい"}, {u8"か", u8"かい"}},
        {{u8"かな", u8"かなぁ"}, {u8"かな", u8"かなー"}},
        {{u8"かな…", u8"か…"}, {u8"かな…", u8"か…"}},
        {{u8"かのう", u8"かの"}, {u8"かのう", u8"かの"}},
        {{u8"でござるか"}, {u8"でございますか"}},
        {{u8"ッスか"}, {u8"かにゃん", u8"かニャ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _kimi(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"貴方"}, {u8"貴方"}},
        {{u8"お前"}, {u8"お前"}},
        {{u8"君"}, {u8"君"}},
        {{u8"君"}, {u8"君"}},
        {{u8"お主"}, {u8"お主"}},
        {{u8"そこもと"}, {u8"そなた様"}},
        {{u8"アンタ"}, {u8"あにゃた"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _ru(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ます", u8"ますよ"}, {u8"ますわ", u8"ますの"}},
        {{u8"るぜ", u8"るぞ"}, {u8"るわ", u8"るよ"}},
        {{u8"るよ", u8"るね"}, {u8"るの", u8"るわ"}},
        {{u8"る…", u8"るが…"}, {u8"る…", u8"るわ…"}},
        {{u8"るぞよ", u8"るぞ"}, {u8"るぞよ", u8"るぞ"}},
        {{u8"るでござる", u8"るでござるよ"}, {u8"るのでございます"}},
        {{u8"るッス"}, {u8"るのニャ", u8"るにゃん"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _tanomu(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"お願いします", u8"頼みます"},
         {u8"お願いしますわ", u8"頼みますわ"}},
        {{u8"頼む", u8"頼むな"}, {u8"頼むよ", u8"頼む"}},
        {{u8"頼むね", u8"頼むよ"}, {u8"頼むわ", u8"頼むね"}},
        {{u8"頼む…", u8"頼むぞ…"}, {u8"頼むわ…", u8"頼むよ…"}},
        {{u8"頼むぞよ"}, {u8"頼むぞよ"}},
        {{u8"頼み申す", u8"頼むでござる"}, {u8"お頼み申し上げます"}},
        {{u8"頼むッス"}, {u8"おねがいにゃ", u8"おねがいニャン"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _ore(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"私"}, {u8"私"}},
        {{u8"俺"}, {u8"あたし"}},
        {{u8"僕"}, {u8"わたし"}},
        {{u8"自分"}, {u8"自分"}},
        {{u8"麻呂"}, {u8"わらわ"}},
        {{u8"拙者"}, {u8"手前"}},
        {{u8"あっし"}, {u8"みゅー"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _ga(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ですが", u8"ですけど"}, {u8"ですが", u8"ですけど"}},
        {{u8"が", u8"がな"}, {u8"が"}},
        {{u8"けど", u8"が"}, {u8"が", u8"けど"}},
        {{u8"が…", u8"けど…"}, {u8"が…", u8"けど…"}},
        {{u8"であるが"}, {u8"であるが"}},
        {{u8"でござるが"}, {u8"でございますが"}},
        {{u8"ッスけど", u8"ッスが"}, {u8"ニャけど", u8"にゃが"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _dana(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ですね"}, {u8"ですわね", u8"ですね"}},
        {{u8"だな"}, {u8"だね", u8"ね"}},
        {{u8"だね"}, {u8"ね"}},
        {{u8"だな…"}, {u8"だね…", u8"ね…"}},
        {{u8"であるな"}, {u8"であるな"}},
        {{u8"でござるな"}, {u8"でございますね"}},
        {{u8"ッスね"}, {u8"にゃ", u8"みゃ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _kure(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ください", u8"くださいよ"}, {u8"くださいな", u8"ください"}},
        {{u8"くれ", u8"くれよ"}, {u8"くれ", u8"よ"}},
        {{u8"ね", u8"よ"}, {u8"ね", u8"ね"}},
        {{u8"くれ…", u8"…"}, {u8"よ…", u8"…"}},
        {{u8"つかわせ", u8"たもれ"}, {u8"つかわせ", u8"たもれ"}},
        {{u8"頂きたいでござる"}, {u8"くださいませ"}},
        {{u8"くれッス"}, {u8"にゃ", u8"みゃ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _daro(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"でしょう"}, {u8"でしょう"}},
        {{u8"だろ"}, {u8"だろうね"}},
        {{u8"だろうね"}, {u8"でしょ"}},
        {{u8"だろ…"}, {u8"でしょ…"}},
        {{u8"であろう"}, {u8"であろうな"}},
        {{u8"でござろうな"}, {u8"でございましょう"}},
        {{u8"ッスね"}, {u8"にゃ", u8"みゃ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _yo(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ですよ", u8"です"}, {u8"ですよ", u8"です"}},
        {{u8"ぜ", u8"ぞ"}, {u8"わ", u8"よ"}},
        {{u8"よ", u8"ぞ"}, {u8"わよ", u8"わ"}},
        {{u8"…", u8"ぞ…"}, {u8"わ…", u8"…"}},
        {{u8"であろう", u8"でおじゃる"}, {u8"であろうぞ", u8"でおじゃる"}},
        {{u8"でござろう"}, {u8"でございますわ"}},
        {{u8"ッスよ", u8"ッス"}, {u8"にゃぁ", u8"みゃぁ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _aru(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"あります", u8"ありますね"}, {u8"あります", u8"ありますわ"}},
        {{u8"ある", u8"あるな"}, {u8"あるね", u8"あるよ"}},
        {{u8"あるね", u8"あるよ"}, {u8"あるわ", u8"あるわね"}},
        {{u8"ある…", u8"あるぞ…"}, {u8"あるわ…"}},
        {{u8"あろう", u8"おじゃる"}, {u8"あろう", u8"おじゃる"}},
        {{u8"あるでござる", u8"あるでござるな"}, {u8"ござます"}},
        {{u8"あるッスよ", u8"あるッス"}, {u8"あにゅ", u8"あみぅ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _u(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"います", u8"いますよ"}, {u8"いますわ", u8"います"}},
        {{u8"うぜ", u8"うぞ"}, {u8"うわ", u8"うよ"}},
        {{u8"うよ", u8"う"}, {u8"うわ", u8"う"}},
        {{u8"う…", u8"うぞ…"}, {u8"うわ…", u8"う…"}},
        {{u8"うぞよ", u8"うぞ"}, {u8"うぞよ", u8"うぞ"}},
        {{u8"うでござる", u8"うでござるよ"}, {u8"うでございます"}},
        {{u8"うッスよ", u8"うッス"}, {u8"うにぁ", u8"うみぁ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _na(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ですね", u8"です"}, {u8"ですわ", u8"ですね"}},
        {{u8"ぜ", u8"な"}, {u8"ね", u8"な"}},
        {{u8"ね", u8"なぁ"}, {u8"わ", u8"わね"}},
        {{u8"…", u8"な…"}, {u8"…", u8"わ…"}},
        {{u8"でおじゃるな", u8"のう"}, {u8"でおじゃるな", u8"のう"}},
        {{u8"でござるな"}, {u8"でございますわ"}},
        {{u8"ッスね", u8"ッス"}, {u8"ニァ", u8"ミァ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string _ta(const Character& speaker, int mark)
{
    std::vector<std::string> candidates[][2] = {
        {{u8"ました", u8"ましたね"}, {u8"ました", u8"ましたわ"}},
        {{u8"た", u8"たな"}, {u8"たね", u8"たよ"}},
        {{u8"たね", u8"たよ"}, {u8"たよ", u8"たね"}},
        {{u8"た…", u8"たぞ…"}, {u8"たわ…"}},
        {{u8"たぞよ", u8"たぞな"}, {u8"たぞよ"}},
        {{u8"たでござる"}, {u8"ましてございます"}},
        {{u8"たッスよ", u8"たッス"}, {u8"たにゃぁ", u8"たみゃぁ"}},
    };
    return choice(candidates[speaker.talk_type][speaker.sex]) +
        i18n::s.get_enum("core.ui.mark", mark);
}



std::string replace_tag(
    const std::string source,
    optional_ref<const Character> client)
{
    if (source == u8"ref"s && talkref == 1)
    {
        return i18n::s.get(
            "core.talk.tag.ref", game_data.number_of_waiting_guests);
    }
    if (source == u8"you"s)
    {
        assert(client);
        return _kimi(*client, 3);
    }
    if (source == u8"sex"s)
    {
        return i18n::s.get_enum("core.ui.sex2", cdata.player().sex);
    }
    if (source == u8"player"s)
    {
        return cdata.player().name;
    }
    if (source == u8"aka"s)
    {
        return cdata.player().alias;
    }
    if (source == u8"npc"s)
    {
        assert(client);
        return client->name;
    }
    if (source == u8"ある"s)
    {
        assert(client);
        return _aru(*client, 3);
    }
    if (source == u8"が"s)
    {
        assert(client);
        return _ga(*client, 3);
    }
    if (source == u8"か"s)
    {
        assert(client);
        return _ka(*client, 3);
    }
    if (source == u8"かな"s)
    {
        assert(client);
        return _kana(*client, 3);
    }
    if (source == u8"だ"s)
    {
        assert(client);
        return _da(*client, 3);
    }
    if (source == u8"よ"s)
    {
        assert(client);
        return _yo(*client, 3);
    }
    if (source == u8"だな"s)
    {
        assert(client);
        return _dana(*client, 3);
    }
    if (source == u8"だろ"s)
    {
        assert(client);
        return _daro(*client, 3);
    }
    if (source == u8"る"s)
    {
        assert(client);
        return _ru(*client, 3);
    }
    if (source == u8"のだ"s)
    {
        assert(client);
        return _nda(*client, 3);
    }
    if (source == u8"な"s)
    {
        assert(client);
        return _na(*client, 3);
    }
    if (source == u8"くれ"s)
    {
        assert(client);
        return _kure(*client, 3);
    }
    return u8"Unknown Code"s;
}



void parse_talk_file(optional_ref<const Character> speaker)
{
    buff = strmid(buff, p, instr(buff, p, u8"%END"s));
    if (noteinfo() <= 1)
    {
        buff(0).clear();
        std::ifstream in{
            lua::resolve_path_for_mod("<core>/locale/<LANGUAGE>/lazy/talk.txt")
                .native(),
            std::ios::binary};
        std::string tmp;
        while (std::getline(in, tmp))
        {
            buff(0) += tmp + '\n';
        }
        p = instr(buff, 0, u8"%DEFAULT,"s + i18n::s.get("core.meta.tag"));
        buff = strmid(buff, p, instr(buff, p, u8"%END"s));
    }
    notedel(0);
    p = rnd(noteinfo());
    noteget(s, p);
    buff = s;
    text_replace_tags_in_quest_board(speaker);
}



void read_talk_file(const std::string& valn)
{
    buff = "";
    notesel(buff);
    {
        buff(0).clear();
        std::ifstream in{
            lua::resolve_path_for_mod("<core>/locale/<LANGUAGE>/lazy/talk.txt")
                .native(),
            std::ios::binary};
        std::string tmp;
        while (std::getline(in, tmp))
        {
            buff(0) += tmp + '\n';
        }
    }
    p = instr(buff, 0, valn + u8","s + i18n::s.get("core.meta.tag"));
    parse_talk_file(none);
}



void get_npc_talk(Character& chara)
{
    buff = "";
    notesel(buff);
    {
        buff(0).clear();
        std::ifstream in{
            lua::resolve_path_for_mod("<core>/locale/<LANGUAGE>/lazy/talk.txt")
                .native(),
            std::ios::binary};
        std::string tmp;
        while (std::getline(in, tmp))
        {
            buff(0) += tmp + '\n';
        }
    }
    p = -1;
    for (int cnt = 0; cnt < 1; ++cnt)
    {
        if (chara.role == Role::maid)
        {
            if (game_data.number_of_waiting_guests > 0)
            {
                talkref = 1;
                p = instr(buff, 0, u8"%MAID,"s + i18n::s.get("core.meta.tag"));
                break;
            }
        }
        if (chara.interest <= 0)
        {
            p = instr(buff, 0, u8"%BORED,"s + i18n::s.get("core.meta.tag"));
            break;
        }
        if (chara.is_player_or_ally())
        {
            p = instr(
                buff, 0, u8"%ALLY_DEFAULT,"s + i18n::s.get("core.meta.tag"));
            break;
        }
        if (chara.id == CharaId::prostitute)
        {
            p = instr(buff, 0, u8"%BITCH,"s + i18n::s.get("core.meta.tag"));
            break;
        }
        if (chara.role == Role::moyer)
        {
            p = instr(buff, 0, u8"%MOYER,"s + i18n::s.get("core.meta.tag"));
            break;
        }
        if (chara.role == Role::slave_master)
        {
            p = instr(
                buff, 0, u8"%SLAVEKEEPER,"s + i18n::s.get("core.meta.tag"));
            break;
        }
        if (is_shopkeeper(chara.role))
        {
            if (rnd(3))
            {
                p = instr(
                    buff, 0, u8"%SHOPKEEPER,"s + i18n::s.get("core.meta.tag"));
                break;
            }
        }
        if (chara.impression >= 100)
        {
            if (rnd(3) == 0)
            {
                p = instr(
                    buff, 0, u8"%RUMOR,LOOT,"s + i18n::s.get("core.meta.tag"));
                break;
            }
        }
        if (area_data[game_data.current_map].christmas_festival)
        {
            if (game_data.current_map == mdata_t::MapId::noyel)
            {
                if (rnd(3))
                {
                    p = instr(
                        buff,
                        0,
                        u8"%FEST,"s + game_data.current_map + u8","s +
                            i18n::s.get("core.meta.tag"));
                    break;
                }
            }
        }
        if (rnd(2))
        {
            p = instr(
                buff,
                0,
                u8"%PERSONALITY,"s + chara.personality + u8","s +
                    i18n::s.get("core.meta.tag"));
            break;
        }
        if (rnd(3))
        {
            p = instr(
                buff,
                0,
                u8"%AREA,"s + game_data.current_map + u8","s +
                    i18n::s.get("core.meta.tag"));
            break;
        }
    }
    if (p == -1)
    {
        p = instr(buff, 0, u8"%DEFAULT,"s + i18n::s.get("core.meta.tag"));
    }
    parse_talk_file(chara);
}



std::string cnvweight(int weight)
{
    return ""s + std::abs(weight) / 1000 + '.' + std::abs(weight) % 1000 / 100 +
        i18n::s.get("core.ui.unit_of_weight");
}



std::string fltname(int category)
{
    if (auto text =
            i18n::s.get_enum_optional("core.item.filter_name", category))
    {
        return *text;
    }
    else
    {
        return i18n::s.get("core.item.filter_name.default");
    }
}



void quest_update_main_quest_journal()
{
    int progress;

    noteadd("@QM[" + i18n::s.get("core.quest.journal.main.title") + "]");
    if (game_data.quest_flags.main_quest >= 0 &&
        game_data.quest_flags.main_quest < 30)
    {
        progress = 0;
    }
    if (game_data.quest_flags.main_quest >= 30 &&
        game_data.quest_flags.main_quest < 50)
    {
        progress = 1;
    }
    if (game_data.quest_flags.main_quest >= 50 &&
        game_data.quest_flags.main_quest < 60)
    {
        progress = 2;
    }
    if (game_data.quest_flags.main_quest >= 60 &&
        game_data.quest_flags.main_quest < 100)
    {
        progress = 3;
    }
    if (game_data.quest_flags.main_quest >= 100 &&
        game_data.quest_flags.main_quest < 110)
    {
        progress = 4;
    }
    if (game_data.quest_flags.main_quest >= 110 &&
        game_data.quest_flags.main_quest < 125)
    {
        progress = 5;
    }
    if (game_data.quest_flags.main_quest >= 125 &&
        game_data.quest_flags.main_quest < 180)
    {
        progress = 6;
    }
    if (game_data.quest_flags.main_quest >= 180 &&
        game_data.quest_flags.main_quest < 1000)
    {
        progress = 7;
    }
    s1 = i18n::s.get_enum("core.quest.journal.main.progress", progress);
    talk_conv(s1, 40 - en * 4);
    buff += s1;
    noteadd(""s);
}



void append_subquest_journal(int val0)
{
    if (val0 == 0)
    {
        noteadd(""s);
        noteadd("@QM[" + i18n::s.get("core.quest.journal.sub.title") + "]");
    }
    p = 0;
    if (game_data.quest_flags.putit_attacks != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.putit_attacks.title");
        p = game_data.quest_flags.putit_attacks;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.putit_attacks.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.putit_attacks.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.thieves_hideout != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.thieves_hideout.title");
        p = game_data.quest_flags.thieves_hideout;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.thieves_hideout.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.thieves_hideout.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.puppys_cave != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.puppys_cave.title");
        p = game_data.quest_flags.puppys_cave;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.puppys_cave.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.nightmare != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.nightmare.title");
        p = game_data.quest_flags.nightmare;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.nightmare.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.nightmare.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 3)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.nightmare.progress", 2);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.pael_and_her_mom != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.pael_and_her_mom.title");
        p = game_data.quest_flags.pael_and_her_mom;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 3)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 4)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 5)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 6)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 7)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 8)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 9)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 10)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pael_and_her_mom.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.wife_collector != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.wife_collector.title");
        p = game_data.quest_flags.wife_collector;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.wife_collector.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.cat_house != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.cat_house.title");
        p = game_data.quest_flags.cat_house;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.cat_house.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.cat_house.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.defense_line != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.defense_line.title");
        p = game_data.quest_flags.defense_line;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.defense_line.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.defense_line.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 3)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.defense_line.progress", 2);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.novice_knight != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.novice_knight.title");
        p = game_data.quest_flags.novice_knight;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.novice_knight.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.novice_knight.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.kamikaze_attack != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.kamikaze_attack.title");
        p = game_data.quest_flags.kamikaze_attack;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.kamikaze_attack.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.kamikaze_attack.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 3)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.kamikaze_attack.progress", 2);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.mias_dream != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.mias_dream.title");
        p = game_data.quest_flags.mias_dream;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.mias_dream.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.rare_books != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.rare_books.title");
        p = game_data.quest_flags.rare_books;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.rare_books.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.pyramid_trial != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.pyramid_trial.title");
        p = game_data.quest_flags.pyramid_trial;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.pyramid_trial.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.red_blossom_in_palmia != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.red_blossom_in_palmia.title");
        p = game_data.quest_flags.red_blossom_in_palmia;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.red_blossom_in_palmia.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.red_blossom_in_palmia.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.ambitious_scientist != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.ambitious_scientist.title");
        p = game_data.quest_flags.ambitious_scientist;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p ==
            game_data.quest_flags.ambitious_scientist *
                    (game_data.quest_flags.ambitious_scientist < 6) +
                (game_data.quest_flags.ambitious_scientist == 0))
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.ambitious_scientist.progress",
                0,
                (6 - game_data.quest_flags.ambitious_scientist));
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.sewer_sweeping != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.sewer_sweeping.title");
        p = game_data.quest_flags.sewer_sweeping;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.sewer_sweeping.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.sewer_sweeping.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.joining_mages_guild != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.joining_mages_guild.title");
        p = game_data.guild.joining_mages_guild;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.joining_mages_guild.progress",
                0,
                game_data.guild.mages_guild_quota);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.joining_thieves_guild != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.joining_thieves_guild.title");
        p = game_data.guild.joining_thieves_guild;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.joining_thieves_guild.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.joining_fighters_guild != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.joining_fighters_guild.title");
        p = game_data.guild.joining_fighters_guild;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.joining_fighters_guild.progress",
                0,
                game_data.guild.fighters_guild_quota,
                chara_db_get_name(
                    int2charaid(game_data.guild.fighters_guild_target)));
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.mages_guild_quota_recurring != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.quota_mages_guild.title");
        p = game_data.guild.mages_guild_quota_recurring;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.quota_mages_guild.progress",
                0,
                game_data.guild.mages_guild_quota);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.fighters_guild_quota_recurring != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.quota_fighters_guild.title");
        p = game_data.guild.fighters_guild_quota_recurring;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.quota_fighters_guild.progress",
                0,
                game_data.guild.fighters_guild_quota,
                chara_db_get_name(
                    int2charaid(game_data.guild.fighters_guild_target)));
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.guild.thieves_guild_quota_recurring != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.quota_thieves_guild.title");
        p = game_data.guild.thieves_guild_quota_recurring;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.quota_thieves_guild.progress",
                0,
                game_data.guild.thieves_guild_quota);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.minotaur_king != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.minotaur_king.title");
        p = game_data.quest_flags.minotaur_king;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.minotaur_king.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    if (val0 == 0)
    {
        if (p == 2)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.minotaur_king.progress", 1);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
    p = 0;
    if (game_data.quest_flags.little_sister != 0)
    {
        s = i18n::s.get("core.quest.journal.sub.little_sister.title");
        p = game_data.quest_flags.little_sister;
        if (val0 == 1)
        {
            if (p >= 1000)
            {
                noteadd(
                    "[" + i18n::s.get("core.quest.journal.sub.done") + "]" + s);
            }
        }
    }
    if (val0 == 0)
    {
        if (p == 1)
        {
            s1 = i18n::s.get_enum(
                "core.quest.journal.sub.little_sister.progress", 0);
            talk_conv(s1, 40 - en * 4);
            buff += u8"("s + s + u8")\n"s + s1;
            noteadd(""s);
        }
    }
}



void append_quest_item_journal()
{
    noteadd("[" + i18n::s.get("core.quest.journal.item.old_talisman") + "]");
    if (game_data.quest_flags.main_quest >= 30)
    {
        noteadd(
            "[" + i18n::s.get("core.quest.journal.item.letter_to_the_king") +
            "]");
    }
    if (game_data.quest_flags.magic_stone_of_fool != 0)
    {
        noteadd(
            "[" + i18n::s.get("core.quest.journal.item.fools_magic_stone") +
            "]");
    }
    if (game_data.quest_flags.magic_stone_of_king != 0)
    {
        noteadd(
            "[" + i18n::s.get("core.quest.journal.item.kings_magic_stone") +
            "]");
    }
    if (game_data.quest_flags.magic_stone_of_sage != 0)
    {
        noteadd(
            "[" + i18n::s.get("core.quest.journal.item.sages_magic_stone") +
            "]");
    }
}



void parse_quest_board_text(int val0)
{
    elona_vector1<std::string> buff2;
    notesel(buffboard);
    SDIM1(buff2);
    p = instr(buffboard, 0, s + u8","s + i18n::s.get("core.meta.tag"));
    buff2 = strmid(buffboard, p, instr(buffboard, p, u8"%END"s));
    notesel(buff2);
    if (noteinfo() <= 1)
    {
        buff2 = u8"no txt"s;
        return;
    }
    p = rnd(noteinfo() - 1) + 1;
    noteget(buff2, p);
    p = instr(buff2, 0, u8":"s);
    s(3) = strmid(buff2, 0, p);
    if (val0 == 2)
    {
        notesel(buff);
        return;
    }
    buff2 = strmid(buff2, p + 1, buff2(0).size() - p - 1);
    if (val0 != 2)
    {
        buff = buff2;
    }
}



void load_random_name_table()
{
    std::vector<std::string> lines;
    range::copy(
        fileutil::read_by_line(lua::resolve_path_for_mod(
            "<core>/locale/<LANGUAGE>/lazy/name.csv")),
        std::back_inserter(lines));

    const auto rows = lines.size();
    _random_name_table.first.resize(rows);
    _random_name_table.second.resize(rows);

    for (size_t i = 0; i < rows; ++i)
    {
        if (lines[i].empty())
            continue;
        const auto pair = strutil::split_on_string(lines[i], ",");
        _random_name_table.first[i] = pair.first;
        _random_name_table.second[i] = pair.second;
    }
}



std::string random_name()
{
    while (true)
    {
        if (const auto name = _random_name_internal())
        {
            return cnven(*name);
        }
    }
}



void load_random_title_table()
{
    std::vector<std::string> lines;
    range::copy(
        fileutil::read_by_line(lua::resolve_path_for_mod(
            "<core>/locale/<LANGUAGE>/lazy/ndata.csv")),
        std::back_inserter(lines));

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const auto columns = strutil::split(lines[i], ',');
        for (size_t j = 0; j < 15; ++j)
        {
            if (j < columns.size())
            {
                _rnlist(j, i) = columns[j];
            }
            else
            {
                _rnlist(j, i) = "";
            }
        }
    }
}



std::string random_title_base(RandomTitleType type)
{
    while (true)
    {
        int row;
        int column;
        while (true)
        {
            row = rnd(_rnlist.j_size());
            column = rnd(14);
            if (_rnlist(column, row) != "")
            {
                break;
            }
        }

        const auto category = _rnlist(14, row);
        if (type == RandomTitleType::weapon ||
            type == RandomTitleType::living_weapon)
        {
            if (category == u8"具"s)
            {
                continue;
            }
        }

        auto title = _rnlist(column, row);
        if (jp)
        {
            if (column == 10 || column == 11)
            {
                if (rnd(5) == 0)
                {
                    column = 0;
                    if (rnd(2) == 0)
                    {
                        title += u8"の"s;
                    }
                }
                else
                {
                    const auto p = rnd(5);
                    if (p == 0)
                    {
                        title += u8"・オブ・"s;
                    }
                    if (p == 1)
                    {
                        return u8"ザ・"s + title;
                    }
                    if (p == 2)
                    {
                        title += u8"・"s;
                    }
                }
            }
            else if (column == 0 || column == 1)
            {
                title += u8"の"s;
                if (rnd(10) == 0)
                {
                    column = 10;
                }
            }
        }
        else
        {
            if (column == 0 || column == 1)
            {
                if (rnd(6) == 0)
                {
                    title += u8" of"s;
                }
                else if (rnd(6) == 0)
                {
                    return u8"the "s + title;
                }
            }
            title = cnven(title + " ");
        }

        bool success = false;
        int new_row;
        for (int _i = 0; _i < 100; ++_i)
        {
            new_row = rnd(_rnlist.j_size());
            if (new_row == row)
            {
                continue;
            }
            if (_rnlist(14, new_row) == category && category != u8"万能"s)
            {
                continue;
            }
            if (column < 10)
            {
                column = rnd(2);
            }
            else
            {
                column = rnd(2);
                column += 10;
            }
            if (_rnlist(column, new_row) == "")
            {
                continue;
            }
            success = true;
            break;
        }

        if (!success)
        {
            continue;
        }

        if (en)
        {
            _rnlist(column, new_row) = cnven(_rnlist(column, new_row));
        }

        title += _rnlist(column, new_row);
        if (strlen_u(title) >= 28)
        {
            continue;
        }

        return title;
    }
}



std::string random_title(RandomTitleType type)
{
    auto ret = random_title_base(type);

    if (type == RandomTitleType::party)
    {
        if (jp)
        {
            if (rnd(5))
            {
                ret += choice(std::initializer_list<const char*>{
                    u8"団",
                    u8"チーム",
                    u8"パーティー",
                    u8"の集い",
                    u8"の軍",
                    u8"アーミー",
                    u8"隊",
                    u8"の一家",
                    u8"軍",
                    u8"の隊",
                    u8"の団",
                });
            }
        }
        else
        {
            if (rnd(2))
            {
                ret = choice(std::initializer_list<const char*>{
                          u8"The army of ",
                          u8"The party of ",
                          u8"The house of ",
                          u8"Clan ",
                      }) +
                    ret;
            }
            else
            {
                ret += choice(std::initializer_list<const char*>{
                    u8" Clan",
                    u8" Party",
                    u8" Band",
                    u8" Gangs",
                    u8" Gathering",
                    u8" House",
                    u8" Army",
                });
            }
        }
    }

    return ret;
}



std::string cheer_up_message(int hours)
{
    auto message = i18n::s.get_enum_optional("core.ui.cheer_up_message", hours);

    if (message)
    {
        return *message;
    }

    return "";
}



void text_replace_tags_in_quest_board(optional_ref<const Character> client)
{
    while (1)
    {
        const int p0 = instr(buff, 0, u8"{"s);
        const int p1 = instr(buff, p0, u8"}"s);
        const int p2 = buff(0).size();
        if (p0 == -1)
        {
            break;
        }
        const auto tag = strmid(buff, p0 + 1, p1 - 1);
        const auto head = strmid(buff, 0, p0);
        const auto tail = strmid(buff, p0 + p1 + 1, p2 - p1 - p0);
        buff = head + replace_tag(tag, client) + tail;
    }
}



void text_replace_tags_in_quest_text(optional_ref<const Character> client)
{
    for (int cnt = 0; cnt < 20; ++cnt)
    {
        p(0) = instr(buff, 0, u8"{"s);
        p(1) = instr(buff, p, u8"}"s);
        p(2) = buff(0).size();
        if (p == -1)
        {
            break;
        }
        s(0) = strmid(buff, p + 1, p(1) - 1);
        s(1) = strmid(buff, 0, p);
        s(2) = strmid(buff, p + p(1) + 1, p(2) - p(1) - p);
        for (int cnt = 0; cnt < 1; ++cnt)
        {
            if (s == u8"client"s)
            {
                s = s(12);
                break;
            }
            if (s == u8"map"s)
            {
                s = s(11);
                break;
            }
            if (s == u8"ref"s)
            {
                s = s(10);
                break;
            }
            if (s == u8"you"s)
            {
                assert(client);
                s = _kimi(*client, 3);
                break;
            }
            if (s == u8"me"s)
            {
                assert(client);
                s = _ore(*client, 3);
                break;
            }
            if (s == u8"reward"s)
            {
                s = s(5);
                break;
            }
            if (s == u8"objective"s)
            {
                s = s(4);
                break;
            }
            if (s == u8"deadline"s)
            {
                s = nquestdate;
                break;
            }
            if (s == u8"player"s)
            {
                s = cdata.player().name;
                break;
            }
            if (s == u8"aka"s)
            {
                s = cdata.player().alias;
                break;
            }
            if (s == u8"npc"s)
            {
                assert(client);
                s = client->name;
                break;
            }
            if (s == u8"ある"s)
            {
                assert(client);
                s = _aru(*client, 3);
                break;
            }
            if (s == u8"う"s)
            {
                assert(client);
                s = _u(*client, 3);
                break;
            }
            if (s == u8"か"s)
            {
                assert(client);
                s = _ka(*client, 3);
                break;
            }
            if (s == u8"が"s)
            {
                assert(client);
                s = _ga(*client, 3);
                break;
            }
            if (s == u8"かな"s)
            {
                assert(client);
                s = _kana(*client, 3);
                break;
            }
            if (s == u8"だ"s)
            {
                assert(client);
                s = _da(*client, 3);
                break;
            }
            if (s == u8"よ"s)
            {
                assert(client);
                s = _yo(*client, 3);
                break;
            }
            if (s == u8"た"s)
            {
                assert(client);
                s = _ta(*client, 3);
                break;
            }
            if (s == u8"だな"s)
            {
                assert(client);
                s = _dana(*client, 3);
                break;
            }
            if (s == u8"だろ"s)
            {
                assert(client);
                s = _daro(*client, 3);
                break;
            }
            if (s == u8"たのむ"s)
            {
                assert(client);
                s = _tanomu(*client, 3);
                break;
            }
            if (s == u8"る"s)
            {
                assert(client);
                s = _ru(*client, 3);
                break;
            }
            if (s == u8"のだ"s)
            {
                assert(client);
                s = _nda(*client, 3);
                break;
            }
            if (s == u8"な"s)
            {
                assert(client);
                s = _na(*client, 3);
                break;
            }
            if (s == u8"くれ"s)
            {
                assert(client);
                s = _kure(*client, 3);
                break;
            }
            s = u8"Unknown Code"s;
        }
        buff = s(1) + s + s(2);
    }
}



std::string name(int chara_index)
{
    if (chara_index == 0)
    {
        return i18n::s.get("core.chara.you");
    }
    if (is_in_fov(cdata[chara_index]) == 0)
    {
        return i18n::s.get("core.chara.something");
    }
    if (cdata.player().blind != 0 ||
        (cdata[chara_index].is_invisible() == 1 &&
         cdata.player().can_see_invisible() == 0 &&
         cdata[chara_index].wet == 0))
    {
        return i18n::s.get("core.chara.something");
    }
    if (en)
    {
        const char first = cdata[chara_index].name[0];
        if (first == '\"' || first == '<')
        {
            return cdata[chara_index].name;
        }
        if (cdata[chara_index].has_own_name() == 0)
        {
            return u8"the "s + cdata[chara_index].name;
        }
    }
    return cdata[chara_index].name;
}



std::string txtitemoncell(int x, int y)
{
    const auto& item_info_memory = cell_data.at(x, y).item_info_memory;
    if (item_info_memory.is_empty())
    {
        return "";
    }

    const auto stack_count = item_info_memory.stack_count();
    if (stack_count < 0)
    {
        return i18n::s.get(
            "core.action.move.item_on_cell.many",
            cell_count_exact_item_stacks({x, y}));
    }

    const auto item_indice = item_info_memory.item_indice();
    std::string items_text;
    bool first = true;
    auto own_state = OwnState::none;
    for (const auto& item_index : item_indice)
    {
        if (item_index == 0)
            break;

        const auto item = item_index < 0
            ? g_inv.ground().at(0) /* TODO phantom ref */
            : g_inv.ground().at(item_index - 1);
        if (first)
        {
            first = false;
            own_state = item->own_state;
        }
        else
        {
            items_text += i18n::s.get("core.misc.and");
        }
        items_text += itemname(item.unwrap());
    }
    if (own_state <= OwnState::none)
    {
        return i18n::s.get("core.action.move.item_on_cell.item", items_text);
    }
    else if (own_state == OwnState::shelter)
    {
        return i18n::s.get(
            "core.action.move.item_on_cell.building", items_text);
    }
    else
    {
        return i18n::s.get(
            "core.action.move.item_on_cell.not_owned", items_text);
    }
}



void cnvbonus(int ability_id, int bonus)
{
    // TODO: i18n
    if (ability_id >= 50 && ability_id < 61)
    {
        if (bonus > 0)
        {
            buff += u8"　　"s + the_ability_db.get_text(ability_id, "name") +
                u8"耐性に <green>クラス"s + bonus / 50 + u8"<col>("s + bonus +
                u8") のボーナス\n"s;
        }
        if (bonus < 0)
        {
            buff += u8"　　"s + the_ability_db.get_text(ability_id, "name") +
                u8"耐性に <red>クラス"s + bonus / 50 + u8"<col>("s + bonus +
                u8") のマイナス修正\n"s;
        }
    }
    else
    {
        if (bonus > 0)
        {
            buff += u8"　　"s + the_ability_db.get_text(ability_id, "name") +
                u8"に <green>+"s + bonus + u8"<col> のボーナス\n"s;
        }
        if (bonus < 0)
        {
            buff += u8"　　"s + the_ability_db.get_text(ability_id, "name") +
                u8"に <red>"s + bonus + u8"<col> のマイナス修正\n"s;
        }
    }
}



std::string get_armor_class_name(const Character& chara)
{
    int id = chara_armor_class(chara);
    if (id == 169)
    {
        return i18n::s.get("core.item.armor_class.heavy");
    }
    else if (id == 170)
    {
        return i18n::s.get("core.item.armor_class.medium");
    }
    else
    {
        return i18n::s.get("core.item.armor_class.light");
    }
}



void csvsort(
    elona_vector1<std::string>& result,
    std::string line,
    int separator)
{
    elona_vector1<int> p_at_m40;
    p_at_m40(0) = 0;
    for (int cnt = 0; cnt < 40; ++cnt)
    {
        result(cnt) = "";
        getstr(result(cnt), line, p_at_m40(0), separator);
        if (strsize == 0)
        {
            break;
        }
        p_at_m40(0) += strsize;
    }
}



void lenfix(std::string& str, int length)
{
    int p_at_m89 = 0;
    p_at_m89 = length - strlen_u(str);
    if (p_at_m89 < 1)
    {
        p_at_m89 = 1;
    }
    for (int cnt = 0, cnt_end = (p_at_m89); cnt < cnt_end; ++cnt)
    {
        str += u8" "s;
    }
}



std::string fixtxt(const std::string& str, int length)
{
    std::string m_at_m104;
    m_at_m104 = ""s + str;
    if (strlen_u(str) < size_t(length))
    {
        while (1)
        {
            if (strlen_u(m_at_m104) >= size_t(length))
            {
                break;
            }
            m_at_m104 += u8" "s;
        }
    }
    else
    {
        m_at_m104 = ""s + strmid(str, 0, length);
    }
    return ""s + m_at_m104;
}



std::string getnpctxt(const std::string& tag, const std::string& default_text)
{
    int p_at_m189 = 0;
    p_at_m189 = instr(txtbuff, 0, tag);
    if (p_at_m189 == -1)
    {
        return default_text;
    }
    p_at_m189 += instr(txtbuff, p_at_m189, u8"\""s);
    if (p_at_m189 == -1)
    {
        return default_text;
    }
    return strmid(
        txtbuff,
        p_at_m189 + 1,
        clamp(instr(txtbuff, p_at_m189 + 1, u8"\""s), 0, 70));
}



std::string guildname()
{
    if (game_data.guild.belongs_to_mages_guild)
    {
        return i18n::s.get("core.guild.mages.name");
    }
    else if (game_data.guild.belongs_to_fighters_guild)
    {
        return i18n::s.get("core.guild.fighters.name");
    }
    else if (game_data.guild.belongs_to_thieves_guild)
    {
        return i18n::s.get("core.guild.thieves.name");
    }
    else
    {
        return i18n::s.get("core.guild.none.name");
    }
}



void initialize_rankn()
{
    SDIM4(rankn, 30, 11, 9);

    for (int category = 0; category < 9; category++)
    {
        if (category == 7)
        {
            // Skips the 7th row because there are no defined locale resources.
            continue;
        }
        for (int rank = 0; rank < 11; rank++)
        {
            rankn(rank, category) =
                i18n::s.get_enum("core.rank._"s + category, rank);
        }
    }

    DIM2(ranknorma, 9);
    ranknorma(0) = 20;
    ranknorma(1) = 60;
    ranknorma(2) = 45;
    ranknorma(6) = 30;
}



std::string ranktitle(int rank_id)
{
    int rank_value = game_data.ranks.at(rank_id) / 100;
    if (rank_value == 1)
    {
        return rankn(0, rank_id);
    }
    if (rank_value <= 5)
    {
        return rankn(1, rank_id);
    }
    if (rank_value <= 10)
    {
        return rankn(2, rank_id);
    }
    if (rank_value <= 80)
    {
        return rankn(rank_value / 15 + 3, rank_id);
    }
    return rankn(9, rank_id);
}



std::string txttargetlevel(
    const Character& base_chara,
    const Character& target_chara)
{
    const int x = base_chara.level;
    const int y = target_chara.level;

    int danger;
    if (x * 20 < y)
        danger = 10;
    else if (x * 10 < y)
        danger = 9;
    else if (x * 5 < y)
        danger = 8;
    else if (x * 3 < y)
        danger = 7;
    else if (x * 2 < y)
        danger = 6;
    else if (x * 3 / 2 < y)
        danger = 5;
    else if (x < y)
        danger = 4;
    else if (x / 3 * 2 < y)
        danger = 3;
    else if (x / 2 < y)
        danger = 2;
    else if (x / 3 < y)
        danger = 1;
    else
        danger = 0;

    return i18n::s.get_enum("core.action.target.level", danger, target_chara);
}



void txttargetnpc(int x, int y)
{
    int dy_ = 0;
    int i_ = 0;
    int p_ = 0;
    dy_ = 0;
    font(14 - en * 2);
    if (!fov_los(cdata.player().position, {x, y}) ||
        dist(cdata.player().position, x, y) >
            cdata.player().vision_distance / 2)
    {
        bmes(
            i18n::s.get("core.action.target.out_of_sight"),
            100,
            windowh - inf_verh - 45 - dy_ * 20);
        ++dy_;
        cansee = 0;
        return;
    }
    if (cell_data.at(x, y).chara_index_plus_one != 0)
    {
        i_ = cell_data.at(x, y).chara_index_plus_one - 1;
        if (cdata[i_].is_invisible() == 0 ||
            cdata.player().can_see_invisible() || cdata[i_].wet)
        {
            s = txttargetlevel(cdata.player(), cdata[i_]);
            bmes(s, 100, windowh - inf_verh - 45 - dy_ * 20);
            ++dy_;
            bmes(
                i18n::s.get(
                    "core.action.target.you_are_targeting",
                    cdata[i_],
                    dist(cdata.player().position, cdata[i_].position)),
                100,
                windowh - inf_verh - 45 - dy_ * 20);
            ++dy_;
        }
    }
    if (!cell_data.at(x, y).item_info_memory.is_empty())
    {
        bmes(txtitemoncell(x, y), 100, windowh - inf_verh - 45 - dy_ * 20);
        ++dy_;
    }
    if (cell_data.at(x, y).feats != 0)
    {
        if (map_data.type == mdata_t::MapType::world_map)
        {
            if (cell_data.at(x, y).feats / 1000 % 100 == 15)
            {
                p_ = cell_data.at(x, y).feats / 100000 % 100 +
                    cell_data.at(x, y).feats / 10000000 * 100;
                bmes(
                    mapname(p_, true), 100, windowh - inf_verh - 45 - dy_ * 20);
                ++dy_;
            }
            if (cell_data.at(x, y).feats / 1000 % 100 == 34)
            {
                bmes(
                    txtbuilding(
                        cell_data.at(x, y).feats / 100000 % 100,
                        cell_data.at(x, y).feats / 10000000),
                    100,
                    windowh - inf_verh - 45 - dy_ * 20);
                ++dy_;
            }
        }
    }
    cansee = 1;
}

} // namespace elona
