local Chara = ELONA.require("core.Chara")
local GUI = ELONA.require("core.GUI")
local Internal = ELONA.require("core.Internal")
local Item = ELONA.require("core.Item")

local common = require("../common.lua")

return {
   root = "core.talk.unique.miches",
   nodes = {
      __start = function()
         local flag = Internal.get_quest_flag("putit_attacks")
         if flag == 1000 then
            return "quest_completed"
         elseif flag == 0 then
            return "quest_ask"
         elseif flag == 1 then
            return "quest_waiting"
         elseif flag == 2 then
            return "quest_finish"
         end
         return "__END__"
      end,
      quest_completed = {
         text = {
            {"complete"}
         }
      },
      quest_ask = {
         text = {
            {"quest.dialog._0"},
            {"quest.dialog._1"}
         },
         choices = {
            {"quest_yes", "quest.choices.yes"},
            {"quest_no", "quest.choices.no"}
         },
         default_choice = "quest_no"
      },
      quest_yes = {
         text = {
            common.journal_updated,
            {"quest.yes"},
         },
         on_finish = function()
            Internal.set_quest_flag("putit_attacks", 1)
         end
      },
      quest_no = {
         text = {
            {"quest.no"},
         }
      },
      quest_waiting = {
         text = {
            {"quest.waiting"},
         }
      },
      quest_finish = {
         text = {
            function()
               Item.create(Chara.player().position, {id = "core.small_shield", level = 10, quality = "good"})
               Item.create(Chara.player().position, {id = "core.girdle", level = 10, quality = "good"})
               Item.create(Chara.player().position, "core.gold_piece", 3000)
               Item.create(Chara.player().position, "core.platinum_coin", 2)

               common.quest_completed()
            end,
            {"quest.end"},
         },
         on_finish = function()
            Internal.set_quest_flag("putit_attacks", 1000)
         end
      },
   }
}
