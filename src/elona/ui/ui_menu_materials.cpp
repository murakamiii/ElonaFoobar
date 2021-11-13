#include "ui_menu_materials.hpp"

#include "../audio.hpp"
#include "../data/types/type_asset.hpp"

namespace elona
{
namespace ui
{

static void _load_materials_list()
{
    for (int cnt = 0; cnt < 400; ++cnt)
    {
        if (mat(cnt) != 0)
        {
            list(0, listmax) = cnt;
            ++listmax;
        }
    }
}

bool UIMenuMaterials::init()
{
    listmax = 0;
    page = 0;
    pagesize = 15;
    cs = 0;
    cs_bk = -1;

    const auto& info = asset_load("core.ie_scroll");
    gsel(0);
    snd("core.scroll");
    wx = (windoww - info.width) / 2 + inf_screenx;
    wy = winposy(info.height + 50);
    ww = info.width;
    wh = info.height + 50;
    window_animation(wx, wy, ww, wh, 9, 4);
    windowshadow = 1;

    _load_materials_list();

    return true;
}

void UIMenuMaterials::update()
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

void UIMenuMaterials::_draw_window()
{
    std::string hints = strhint2 + strhint3b;
    showscroll(hints, wx, wy, ww, wh);
    display_topic(i18n::s.get("core.ui.material.name"), wx + 38, wy + 36);
    display_topic(i18n::s.get("core.ui.material.detail"), wx + 296, wy + 36);
}

void UIMenuMaterials::_draw_key(int cnt)
{
    if (cnt % 2 == 0)
    {
        boxf(wx + 70, wy + 66 + cnt * 19, 490, 18, {12, 14, 16, 16});
    }
    display_key(wx + 68, wy + 66 + cnt * 19 - 2, cnt);
}

void UIMenuMaterials::_draw_keys()
{
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

void UIMenuMaterials::_draw_single_list_entry_name(int cnt, int list_item)
{
    std::string mat_name = ""s + matname(list_item) + " " +
        i18n::s.get("core.crafting.menu.x") + " " + mat(list_item);
    cs_list(cs == cnt, mat_name, wx + 96, wy + 66 + cnt * 19 - 1);
}

void UIMenuMaterials::_draw_single_list_entry_desc(int cnt, int list_item)
{
    std::string mat_desc = matdesc(list_item);
    mes(wx + 308, wy + 66 + cnt * 19 + 2, mat_desc);
}

void UIMenuMaterials::_draw_single_list_entry(int cnt, int list_item)
{
    _draw_single_list_entry_name(cnt, list_item);
    _draw_single_list_entry_desc(cnt, list_item);

    draw_item_material(matref(2, list_item), wx + 47, wy + 69 + cnt * 19 + 2);
}

void UIMenuMaterials::_draw_list_entries()
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
        _draw_single_list_entry(cnt, list_item);
    }
    if (keyrange != 0)
    {
        cs_bk = cs;
    }
}

void UIMenuMaterials::draw()
{
    _draw_window();
    _draw_keys();
    _draw_list_entries();
}

optional<UIMenuMaterials::ResultType> UIMenuMaterials::on_key(
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
        return UIMenuMaterials::Result::cancel();
    }

    return none;
}

} // namespace ui
} // namespace elona
