local Chara = ELONA.require("core.Chara")
local Data = ELONA.require("core.Data")
local Internal = ELONA.require("core.Internal")
local Rand = ELONA.require("core.Rand")
local GUI = ELONA.require("core.GUI")
local math = math
local I18N = ELONA.require("core.I18N")
local Item = ELONA.require("core.Item")
local World = ELONA.require("core.World")
local table = table
local string = string

local common = require("../common.lua")

local function start_trial()
    World.data.joining_fighters_guild = 1
    GUI.show_journal_update_message()
    World.data.fighters_guild_target = Internal.generate_fighters_guild_target(10)
    World.data.fighters_guild_quota = 15
end

local function join_guild()
    World.data.ranks[8] = 10000

    World.join_guild("fighters")

    GUI.txt(I18N.get("core.quest.completed"))
    GUI.play_sound("core.complete1")
    GUI.show_journal_update_message()
    GUI.txt(I18N.get("core.talk.unique.doria.nonmember.joined"), "orange")
end

local function move_self(t)
   t.speaker:move_to(29, 2)
   t.speaker.initial_position.x = 29
   t.speaker.initial_position.y = 2
end

local function update_quota()
   World.data.fighters_guild_target = Internal.generate_fighters_guild_target(Chara.player().level + 10)
   World.data.fighters_guild_quota = 2 + Rand.between(2, 5)
   World.data.fighters_guild_quota_recurring = true
   GUI.show_journal_update_message()
end

local function receive_reward()
   World.data.fighters_guild_quota_recurring = false
   Item.create(Chara.player().position, {objlv = 51 - World.data.ranks[8] // 200, quality = "good", flttypemajor = 10000})
   Item.create(Chara.player().position, "core.gold_piece", 10000 - World.data.ranks[8] + 1000)
   Item.create(Chara.player().position, "core.platinum_coin", math.clamp(4 - World.data.ranks[8] // 2500, 1, 4))

   common.quest_completed()

   World.modify_ranking(8, 500, 8)
end

local function quota_args()
   local id = Data.get_id_by_integer("core.chara", World.data.fighters_guild_target)
   local name = I18N.get_data_text("core.chara", id, "name")
   return {World.data.fighters_guild_quota, name}
end

return {
   root = "core.talk.unique.doria",
   nodes = {
      __start = function()
         if World.belongs_to_guild("fighters") == false then
            return "guild_nonmember"
         end

         return "guild_member"
      end,

      guild_nonmember = {
         text = {
            {"nonmember.dialog"},
         },
         choices = {
            {"guild_desc", "nonmember.choices.tell_me_about"},
            {"guild_join_check", "nonmember.choices.want_to_join"},
            {"__END__", "__BYE__"}
         }
      },
      guild_desc = {
         text = {
            {"nonmember.tell_me_about"},
         },
         choices = {
            {"__start", "__MORE__"},
         }
      },
      guild_join_check = function()
         if World.data.joining_fighters_guild == 0 then
            return "guild_join_start"
         elseif World.data.fighters_guild_quota > 0 then
            return "guild_join_waiting"
         end

         return "guild_join_finish"
      end,
      guild_join_start = {
         text = {
            {"nonmember.want_to_join._0"},
            start_trial,
            {"nonmember.want_to_join._1", args = quota_args},
         },
         choices = {
            {"__start", "__MORE__"},
         }
      },
      guild_join_waiting = {
         text = {
            {"nonmember.quota", args = quota_args},
         },
         choices = {
            {"__start", "__MORE__"},
         }
      },
      guild_join_finish = {
         text = {
            {"nonmember.end._0"},
            join_guild,
            {"nonmember.end._1"},
         },
         on_finish = move_self,
      },

      guild_member = {
         text = {
            move_self,
            {"member.dialog", args = function() return {World.ranking_title(8), Chara.player().basename} end},
         },
         choices = function()
            local choices = {}
            if not World.data.fighters_guild_quota_recurring then
               table.insert(choices, {"guild_quota_new", "core.talk.unique.lexus.member.choices.new_quota"})
            else
               table.insert(choices, {"guild_quota_check", "core.talk.unique.lexus.member.choices.report_quota"})
            end
            table.insert(choices, {"__END__", "__BYE__"})

            return choices
         end
      },
      guild_quota_new = {
         text = {
            update_quota,
            {"member.new_quota", args = quota_args},
         },
         choices = {
            {"__start", "__MORE__"},
         }
      },
      guild_quota_check = function()
         if World.data.fighters_guild_quota > 0 then
            return "guild_quota_waiting"
         end
         return "guild_quota_finish"
      end,
      guild_quota_waiting = {
         text = {
            {"core.talk.unique.lexus.member.report_quota.waiting"},
         },
         choices = {
            {"__start", "__MORE__"},
         }
      },
      guild_quota_finish = {
         text = {
            receive_reward,
            {"core.talk.unique.lexus.member.report_quota.end"},
         },
      },
   }
}
