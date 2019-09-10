// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Aaron Giles
/***************************************************************************

    config.c

    Configuration file I/O.
***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "drivenum.h"
#include "config.h"
#include "xmlfile.h"

#define DEBUG_CONFIG        0

//**************************************************************************
//  CONFIGURATION MANAGER
//**************************************************************************

//-------------------------------------------------
//  configuration_manager - constructor
//-------------------------------------------------


configuration_manager::configuration_manager(running_machine &machine)
	: m_machine(machine)
{
}

/*************************************
 *
 *  Register to be involved in config
 *  save/load
 *
 *************************************/

void configuration_manager::config_register(const char* nodename, config_saveload_delegate load, config_saveload_delegate save)
{
	config_element element;
	element.name = nodename;
	element.load = load;
	element.save = save;

	m_typelist.push_back(element);
}



/*************************************
 *
 *  Settings save/load frontend
 *
 *************************************/

int configuration_manager::load_settings()
{
	const char *controller = machine().options().ctrlr();
	int loaded = 0;

	/* loop over all registrants and call their init function */
	for (auto type : m_typelist)
		type.load(config_type::CONFIG_TYPE_INIT, nullptr);

	/* now load the controller file */
	if (controller[0] != 0)
	{
		/* open the config file */
		emu_file file(machine().options().ctrlr_path(), OPEN_FLAG_READ);
		osd_file::error filerr = file.open(controller, ".cfg");

		if (filerr != osd_file::error::NONE)
			throw emu_fatalerror("Could not load controller file %s.cfg", controller);

		/* load the XML */
		if (!load_xml(file, config_type::CONFIG_TYPE_CONTROLLER))
			throw emu_fatalerror("Could not load controller file %s.cfg", controller);
	}

	/* next load the defaults file */
	emu_file file(machine().options().cfg_directory(), OPEN_FLAG_READ);
	osd_file::error filerr = file.open("default.cfg");
	if (filerr == osd_file::error::NONE)
		load_xml(file, config_type::CONFIG_TYPE_DEFAULT);

	/* ages custom dips loading - before read game cfg */
	if (! machine().options().customs_forced())
		custom_settings();

	/* finally, load the game-specific file */
	filerr = file.open(machine().basename(), ".cfg");
	if (filerr == osd_file::error::NONE)
		loaded = load_xml(file, config_type::CONFIG_TYPE_GAME);

	/* ages custom dips - after read game cfg */
	if (machine().options().customs_forced())
		custom_settings();

	/* loop over all registrants and call their final function */
	for (auto type : m_typelist)
		type.load(config_type::CONFIG_TYPE_FINAL, nullptr);

	/* if we didn't find a saved config, return 0 so the main core knows that it */
	/* is the first time the game is run and it should diplay the disclaimer. */
	return loaded;
}


void configuration_manager::save_settings()
{
	/* loop over all registrants and call their init function */
	for (auto type : m_typelist)
		type.save(config_type::CONFIG_TYPE_INIT, nullptr);

	/* save the defaults file */
	emu_file file(machine().options().cfg_directory(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
	osd_file::error filerr = file.open("default.cfg");
	if (filerr == osd_file::error::NONE)
		save_xml(file, config_type::CONFIG_TYPE_DEFAULT);

	/* finally, save the game-specific file */
	filerr = file.open(machine().basename(), ".cfg");
	if (filerr == osd_file::error::NONE)
		save_xml(file, config_type::CONFIG_TYPE_GAME);

	/* loop over all registrants and call their final function */
	for (auto type : m_typelist)
		type.save(config_type::CONFIG_TYPE_FINAL, nullptr);
}



/*************************************
 *
 *  XML file load
 *
 *************************************/

int configuration_manager::load_xml(emu_file &file, config_type which_type)
{
	xml_data_node *root, *confignode, *systemnode;
	const char *srcfile;
	int version, count;

	/* read the file */
	root = xml_file_read(file, nullptr);
	if (!root)
		goto error;

	/* find the config node */
	confignode = xml_get_sibling(root->child, "mameconfig");
	if (!confignode)
		goto error;

	/* validate the config data version */
	version = xml_get_attribute_int(confignode, "version", 0);
	if (version != CONFIG_VERSION)
		goto error;

	/* strip off all the path crap from the source filename */
	srcfile = strrchr(machine().system().source_file, '/');
	if (!srcfile)
		srcfile = strrchr(machine().system().source_file, '\\');
	if (!srcfile)
		srcfile = strrchr(machine().system().source_file, ':');
	if (!srcfile)
		srcfile = machine().system().source_file;
	else
		srcfile++;

	/* loop over all system nodes in the file */
	count = 0;
	for (systemnode = xml_get_sibling(confignode->child, "system"); systemnode; systemnode = xml_get_sibling(systemnode->next, "system"))
	{
		/* look up the name of the system here; skip if none */
		const char *name = xml_get_attribute_string(systemnode, "name", "");

		/* based on the file type, determine whether we have a match */
		switch (which_type)
		{
			case config_type::CONFIG_TYPE_GAME:
				/* only match on the specific game name */
				if (strcmp(name, machine().system().name) != 0)
					continue;
				break;

			case config_type::CONFIG_TYPE_DEFAULT:
				/* only match on default */
				if (strcmp(name, "default") != 0)
					continue;
				break;

			case config_type::CONFIG_TYPE_CONTROLLER:
			{
				int clone_of;
				/* match on: default, game name, source file name, parent name, grandparent name */
				if (strcmp(name, "default") != 0 &&
					strcmp(name, machine().system().name) != 0 &&
					strcmp(name, srcfile) != 0 &&
					((clone_of = driver_list::clone(machine().system())) == -1 || strcmp(name, driver_list::driver(clone_of).name) != 0) &&
					(clone_of == -1 || ((clone_of = driver_list::clone(clone_of)) == -1) || strcmp(name, driver_list::driver(clone_of).name) != 0))
					continue;
				break;
			}
			default:
				break;
		}

		/* log that we are processing this entry */
		if (DEBUG_CONFIG)
			osd_printf_debug("Entry: %s -- processing\n", name);

		/* loop over all registrants and call their load function */
		for (auto type : m_typelist)
			type.load(which_type, xml_get_sibling(systemnode->child, type.name.c_str()));
		count++;
	}

	/* error if this isn't a valid game match */
	if (count == 0)
		goto error;

	/* free the parser */
	xml_file_free(root);
	return 1;

error:
	if (root)
		xml_file_free(root);
	return 0;
}



/*************************************
 *
 *  XML file save
 *
 *************************************/

int configuration_manager::save_xml(emu_file &file, config_type which_type)
{
	xml_data_node *root = xml_file_create();
	xml_data_node *confignode, *systemnode;

	/* if we don't have a root, bail */
	if (!root)
		return 0;

	/* create a config node */
	confignode = xml_add_child(root, "mameconfig", nullptr);
	if (!confignode)
		goto error;
	xml_set_attribute_int(confignode, "version", CONFIG_VERSION);

	/* create a system node */
	systemnode = xml_add_child(confignode, "system", nullptr);
	if (!systemnode)
		goto error;
	xml_set_attribute(systemnode, "name", (which_type == config_type::CONFIG_TYPE_DEFAULT) ? "default" : machine().system().name);

	/* create the input node and write it out */
	/* loop over all registrants and call their save function */
	for (auto type : m_typelist)
	{
		xml_data_node *curnode = xml_add_child(systemnode, type.name.c_str(), nullptr);
		if (!curnode)
			goto error;
		type.save(which_type, curnode);

		/* if nothing was added, just nuke the node */
		if (!curnode->value && !curnode->child)
			xml_delete_node(curnode);
	}

	/* flush the file */
	xml_file_write(root, file);

	/* free and get out of here */
	xml_file_free(root);
	return 1;

error:
	xml_file_free(root);
	return 0;
}


/***************************************************************************
 *   FUNCTION AGES MOD
 **************************************************************************/

/** Difficult level (enumeration). */
/*@{*/
#define DIFFICULTY_NONE -1 /**< Don't change the value stored in the .cfg file. */
#define DIFFICULTY_EASIEST 0
#define DIFFICULTY_EASY 1
#define DIFFICULTY_MEDIUM 2
#define DIFFICULTY_HARD 3
#define DIFFICULTY_HARDEST 4
/*@}*/

const char* NAME_EASIEST[] = { "Easiest", "Very Easy", 0 };
const char* NAME_EASY[] = { "Easy", "Easier", "Easy?", 0 };
const char* NAME_MEDIUM[] = { "Medium", "Normal", "Normal?", 0 };
const char* NAME_HARD[] = { "Hard", "Harder", "Difficult", "Hard?", 0 };
const char* NAME_HARDEST[] = { "Hardest", "Very Hard", "Very Difficult", 0 };

const float LEVEL_MULT[] = { 1/4, 2/4, 3/4, 2/4, 1/4 };

void configuration_manager::custom_settings()
{
	//config_customize_language(context, list);
	config_customize_difficulty(machine().options().custom_difficulty());
	//config_customize_freeplay(machine().m_portlist);
}

void set_difficulty(int misc_difficulty, ioport_field &found, int steps) {
	
	osd_printf_warning("emu:custom_difficulty: Found Dip:%s, m:%i, df:%i (%i/%i) dl: %i s: %i\n", 
															found.name(), found.mask(),
															found.defvalue(), found.minval(), 
															found.maxval(), found.delta(), steps
														 );
	const char** names;
	const char** names_secondary;

	// get the list of names
	switch (misc_difficulty) {
	case DIFFICULTY_NONE :
		// nothing to do
		return;
	case DIFFICULTY_EASIEST :
		names = NAME_EASIEST;
		names_secondary = NAME_EASY;
		break;
	case DIFFICULTY_EASY :
		names = NAME_EASY;
		names_secondary = 0;
		break;
	case DIFFICULTY_MEDIUM :
		names = NAME_MEDIUM;
		names_secondary = NAME_EASY;
		break;
	case DIFFICULTY_HARD :
		names = NAME_HARD;
		names_secondary = 0;
		break;
	case DIFFICULTY_HARDEST :
		names = NAME_HARDEST;
		names_secondary = NAME_HARD;
		break;
	}

	int level = -1;
	for(int j=0;names[j] && level == -1;++j)
		for (ioport_setting const &iop : found.settings())
		{
			osd_printf_debug("set: name=\"%s\" number=\"%u\"\n", util::xml::normalize_string(iop.name()), iop.value());
			if (strcmp(names[j], iop.name())==0)
			{
				level = iop.value();
				osd_printf_debug("emu:custom_difficulty: Primary match Found: %s switch! set(%i)\n" , iop.name(), level);
				break;
			}
			
		}

	// search a secondary match
	if ((level == -1) && names_secondary)
	{
		for(int j=0;names_secondary[j] && level == -1;++j)
			for (ioport_setting const &iop : found.settings())
			{
				osd_printf_debug("set: name=\"%s\" number=\"%u\"\n", util::xml::normalize_string(iop.name()), iop.value());
				if (strcmp(names_secondary[j], iop.name())==0)
				{
					level = iop.value();
					osd_printf_debug("emu:custom_difficulty: Primary match Found: %s switch! set(%i)\n" , iop.name(), level);
					break;
				}
				
			}

	}
	
	// interpolate
	if (level == -1)
	{
		switch (misc_difficulty)
		{
			case DIFFICULTY_EASIEST :
				level = 0;
				break;
			case DIFFICULTY_EASY :
				level = steps * 1 / 4;
				printf(" %i / 4\n", steps);
				break;
			case DIFFICULTY_MEDIUM :
				level = steps * 2 / 4;
				if(level < found.defvalue())
					level = found.defvalue();
				printf(" %i * 2 / 4 df(%i)\n", steps, found.defvalue());
				break;
			case DIFFICULTY_HARD :
				level = steps * 3 / 4;
				printf(" %i * 3 / 4\n", steps);
				break;
			case DIFFICULTY_HARDEST :
				level = 0xff;
				break;
		}

		for (ioport_setting const &iop : found.settings())
		{
			if(!level--)
			{
				level = iop.value();
				osd_printf_debug("emu:custom_difficulty: interpolate Selected %s (%i - %i)\n", iop.name(), iop.value(), level);
				break;
			 }
		}
	}


	ioport_field::user_settings settings;
	found.get_user_settings(settings);
	settings.value = level;
	found.set_user_settings(settings);

	osd_printf_warning("emu:custom_difficulty: %s set=%i default(%i)\n", found.name(), level, found.defvalue());

}


/**
 * User customization of the difficulty dipswitch.
 */
void configuration_manager::config_customize_difficulty(const char * name_difficulty)
{
	int misc_difficulty = DIFFICULTY_NONE;
	if (core_stricmp(name_difficulty, "easiest") == 0)
		misc_difficulty = DIFFICULTY_EASIEST;
	else if (core_stricmp(name_difficulty, "easy") == 0)
		misc_difficulty = DIFFICULTY_EASY;
	else if (core_stricmp(name_difficulty, "medium") == 0)
		misc_difficulty = DIFFICULTY_MEDIUM;
	else if (core_stricmp(name_difficulty, "hard") == 0)
		misc_difficulty = DIFFICULTY_HARD;
	else if (core_stricmp(name_difficulty, "hardest") == 0)
		misc_difficulty = DIFFICULTY_HARDEST;

	if(misc_difficulty ==  DIFFICULTY_NONE)
		return;

	osd_printf_debug("emu:custom_difficulty: dif:%s(%i)\n", name_difficulty, misc_difficulty);

	for (auto &port : machine().ioport().ports())
		for (ioport_field &field : port.second->fields())
		{
			if (field.type() != IPT_DIPSWITCH)
				continue;

			const char *name = field.name();
			if (name != NULL && strcmp(name, "Difficulty") == 0)
			{   
				int steps = field.settings().count() - 1;
				set_difficulty(misc_difficulty, field, steps);
				return ;
			}
		}

	osd_printf_warning("emu:custom_difficulty: dip switch not found");
	return;
   
}

