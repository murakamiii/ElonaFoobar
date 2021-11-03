#include "ui_menu_message_log.hpp"

#include "../audio.hpp"
#include "../draw.hpp"
#include "../i18n.hpp"
#include "../message_log.hpp"



namespace
{

void _draw_window()
{
    constexpr auto chunk_width = 192;

    const auto log_window_width = windoww - inf_msgx;
    const auto p = log_window_width / chunk_width;

    window2(wx, wy, ww, wh, 1, -1);

    for (int dy = 0; dy < inf_maxlog - 3; ++dy)
    {
        for (int dx = 0; dx < p + 1; ++dx)
        {
            const auto x =
                dx == p ? log_window_width % chunk_width : chunk_width;
            draw_region(
                "message_window_contents",
                dx * chunk_width + inf_msgx,
                inf_msgy - (dy + 1) * inf_msgspace,
                0,
                dy % 4 * inf_msgspace,
                x,
                inf_msgspace);
        }
    }

    for (int dx = 0; dx < p + 1; ++dx)
    {
        const auto x = dx == p ? log_window_width % chunk_width : chunk_width;
        draw_region(
            "message_window_border", dx * chunk_width + inf_msgx, inf_msgy, x);
    }
}



void _draw_single_message(size_t cnt, int message_offset)
{
    const auto n = message_log.line_size();
    if (n == 0)
    {
        return;
    }
    if (static_cast<int>(n) + message_offset < static_cast<int>(cnt) + 5)
    {
        return;
    }

    int message_width = 0;
    font(14 - en * 2);
    for (const auto& span : message_log.at(n - cnt - 5 + message_offset))
    {
        mes(message_width * 7 + inf_msgx + 6,
            inf_msgy - (cnt + 1) * inf_msgspace + vfix - 3,
            span.text,
            span.color);

        message_width += strlen_u(span.text);
    }
}



void _draw_messages(int message_offset)
{
    for (int cnt = 0; cnt < inf_maxlog - 4; ++cnt)
    {
        _draw_single_message(cnt, message_offset);
    }
}

} // namespace



namespace elona
{
namespace ui
{

bool UIMenuMessageLog::init()
{

    windowshadow = 1;
    key_list(0) = keybind_get_bound_key_name("enter");
    keyrange = 0;
    pagesize = 0;
    wx = inf_msgx - 2;
    wy = inf_msgy - (inf_maxlog - 3) * inf_msgspace - 1;
    ww = windoww - inf_msgx + 6;
    wh = (inf_maxlog - 3) * inf_msgspace;
    snd("core.log");
    show_title(i18n::s.get("core.ui.message.key_hint"));
    window_animation_corner(wx, wy, ww, wh, 8, 4);

    return true;
}



void UIMenuMessageLog::update()
{
}



void UIMenuMessageLog::draw()
{
    _draw_window();
    _draw_messages(message_offset);
}



optional<UIMenuMessageLog::ResultType> UIMenuMessageLog::on_key(
    const std::string& action)
{
    if (action == "north")
    {
        _scroll_by(-1);
        return none;
    }
    else if (action == "south")
    {
        _scroll_by(1);
        return none;
    }
    else if (action == "previous_page")
    {
        _scroll_by(-(inf_maxlog - 4));
        return none;
    }
    else if (action == "next_page")
    {
        _scroll_by(inf_maxlog - 4);
        return none;
    }
    else if (action != ""s)
    {
        update_screen();
        return UIMenuMessageLog::Result::finish();
    }

    return none;
}



void UIMenuMessageLog::_scroll_by(int lines)
{
    const auto min = 4 - static_cast<int>(message_log.line_size());
    const auto max = 0;

    message_offset += lines;
    if (message_offset < min)
    {
        message_offset = min;
    }
    if (max < message_offset)
    {
        message_offset = max;
    }
}

} // namespace ui
} // namespace elona
