#include "ui_menu_town_politics.hpp"

#include "../area.hpp"
#include "../audio.hpp"
#include "../draw.hpp"
#include "../i18n.hpp"
#include "../map.hpp"

namespace elona
{
namespace ui
{

static void _load_politics_list(bool is_town)
{
    int city = area_data[game_data.current_map].quest_town_id;

    // TODO: untranslated
    listmax = 0;
    page = 0;
    pagesize = 13;
    cs = 0;
    cs_bk = -1;

    list(0, listmax) = 1;
    listn(0, listmax) = i18n::s.get(
        "core.ui.politics.name", mapname(game_data.politics_map_id));
    ++listmax;

    if (is_town)
    {
        list(0, listmax) = 0;
        listn(0, listmax) =
            i18n::s.get("core.ui.politics.taxes", podata(150, city));
        ++listmax;
        p = rnd(1000);
        list(0, listmax) = 0;
        listn(0, listmax) =
            i18n::s.get("core.ui.politics.well_pollution", p(0));
        ++listmax;
        list(0, listmax) = 0;
        listn(0, listmax) = i18n::s.get("core.ui.politics.lawless");
        ++listmax;
    }
}

bool UIMenuTownPolitics::init()
{
    bool is_town = map_data.type == mdata_t::MapType::town;
    _load_politics_list(is_town);

    asset_load("core.deco_politics");
    gsel(0);
    draw_bg("core.deco_politics_a");
    render_hud();
    asset_load("core.ie_scroll");
    gsel(0);
    windowshadow = 1;
    snd("core.scroll");
    ww = 480;
    wh = 400;
    wx = (windoww - ww) / 2 + inf_screenx;
    wy = winposy(wh);

    return true;
}

void UIMenuTownPolitics::update()
{
    cs_bk = -1;
    pagemax = (listmax - 1) / pagesize;
    if (page < 0)
    {
        page = pagemax;
    }
    else if (page > pagemax)
    {
        page = 0;
    }
}

void UIMenuTownPolitics::_draw_window()
{
    std::string hint = strhint2 + strhint3b;
    showscroll(hint, wx, wy, ww, wh);
    display_topic(i18n::s.get("core.ui.politics.law"), wx + 65, wy + 45);

    font(12 + sizefix - en * 2);
    mes(wx + 185, wy + 52, i18n::s.get("core.ui.politics.global"));

    if (map_data.type == mdata_t::MapType::town)
    {
        mes(wx + 285,
            wy + 52,
            i18n::s.get(
                "core.ui.politics.law_of", mapname(game_data.current_map)));
    }

    gmode(2);
    draw_indexed("core.politics_law", wx + 155, wy + 46, 1);
    gmode(2);
    draw_indexed("core.politics_law", wx + 255, wy + 46, 0);
}

void UIMenuTownPolitics::_draw_key(int cnt)
{
    if (cnt % 2 == 0)
    {
        boxf(wx + 74, wy + 76 + cnt * 19, 365, 18, {12, 14, 16, 16});
    }
    display_key(wx + 72, wy + 76 + cnt * 19 - 2, cnt);
}

void UIMenuTownPolitics::_draw_keys()
{
    font(14 - en * 2);
    keyrange = 0;
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        p = pagesize * page + cnt;
        if (p >= listmax)
        {
            break;
        }
        key_list(cnt) = key_select(cnt);
        ++keyrange;

        _draw_key(cnt);
    }
}

static void
_draw_single_list_entry(int cnt, int list_item, const std::string& text)
{
    cs_list(cs == cnt, text, wx + 100, wy + 76 + cnt * 19 - 1);
    gmode(2);
    draw_indexed(
        "core.politics_law", wx + 42, wy + 68 + cnt * 19 + 2, list_item);
}

void UIMenuTownPolitics::_draw_list_entries()
{
    font(14 - en * 2);
    cs_listbk();
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        int index = pagesize * page + cnt;
        if (index >= listmax)
        {
            break;
        }

        int list_item = list(0, index);
        const std::string& text = listn(0, index);
        _draw_single_list_entry(cnt, list_item, text);
    }
    if (keyrange != 0)
    {
        cs_bk = cs;
    }
}

void UIMenuTownPolitics::draw()
{
    _draw_window();
    _draw_keys();
    _draw_list_entries();
}

optional<UIMenuTownPolitics::ResultType> UIMenuTownPolitics::on_key(
    const std::string& action)
{
    if (action == "next_page")
    {
        if (pagemax != 0)
        {
            snd("core.pop1");
            ++page;
            set_reupdate();
        }
    }
    else if (action == "previous_page")
    {
        if (pagemax != 0)
        {
            snd("core.pop1");
            --page;
            set_reupdate();
        }
    }
    else if (action == "cancel")
    {
        update_screen();
        return UIMenuTownPolitics::Result::cancel();
    }

    return none;
}

} // namespace ui
} // namespace elona
