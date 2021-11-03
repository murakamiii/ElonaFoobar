#include "ui_menu_charamake_attributes.hpp"

#include "../ability.hpp"
#include "../audio.hpp"
#include "../character_making.hpp"
#include "../class.hpp"
#include "../data/types/type_ability.hpp"
#include "../draw.hpp"
#include "../i18n.hpp"
#include "../menu.hpp"
#include "../race.hpp"

namespace elona
{
namespace ui
{

bool UIMenuCharamakeAttributes::init()
{
    return true;
}

void UIMenuCharamakeAttributes::_reroll_attributes()
{
    chara_delete(cdata.player());
    race_init_chara(cdata.player(), _race_id);
    class_init_chara(cdata.player(), _class_id);
    cdata.player().level = 1;
    for (int cnt = 10; cnt < 18; ++cnt)
    {
        if (_locked_attributes(cnt - 10) == 0)
        {
            if (_minimum)
            {
                cdata.player().get_skill(cnt).base_level -=
                    cdata.player().get_skill(cnt).base_level / 2;
            }
            else
            {
                cdata.player().get_skill(cnt).base_level -=
                    rnd(cdata.player().get_skill(cnt).base_level / 2 + 1);
            }
            _attributes(cnt - 10) =
                cdata.player().get_skill(cnt).base_level * 1000000 +
                cdata.player().get_skill(cnt).experience * 1000 +
                cdata.player().get_skill(cnt).potential;
        }
    }
}

static void _load_attributes_list(elona_vector1<int>& _attributes)
{
    listmax = 0;
    list(0, 0) = 0;
    listn(0, 0) = i18n::s.get("core.chara_making.common.reroll");
    ++listmax;
    list(0, 1) = 0;
    listn(0, 1) = i18n::s.get("core.chara_making.roll_attributes.proceed");
    ++listmax;
    for (int cnt = 10; cnt < 18; ++cnt)
    {
        list(0, listmax) = _attributes(cnt - 10);
        listn(0, listmax) = the_ability_db.get_text(cnt, "name");
        ++listmax;
    }
}

void UIMenuCharamakeAttributes::update()
{
    cs = 0;
    cs_bk = -1;
    pagesize = 0;

    character_making_draw_background(
        "core.chara_making.roll_attributes.caption");

    _reroll_attributes();
    _minimum = false;

    _load_attributes_list(_attributes);

    windowshadow = 1;
}

void UIMenuCharamakeAttributes::_draw_window_background()
{
    ui_display_window(
        i18n::s.get("core.chara_making.roll_attributes.attribute_reroll"),
        strhint3b + keybind_get_bound_key_name("switch_mode_2") + " [" +
            i18n::s.get("core.chara_making.roll_attributes.min_roll") + "]",
        (windoww - 360) / 2 + inf_screenx,
        winposy(352, 1) - 20,
        360,
        352);
}

void UIMenuCharamakeAttributes::_draw_window_topic()
{
    x = 150;
    y = 240;
    gmode(2, 30);
    gcopy_c(2, 0, 0, 180, 300, wx + 85, wy + wh / 2, x, y);
    gmode(2);
    display_topic(
        i18n::s.get("core.chara_making.roll_attributes.title"),
        wx + 28,
        wy + 30);
}

void UIMenuCharamakeAttributes::_draw_window_desc(int locks_left)
{
    font(12 + sizefix - en * 2);
    mes(wx + 175,
        wy + 52,
        i18n::s.get("core.chara_making.roll_attributes.locked_items_desc"));
    font(13 - en * 2, snail::Font::Style::bold);
    mes(wx + 180,
        wy + 84,
        i18n::s.get("core.chara_making.roll_attributes.locks_left") + u8": "s +
            locks_left);
}

void UIMenuCharamakeAttributes::_draw_window(int locks_left)
{
    _draw_window_background();
    _draw_window_topic();
    _draw_window_desc(locks_left);
}

void UIMenuCharamakeAttributes::_draw_attribute_locked(int cnt)
{
    font(12 - en * 2, snail::Font::Style::bold);
    mes(wx + 240, wy + 66 + cnt * 23 + 2, u8"Locked!"s, {20, 20, 140});
}

void UIMenuCharamakeAttributes::_draw_attribute_value(
    int cnt,
    int list_value,
    bool is_locked)
{
    // Copy image from item sheet.
    // TODO: migrate to PicLoader
    gmode(2);
    gcopy_c(
        1,
        (cnt - 2) * inf_tiles,
        672,
        inf_tiles,
        inf_tiles,
        wx + 198,
        wy + 76 + cnt * 23);

    mes(wx + 210, wy + 66 + cnt * 23, ""s + list_value / 1000000);

    if (is_locked)
    {
        _draw_attribute_locked(cnt);
    }
}

void UIMenuCharamakeAttributes::_draw_attribute(
    int cnt,
    int list_value,
    const std::string& text,
    bool is_locked)
{
    display_key(wx + 38, wy + 66 + cnt * 23 - 2, cnt);

    font(14 - en * 2);
    cs_list(cs == cnt, text, wx + 64, wy + 66 + cnt * 23 - 1);

    font(15 - en * 2, snail::Font::Style::bold);
    if (cnt >= 2)
    {
        _draw_attribute_value(cnt, list_value, is_locked);
    }
}

void UIMenuCharamakeAttributes::draw()
{
    int locks_left = _locked_attributes(8);
    _draw_window(locks_left);

    for (int cnt = 0; cnt < 10; ++cnt)
    {
        key_list(cnt) = key_select(cnt);
        keyrange = cnt + 1;

        int list_value = list(0, cnt);
        const std::string& text = listn(0, cnt);
        bool is_locked = cnt < 2 ? false : _locked_attributes(cnt - 2) == 1;
        _draw_attribute(cnt, list_value, text, is_locked);
    }
    cs_bk = cs;
}

optional<UIMenuCharamakeAttributes::ResultType>
UIMenuCharamakeAttributes::on_key(const std::string& action)
{
    if (auto selected = get_selected_index_this_page())
    {
        if (*selected == 0)
        {
            snd("core.dice");
            set_reupdate();
            return none;
        }
        if (*selected == 1)
        {
            std::vector<int> ret;
            for (size_t i = 0; i < _attributes.size(); ++i)
            {
                ret.push_back(_attributes(i));
            }
            return UIMenuCharamakeAttributes::Result::finish(ret);
        }
        if (_locked_attributes(*selected - 2) != 0)
        {
            ++_locked_attributes(8);
            _locked_attributes(*selected - 2) = 0;
        }
        else if (_locked_attributes(8) > 0)
        {
            _locked_attributes(*selected - 2) = 1;
            --_locked_attributes(8);
        }
        snd("core.ok1");
    }
    else if (action == "switch_mode_2")
    {
        _minimum = true;
        snd("core.dice");
        set_reupdate();
        return none;
    }
    else if (action == "cancel")
    {
        return UIMenuCharamakeAttributes::Result::cancel();
    }
    else if (getkey(snail::Key::f1))
    {
        show_game_help();
        return UIMenuCharamakeAttributes::Result::finish();
    }

    return none;
}

} // namespace ui
} // namespace elona
