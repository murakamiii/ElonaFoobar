local jarowinkler = require("kernel.ush.jarowinkler")
local parse = require("kernel.ush.parse")



local _did_you_mean_threshold = 0.8



local function _normalize_command_name(s)
   return s:lower():gsub("-", "_")
end



local Executor = {}



function Executor.new(env)
   local self = {
      _env = env,
   }
   setmetatable(self, {__index = Executor})
   return self
end



function Executor:_look_up_command(command_name)
   -- special alias for "help"
   if command_name == "?" then
      command_name = "help"
   else
      command_name = _normalize_command_name(command_name)
   end
   local is_prefixed = command_name:find("%.")

   local commands = {}
   for mod_id, command_table in pairs(self._env.COMMANDS) do
      for name, command in pairs(command_table) do
         local full_name = mod_id.."."..name
         if is_prefixed then
            if full_name == command_name then
               return command -- perfect match; return it immediately.
            end
         else
            if name == command_name then
               -- partial match; add it to candidates.
               commands[#commands+1] = {
                  mod_id = mod_id,
                  name = name,
                  full_name = full_name,
                  command = command,
               }
               break
            end
         end
      end
   end

   if #commands == 1 then
      return commands[1].command
   end

   if #commands > 1 then
      if #commands == 2 then
         -- If one of the two is built-in command, the other overrides it and is executed.
         if commands[1].mod_id == "_builtin_" then return commands[2].command end
         if commands[2].mod_id == "_builtin_" then return commands[1].command end
         -- If one of the two is user-defined command, it overrides the other and is executed.
         if commands[1].mod_id == "_console_" then return commands[1].command end
         if commands[2].mod_id == "_console_" then return commands[2].command end
      end

      -- Ambiguous invocation.
      self._env.TERM.println("(ush) command invocation '"..name.."' is ambiguous:")
      table.sort(commands, function(a, b) return a.full_name < b.full_name end)
      for _, cmd in ipairs(commands) do
         self._env.TERM.println("  "..cmd.full_name)
      end

      return nil -- Ambiguous call
   end

   assert(#commands == 0)

   -- Do "Did you mean?" suggestion.
   local did_you_mean = {}
   local calculator = jarowinkler.DefaultCalculator.new()
   for mod_id, command_table in pairs(self._env.COMMANDS) do
      for name, _ in pairs(command_table) do
         local full_name = mod_id.."."..name
         local distance = calculator:distance(command_name, is_prefixed and full_name or name)
         if distance >= _did_you_mean_threshold then
            did_you_mean[#did_you_mean+1] = {distance = distance, command_name = full_name}
         end
      end
   end
   table.sort(did_you_mean, function(a, b) return a.distance < b.distance end)

   self._env.TERM.println("(ush) command not found: "..command_name)
   if #did_you_mean ~= 0 then
      if #did_you_mean == 1 then
         self._env.TERM.println("Did you mean '"..did_you_mean[1].command_name.."'?")
      else
         self._env.TERM.println("Did you mean any of these command?")
         for _, cmd in ipairs(did_you_mean) do
            self._env.TERM.println("  "..cmd.command_name)
         end
      end
   end

   return nil -- Not found
end



-- :: string -> List<any> -> (boolean, any)
function Executor:execute(command_name, ...)
   local command = self:_look_up_command(command_name)
   if not command then
      return nil -- not found
   end

   local args = {...}
   -- prelude.print("RUN  "..command_name.."  "..table.concat(args, " "))
   return pcall(command, table.unpack(args))
end



-- :: string -> Env -> (boolean, any)
return function(input, env)
   local exe = Executor.new(env)
   local ok, result = pcall(parse, input)
   if not ok then
      return false, result
   end
   if not result then -- no input
      return true, nil
   end
   return exe:execute(table.unpack(result))
end
