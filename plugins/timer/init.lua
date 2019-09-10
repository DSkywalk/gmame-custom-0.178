-- license:BSD-3-Clause
-- copyright-holders:Carl
require('lfs')
local exports = {}
exports.name = "timer"
exports.version = "0.0.1"
exports.description = "Game play timer"
exports.license = "The BSD 3-Clause License"
exports.author = { name = "Carl" }

local timer = exports

function timer.startplugin()
	local timer_path = "timer"
	local timer_started = false
	local total_time = 0
	local start_time = 0
	local play_count = 0

	local function get_filename()
		local path
		if emu.softname() ~= "" then
			path = timer_path .. '/' .. emu.romname() .. "_" .. emu.softname() .. ".time"
		else
			path = timer_path .. '/' .. emu.romname() .. ".time"
		end
		return path
	end

	local function save()
		total_time = total_time + (os.time() - start_time)
		os.remove(get_filename()) -- truncate file
		file = io.open(get_filename(), "w")
		if not file then
			lfs.mkdir(timer_path)
			file = io.open(get_filename(), "w")
		end
		if file then
			file:write(total_time .. "\n")
			file:write(play_count)
			file:close()
		end
	end


	emu.register_start(function()
		local file
		if timer_started then
			save()
		end
		timer_started = true
		local file = io.open(get_filename(), "r")
		if file then
			total_time = file:read("n")
			play_count = file:read("n")
			file:close()
		end
		if not play_count then
			play_count = 0
		end
		if not total_time then
			total_time = 0
		end
		start_time = os.time()
		play_count = play_count + 1
	end)

	emu.register_stop(function()
		timer_started = false
		save()
		total_time = 0
		play_count = 0
	end)

	local function sectohms(time)
		local hrs = math.floor(time / 3600)
		local min = math.floor((time % 3600) / 60)
		local sec = time % 60
		return string.format("%03d:%02d:%02d", hrs, min, sec)
	end

	local function menu_populate()
		local time = os.time() - start_time
		return {{ "Current time", "", "off" },
			{ sectohms(time), "", "off" },
			{ "Total time", "", "off" },
			{ sectohms(total_time + time), "", "off" },
			{ "Play Count", "", "off" },
			{ play_count, "", "off" }}
	end

	local function menu_callback(index, event)
		return true
	end

	emu.register_menu(menu_callback, menu_populate, "Timer")
end

return exports
