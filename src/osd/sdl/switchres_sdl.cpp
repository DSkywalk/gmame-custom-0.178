/**************************************************************

   switchres_sdl.cpp - SDL OSD SwitchRes core routines

   ---------------------------------------------------------

   SwitchRes   Modeline generation engine for emulation

   GroovyMAME  Integration of SwitchRes into the MAME project
               Some reworked patches from SailorSat's CabMAME

   License     GPL-2.0+
   Copyright   2010-2016 - Chris Kennedy, Antonio Giner

 **************************************************************/

// SDL headers
#include "SDL_syswm.h"

// MAME headers
#include "osdepend.h"
#include "emu.h"
#include "emuopts.h"
#include "../../frontend/mame/mameopts.h"

// MAMEOS headers
#include "video.h"
#include "input.h"
#include "output.h"
#include "osdsdl.h"
#include "window.h"

#define XRANDR_ARGS ""
#define min(a,b)({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);_a < _b ? _a : _b; })

#define XRANDR_TIMING      0x00000020
extern int fd;

//============================================================
//  PROTOTYPES
//============================================================

bool switchres_init_osd(running_machine &machine);
bool switchres_modeline_setup(running_machine &machine);
bool switchres_modeline_remove(running_machine &machine);
bool switchres_modeline_reset(running_machine &machine);
bool switchres_resolution_change(sdl_window_info *window);
static bool add_custom_video_mode(modeline *mode, char *connector);
static bool set_custom_video_mode(modeline *mode, char *connector);
static int del_custom_video_mode(modeline *mode, char *connector);
static void set_option_osd(running_machine &machine, const char *option_ID, bool state);
static bool get_output(char *command, char *output);
int set_mame_focus(SDL_Window * window);

//============================================================
//  LOCAL VARIABLES
//============================================================

char desktop_res[32]={'\x00'};
int mode_count = 1;

//============================================================
//  switchres_init_osd
//============================================================

bool switchres_init_osd(running_machine &machine)
{
	config_settings *cs = &machine.switchres.cs;
	game_info *game = &machine.switchres.game;
	modeline *mode_table = machine.switchres.video_modes;
	modeline *user_mode = &machine.switchres.user_mode;
	monitor_range *range = machine.switchres.range;
	const char * aspect;
	char *connector = machine.switchres.cs.connector;
	char resolution[32]={'\x00'};
	char orientation[32]={'\x00'};
	char current_res[32]={'\x00'};

	sdl_options &options = downcast<sdl_options &>(machine.options());

	// Initialize structures and config settings
	memset(cs, 0, sizeof(struct config_settings));
	memset(game, 0, sizeof(struct game_info));

	// Init Switchres common info
	switchres_init(machine);

	// Complete config settings
	strcpy(resolution, options.resolution());
	cs->monitor_count = options.numscreens();

	// Get current resolution
	get_output((char*)"xrandr -q | grep '[0-9]\\*'", current_res);
	sscanf(current_res, " %s ", desktop_res);

	// Get current orientation
	if (get_output((char*)"xrandr -q | grep ' connected ' | awk '{print $4}' | head -1", orientation))
		if (!strcmp(orientation, "right") || !strcmp(orientation, "left"))
		{
			osd_printf_verbose("Switchres: desktop rotated %s\n", orientation);
			cs->desktop_rotated = 1;
		}

	// Get connector name
	if (!strcmp(cs->connector, "auto"))
	{
		if (get_output((char*)"xrandr -q | grep ' connected ' | awk '{print $1}' | head -1", connector))
			osd_printf_verbose("SwitchRes: Found output connector '%s'\n",	connector);
		else
			osd_printf_error("SwitchRes: Error getting connector with xrandr");
	}

	// Get per window resolution
	strcpy(resolution, strcmp(options.resolution(0), "auto")? options.resolution(0) : options.resolution());

	// Get monitor aspect
	aspect = strcmp(options.aspect(0), "auto")? options.aspect(0) : options.aspect();
	if (strcmp(aspect, "auto"))
	{
		float num, den;
		sscanf(aspect, "%f:%f", &num, &den);
		cs->monitor_aspect = cs->desktop_rotated? den/num : num/den;
	}
	else
		cs->monitor_aspect = STANDARD_CRT_ASPECT;

	// Create dummy mode table
	mode_table[1].width = mode_table[1].height = 1;
	mode_table[1].refresh = 60;
	mode_table[1].vfreq = mode_table[1].refresh;
	mode_table[1].hactive = mode_table[1].vactive = 1;
	mode_table[1].type = XYV_EDITABLE | XRANDR_TIMING | (cs->desktop_rotated? MODE_ROTATED : MODE_OK);

	if (user_mode->hactive)
	{
		user_mode->width = user_mode->hactive;
		user_mode->height = user_mode->vactive;
		user_mode->refresh = int(user_mode->refresh);
		user_mode->type = XRANDR_TIMING | MODE_USER_DEF | (cs->desktop_rotated? MODE_ROTATED : MODE_OK);
	}

	// Create automatic specs and force resolution for LCD monitors
	if (!strcmp(cs->monitor, "lcd"))
	{
		modeline current;
		memset(&current, 0, sizeof(struct modeline));

		osd_printf_verbose("SwitchRes: Creating automatic specs for LCD based on VESA GTF\n");
		sscanf(desktop_res, "%dx%d", &current.width, &current.height);
		current.refresh = 60;
		modeline_vesa_gtf(&current);
		modeline_to_monitor_range(range, &current);
		monitor_show_range(range);

		sprintf(resolution, "%dx%d@%d", current.width, current.height, current.refresh);
	}
	// Otherwise (non-LCD), convert the user defined modeline into a -resolution option
	else if (user_mode->hactive)
		sprintf(resolution, "%dx%d", user_mode->hactive, user_mode->vactive);

	// Get resolution from ini
	if (strcmp(resolution, "auto"))
	{
		osd_printf_verbose("SwitchRes: -resolution was set at command line or in .ini file as %s\n", resolution);

		if ((sscanf(resolution, "%dx%d@%d", &cs->width, &cs->height, &cs->refresh) < 3) &&
			((!strstr(resolution, "x") || (sscanf(resolution, "%dx%d", &cs->width, &cs->height) != 2))))
				osd_printf_info("SwitchRes: illegal -resolution value: %s\n", resolution);
		else
		{
			// Add the user's resolution to our table
			if (!user_mode->hactive)
			{
				mode_table[1].width = mode_table[1].hactive = cs->width? cs->width : 1;
				mode_table[1].height = mode_table[1].vactive = cs->height? cs->height : 1;
				mode_table[1].refresh = cs->refresh? int(cs->refresh) : 60;
				mode_table[1].vfreq = mode_table[1].refresh;
				mode_table[1].type |= MODE_USER_DEF;
				if (cs->width) mode_table[1].type &= ~X_RES_EDITABLE;
				if (cs->height) mode_table[1].type &= ~Y_RES_EDITABLE;
			}
		}
	}
	// Get game info
	switchres_get_game_info(machine);

	return true;
}

//============================================================
//  switchres_modeline_setup
//============================================================

bool switchres_modeline_setup(running_machine &machine)
{
	modeline *best_mode = &machine.switchres.best_mode;
	modeline *mode_table = machine.switchres.video_modes;
	char *connector = machine.switchres.cs.connector;
	sdl_options &options = downcast<sdl_options &>(machine.options());
	sdl_osd_interface &osd = downcast<sdl_osd_interface &>(machine.osd());
	std::string error_string;

	osd_printf_verbose("\nSwitchRes: Entering switchres_modeline_setup\n");

	// Find most suitable video mode and generate a modeline for it if we're allowed
	if (!switchres_get_video_mode(machine))
	{
		set_option_osd(machine, OSDOPTION_SWITCHRES, false);
		return false;
	}

	// Make the new modeline available to the system
	if (machine.options().modeline_generation())
	{
		// Lock mode before adding it to mode table
		best_mode->type |= MODE_DISABLED;

		// Check if the same mode had been created already
		int i;
		bool found = false;
		for (i = 2; i <= mode_count; i++)
			if (!memcmp(&mode_table[i], best_mode, sizeof(modeline) - sizeof(mode_result)))
				found = true;

		// Create the new mode and store it in our table
		if (!found)
		{
			mode_count++;
			memcpy(&mode_table[mode_count], best_mode, sizeof(modeline));
			add_custom_video_mode(best_mode, connector);
		}

		// Switch to the new mode
		set_custom_video_mode(best_mode, connector);
	}

	// Set MAME common options
	switchres_set_options(machine);

	// Black frame insertion / multithreading
	bool black_frame_insertion = options.black_frame_insertion() && best_mode->result.v_scale > 1 && best_mode->vfreq > 100;
	set_option_osd(machine, OPTION_BLACK_FRAME_INSERTION, black_frame_insertion);

	// Set MAME OSD specific options

	// Vertical synchronization management (autosync)
	// Disable -syncrefresh if our vfreq is scaled or out of syncrefresh_tolerance, or we don't have DRI access
	bool sync_refresh_effective = black_frame_insertion || !(fd == 0 || (best_mode->result.weight & R_V_FREQ_OFF) || best_mode->result.v_scale > 1);
	set_option_osd(machine, OPTION_SYNCREFRESH, options.autosync()? sync_refresh_effective : options.sync_refresh());
	set_option_osd(machine, OSDOPTION_WAITVSYNC, options.autosync()? sync_refresh_effective : options.wait_vsync());

	// Set scaling/stretching options
	set_option_osd(machine, OPTION_KEEPASPECT, true);
	set_option_osd(machine, OPTION_UNEVENSTRETCH, best_mode->result.weight & R_RES_STRETCH);
	set_option_osd(machine, OPTION_UNEVENSTRETCHX, (!(best_mode->result.weight & R_RES_STRETCH) && (best_mode->width >= options.super_width())));
	set_option_osd(machine, OSDOPTION_FILTER, ((best_mode->result.weight & R_RES_STRETCH || best_mode->interlace)));

	// Refresh video options
	osd.extract_video_config();

	return true;
}

//============================================================
//  switchres_modeline_remove
//============================================================

bool switchres_modeline_remove(running_machine &machine)
{
	sdl_options &options = downcast<sdl_options &>(machine.options());

	// Reset SDL options
	switchres_reset_options(machine);
	options.revert(OPTION_PRIORITY_SWITCHRES);

	return true;
}

//============================================================
//  switchres_modeline_reset
//============================================================

bool switchres_modeline_reset(running_machine &machine)
{
	config_settings *cs = &machine.switchres.cs;
	modeline *mode_table = machine.switchres.video_modes;
	char cmd[256]={'\x00'};

	osd_printf_verbose("SwitchRes: Restoring desktop resolution: %s\n", desktop_res);
	sprintf(cmd, "xrandr --output %s --mode %s", cs->connector,  desktop_res);
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	// Remove modelines
	while (mode_count > 1)
	{
		del_custom_video_mode(&mode_table[mode_count], cs->connector);
		mode_count--;
	}

	return true;
}

//============================================================
//  switchres_resolution_change
//============================================================

bool switchres_resolution_change(sdl_window_info *window)
{
	running_machine &machine = window->machine();
	modeline *best_mode = &machine.switchres.best_mode;
	modeline previous_mode;

	// If there's no pending change, just exit
	if (!switchres_check_resolution_change(machine))
		return false;

	// Get the new resolution
	previous_mode = *best_mode;
	switchres_modeline_setup(machine);

	// Only change resolution if the new one is actually different
	if (memcmp(&previous_mode, best_mode, offsetof(modeline, result)))
		return true;

	return false;
}

//============================================================
//  add_custom_video_mode
//============================================================

static bool add_custom_video_mode(modeline *mode, char *connector)
{
	char modeline[256]={'\x00'};
	char cmd[512]={'\x00'};

	if (!mode)
		return false;

	// Add new modeline
	sprintf(cmd, "xrandr %s --newmode %s", XRANDR_ARGS, modeline_print(mode, modeline, MS_LABEL_SDL | MS_PARAMS));
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	// Add modeline to interface
	sprintf(cmd, "xrandr %s --addmode %s %s", XRANDR_ARGS, connector, modeline_print(mode, modeline, MS_LABEL_SDL));
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	return true;
}

//============================================================
//  set_custom_video_mode
//============================================================

static bool set_custom_video_mode(modeline *mode, char *connector)
{
	char modeline[256]={'\x00'};
	char cmd[512]={'\x00'};

	// Use xrandr to switch to new mode. SDL_SetVideoMode doesn't work when (new_width, new_height)==(old_width, old_height)
	sprintf(cmd, "xrandr %s --output %s --mode %s", XRANDR_ARGS, connector, modeline_print(mode, modeline, MS_LABEL_SDL));
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	return true;
}

//============================================================
//  del_custom_video_mode
//============================================================

static int del_custom_video_mode(modeline *mode, char *connector)
{
	char modeline[256]={'\x00'};
	char cmd[512]={'\x00'};

	if (!mode)
		return false;

	// Delete modeline from interface
	sprintf(cmd, "xrandr %s --delmode %s %s", XRANDR_ARGS, connector, modeline_print(mode, modeline, MS_LABEL_SDL));
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	// Remove modeline
	sprintf(cmd, "xrandr %s --rmmode %s", XRANDR_ARGS, modeline_print(mode, modeline, MS_LABEL_SDL));
	osd_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

	return true;
}

//============================================================
//  set_option_osd - option setting wrapper
//============================================================

static void set_option_osd(running_machine &machine, const char *option_ID, bool state)
{
	sdl_options &options = downcast<sdl_options &>(machine.options());
	std::string error_string;

	options.set_value(option_ID, state, OPTION_PRIORITY_SWITCHRES, error_string);
	osd_printf_verbose("SwitchRes: Setting option -%s%s\n", machine.options().bool_value(option_ID)?"":"no", option_ID);
}

//============================================================
//  get_output
//============================================================

static bool get_output(char *command, char *output)
{
	FILE *pi;
	pi = popen(command, "r");

	if (pi != NULL)
	{
		char c;
		int i = 0;
		c = fgetc(pi);
		while(c != '\n' && i < 255)
		{
			output[i++] = c;
			c = fgetc(pi);
		}
		output[i] = '\0';
		pclose(pi);
		return true;

	}
	else
		return false;
}

//============================================================
//  set_mame_focus
//============================================================

int set_mame_focus(SDL_Window * window)
{
#ifdef SDLMAME_X11
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (strncmp(SDL_GetCurrentVideoDriver(),"x11",3)==0) 
	{
		if (SDL_GetWindowWMInfo(window,&info))
		{
			Window focus;
			int revert = RevertToNone;
			XGetInputFocus(info.info.x11.display, &focus, &revert);

			if ( focus != info.info.x11.window )
			{
				osd_printf_verbose("SwitchRes: possible loss of FocusIn event, force it to grab keyboard.\n");
				XSetInputFocus(info.info.x11.display, info.info.x11.window, RevertToNone, CurrentTime);
				XFlush(info.info.x11.display);
				XGetInputFocus(info.info.x11.display, &focus, &revert);
				osd_printf_verbose("SwitchRes: window handle is 0x%06x\n", (unsigned)focus);
			}
	}
	else
	{
		osd_printf_error("SwitchRes: error getting SDL window information.\n");
		return 1;
	}
}
#endif
	return 0;
}

