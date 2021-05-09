#include "g_local.h"

#define Function(f) {#f, f}	// 3.20

mmove_t mmove_reloc;	// 3.20

field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
//	{"move_angles", FOFS(move_angles), F_VECTOR},	// RSP 100200 - not used
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},		// 3.20
	{"angles", FOFS(s.angles), F_VECTOR},		// 3.20
	{"angle", FOFS(s.angles), F_ANGLEHACK},		// 3.20

	// 3.20
	{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
	{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
	{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
	{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
	{"locked_to", FOFS(locked_to), F_EDICT, FFL_NOSPAWN},	// RSP 042299 - missing function pointer
	{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
	{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
	{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
	{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
	{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
	{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
	{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
	{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
	{"cell", FOFS(cell), F_EDICT, FFL_NOSPAWN},				// RSP 100800
	{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},
	// JBM 19981220 - Added support for the speaker sequencer
	{"sequencer_data", FOFS(sequencer_data), F_TNT_SEQUENCER_INFO, FFL_NOSPAWN},

	{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
	{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
	{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
	{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
	{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
	{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
	{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

	{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
	{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
	{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
	{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
	{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
	{"pivot", FOFS(monsterinfo.pivot), F_FUNCTION, FFL_NOSPAWN},	// RSP 042299 - savegame crash fix?
	{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
	{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
	{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
	{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
	{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
	{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

	{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},
	// 3.20 end

	// AJA 19980613 - Added the "name" and "z_offset" fields for
	// the misc_sprite entity.
	{"name", FOFS(name), F_LSTRING},
	{"z_offset", FOFS(z_offset), F_INT},

	// MRB 19980921 - Readded JohnM's radius field
	{"radius", FOFS(radius), F_FLOAT},

	// temp spawn vars -- only valid when the spawn function is called
	{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
	{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
	{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
	{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
	{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
	{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},

// 3.20
//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item", FOFS(item), F_ITEM},

	{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
	{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
	{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
	{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
	{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},
	
	// JBM 122098 - Added support for the speaker_sequencer
	{"noises", STOFS(noises), F_LSTRING, FFL_SPAWNTEMP},
	{"times", STOFS(times), F_LSTRING, FFL_SPAWNTEMP},
	{"speakers", STOFS(speakers), F_LSTRING, FFL_SPAWNTEMP},
	{"attenuations", STOFS(attenuations), F_LSTRING, FFL_SPAWNTEMP},
	{"volumes", STOFS(volumes), F_LSTRING, FFL_SPAWNTEMP},
	// JBM 010198 - Add the random_style support
	{"random_style", STOFS(random_style), F_INT, FFL_SPAWNTEMP},

	{"attack", FOFS(attack), F_LSTRING},	// RSP 050199 - Waypoint target
	{"retreat", FOFS(retreat), F_LSTRING},	// RSP 050199 - Waypoint target
	{"events", STOFS(events), F_LSTRING, FFL_SPAWNTEMP},	// RSP 050899 - f_e_s
	{"duration", STOFS(duration), F_FLOAT, FFL_SPAWNTEMP},	// RSP 050899 - f_e_s
	{"sizes", STOFS(sizes), F_LSTRING, FFL_SPAWNTEMP},	// RSP 052699 - item_deco
	{"model2", FOFS(model2), F_LSTRING},			// RSP 061899 - for use with func_BoxCar
	{"model3", FOFS(model3), F_LSTRING},			// MRB 071299 - for battlebot
//	{"model4", FOFS(model4), F_LSTRING},			// RSP 061899 - for use with func_BoxCar
	{"frames", STOFS(frames), F_INT, FFL_SPAWNTEMP},	// RSP 062099 - item_deco
	{"curpath", FOFS(curpath), F_EDICT, FFL_NOSPAWN},	// RSP 072499 - for use with func_rotrain
	{"dlockOwner", FOFS(dlockOwner), F_EDICT, FFL_NOSPAWN},	// RSP 090199 - for use with dreadlock
	{"decoy", FOFS(decoy), F_EDICT, FFL_NOSPAWN},		// RSP 070700 - for use with decoy
	{"skins", STOFS(skins), F_INT, FFL_SPAWNTEMP},		// RSP 0072500 - for beetle
	{"teleportgun_victim", FOFS(teleportgun_victim), F_EDICT, FFL_NOSPAWN},	// RSP 083100 - needed for savegame
	{0, 0, 0, 0}	// 3.20
};

// 3.20
field_t		levelfields[] =
{
	{"changemap", LLOFS(changemap), F_LSTRING},                 
	{"sight_client", LLOFS(sight_client), F_EDICT},
	{"sight_entity", LLOFS(sight_entity), F_EDICT},
	{"sound_entity", LLOFS(sound_entity), F_EDICT},
	{"sound2_entity", LLOFS(sound2_entity), F_EDICT},
	{"current_entity", LLOFS(current_entity), F_EDICT},	// RSP 042399 - ID missed this one
	{NULL, 0, F_INT}
};

// 3.20
field_t		clientfields[] =
{
	{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
	{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
//	{"pers.extratoy", CLOFS(pers.extratoy), F_ITEM},		// RSP 092100 - not used
	{"newweapon", CLOFS(newweapon), F_ITEM},
//	{"pers.currenttoy", CLOFS(pers.currenttoy), F_EDICT},	// RSP 092100 - not used
	{"chase_target", CLOFS(chase_target), F_EDICT},			// RSP 042299 - needed for savegame
	{"pers.Dreadlock", CLOFS(pers.Dreadlock), F_EDICT},		// RSP 090199 - needed for savegame
	{NULL, 0, F_INT}
};


// RSP 031199 for saving sequencer data
field_t		sequencerfields[] =
{
	{"noises", SQOFS(noises), F_SEQUENCER_PPCHAR},
	{"times", SQOFS(times), F_SEQUENCER_PPCHAR},
	{"speakers", SQOFS(speakers), F_SEQUENCER_PPCHAR},
	{"attenuations", SQOFS(attenuations), F_SEQUENCER_PPCHAR},
	{"volumes", SQOFS(volumes), F_SEQUENCER_PPCHAR},
	{"events", SQOFS(events), F_SEQUENCER_PPCHAR},
	
	{NULL, 0, F_INT}
};

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame (void)
{
	gi.dprintf ("==== InitGame ====\n");

	gi.dprintf ("== LUC DLL 2.73 ==\n");

	gun_x = gi.cvar ("gun_x", "0", 0);
	gun_y = gi.cvar ("gun_y", "0", 0);
	gun_z = gi.cvar ("gun_z", "0", 0);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar ("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar ("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar ("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar ("sv_gravity", "800", 0);

	// noset vars
	dedicated = gi.cvar ("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	gi.cvar ("gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar ("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);

	maxclients = gi.cvar ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar ("maxspectators", "4", CVAR_SERVERINFO);	// 3.20
	deathmatch = gi.cvar ("deathmatch", "0", CVAR_LATCH);
	coop = gi.cvar ("coop", "0", CVAR_LATCH);
	skill = gi.cvar ("skill", "1", CVAR_LATCH);
	maxentities = gi.cvar ("maxentities", "1024", CVAR_LATCH);

	// change anytime vars
	dmflags = gi.cvar ("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.cvar ("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar ("timelimit", "0", CVAR_SERVERINFO);
	password = gi.cvar ("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar ("spectator_password", "", CVAR_USERINFO);	// 3.20
	needpass = gi.cvar ("needpass", "0", CVAR_SERVERINFO);			// 3.20
	filterban = gi.cvar ("filterban", "1", 0);				// 3.20

	g_select_empty = gi.cvar ("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar ("run_pitch", "0.002", 0);
	run_roll = gi.cvar ("run_roll", "0.005", 0);
	bob_up  = gi.cvar ("bob_up", "0.005", 0);
	bob_pitch = gi.cvar ("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar ("bob_roll", "0.002", 0);

	// flood control (3.20)
	flood_msgs = gi.cvar ("flood_msgs", "4", 0);
	flood_persecond = gi.cvar("flood_persecond","4",0);
	flood_waitdelay = gi.cvar("flood_waitdelay","10",0);

	// dm map list (3.20)
	sv_maplist = gi.cvar ("sv_maplist", "", 0);

	// RSP 042599 - Added CD cvar. "0" = get from CD. "1" = get from pak
	cd_source = gi.cvar("cd_source","0",0);
	if (strcmp("1",cd_source->string) == 0)
		cd_source->value = CD_SOURCE_PAK;
	else
		cd_source->value = CD_SOURCE_CD;

	// items
	InitItems ();

	Com_sprintf (game.helpmessage1, sizeof(game.helpmessage1), "");

	Com_sprintf (game.helpmessage2, sizeof(game.helpmessage2), "");

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);

	// JBM 122098 - Initialize all the tnt_seq's for all the ents.
	g_tnt_seqs = gi.TagMalloc (game.maxentities * sizeof(g_tnt_seqs[0]), TAG_GAME);

	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->value;
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients+1;
}

//=========================================================

void WriteField1 (FILE *f, field_t *field, byte *base)
{
	void	*p;
	int		len;
	int		index;
	char 	**str;	// RSP 031199

	if (field->flags & FFL_SPAWNTEMP)	// 3.20
		return;

	p = (void *)(base + field->ofs);

	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;
	case F_LSTRING:
//	case F_GSTRING:	// 3.20
		if ( *(char **)p )
			len = strlen(*(char **)p) + 1;
		else
			len = 0;
		*(int *)p = len;
		break;
	case F_TNT_SEQUENCER_INFO:
		if ( *(tnt_sequencer_info **)p == NULL)
			index = -1;
		else
			index = *(tnt_sequencer_info **)p - g_tnt_seqs;
		*(int *)p = index;
		break;
	case F_EDICT:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(edict_t **)p - g_edicts;
		*(int *)p = index;
		break;
	case F_CLIENT:
		if ( *(gclient_t **)p == NULL)
			index = -1;
		else
			index = *(gclient_t **)p - game.clients;
		*(int *)p = index;
		break;
	case F_ITEM:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(gitem_t **)p - itemlist;
		*(int *)p = index;
		break;

	//relative to code segment (3.20)
	case F_FUNCTION:
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - ((byte *)(void *)InitGame);
		*(int *)p = index;
		break;

	//relative to data segment (3.20)
	case F_MMOVE:
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - (byte *)&mmove_reloc;
		*(int *)p = index;
		break;

	// RSP 031199 - Added support for sequencer **char fields
	case F_SEQUENCER_PPCHAR:
		if ( *(char ***)p == NULL)
			index = -1;
		else
		{
			index = 0;
			str = *(char ***)p;	// set pointer to start of array
			// count pointers in this array
			while (str[index])
				index++;
		}
		*(int *)p = index;
		break;

	default:
		gi.error ("WriteEdict: unknown field type");
	}
}

void WriteField2 (FILE *f, field_t *field, byte *base)
{
	int		len;
	void	*p;

	if (field->flags & FFL_SPAWNTEMP)	// 3.20
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_LSTRING:
//	case F_GSTRING:	// 3.20
		if ( *(char **)p )
		{
			len = strlen(*(char **)p) + 1;
			fwrite (*(char **)p, len, 1, f);
		}
		break;

	// RSP 031199 - Added support for sequencer **char fields
	case F_SEQUENCER_PPCHAR:
		if (*(char ***)p)
		{
			int 	i;
			char	**str;

			str = *(char ***)p;	// set pointer to start of array
			// write strings in this array
			for (i = 0 ; str[i] ; i++)
				fwrite(str[i],strlen(str[i])+1,1,f);	// write string
		}
		break;
	}
}

void ReadField (FILE *f, field_t *field, byte *base)
{
	void	*p;
	int		len;
	int		index;

	if (field->flags & FFL_SPAWNTEMP)	// 3.20
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;
	case F_LSTRING:
		len = *(int *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = gi.TagMalloc (len, TAG_LEVEL);
			fread (*(char **)p, len, 1, f);
		}
		break;

/* 3.20
	case F_GSTRING:
		len = *(int *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = gi.TagMalloc (len, TAG_GAME);
			fread (*(char **)p, len, 1, f);
		}
		break;
 */

	// JBM 122098 - Added support for the speaker sequencer. F_TNT_SEQUENCER_INFO is the datatype for
	// save games stuff
	case F_TNT_SEQUENCER_INFO:
		index = *(int *)p;
		if ( index == -1 )
			*(tnt_sequencer_info **)p = NULL;
		else
			*(tnt_sequencer_info **)p = &g_tnt_seqs[index];
		break;
	case F_EDICT:
		index = *(int *)p;
		if ( index == -1 )
			*(edict_t **)p = NULL;
		else
			*(edict_t **)p = &g_edicts[index];
		break;
	case F_CLIENT:
		index = *(int *)p;
		if ( index == -1 )
			*(gclient_t **)p = NULL;
		else
			*(gclient_t **)p = &game.clients[index];
		break;
	case F_ITEM:
		index = *(int *)p;
		if ( index == -1 )
			*(gitem_t **)p = NULL;
		else
			*(gitem_t **)p = &itemlist[index];
		break;

	// RSP 031199 - Added support for sequencer **char fields
	case F_SEQUENCER_PPCHAR:
		index = *(int *)p;
		if ( index == -1 )
			*(char **)p = NULL;
		else
		{
			int 	i,j;
			char	**str;
			char	buf[512];

			// create array of string pointers
			str = gi.TagMalloc((index+1)*sizeof(str),TAG_LEVEL);
			// read strings in this array
			for (i = 0 ; i < index ; i++)
			{
				for (j = 0 ;; j++)
				{
					fread(&buf[j],1,1,f);
					if (!buf[j])
						break;
				}
				str[i] = gi.TagMalloc(strlen(buf)+1,TAG_LEVEL);
				strcpy(str[i],buf);	// copy string
			}
			str[index] = NULL;	// delimiter marking end of array
			*(char ***)p = str;	// save array pointer in edict
		}
		break;

	//relative to code segment (3.20)
	case F_FUNCTION:
		index = *(int *)p;
		if ( index == 0 )
			*(byte **)p = NULL;
		else
			*(byte **)p = ((byte *)(void *)InitGame) + index;
		break;

	//relative to data segment (3.20)
	case F_MMOVE:
		index = *(int *)p;
		if (index == 0)
			*(byte **)p = NULL;
		else
			*(byte **)p = (byte *)&mmove_reloc + index;
		break;

	default:
		gi.error ("ReadEdict: unknown field type");
	}
}

//=========================================================

/*
==============
WriteClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteClient (FILE *f, gclient_t *client)
{
	field_t		*field;
	gclient_t	temp;
	
	// all of the ints, floats, and vectors stay as they are
	temp = *client;
	// change the pointers to lengths or indexes
	for (field = clientfields ; field->name ; field++)
		WriteField1 (f, field, (byte *)&temp);

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field = clientfields ; field->name ; field++)
		WriteField2 (f, field, (byte *)client);
}

/*
==============
ReadClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadClient (FILE *f, gclient_t *client)
{
	field_t *field;

	fread (client, sizeof(*client), 1, f);

	for (field = clientfields ; field->name ; field++)
		ReadField (f, field, (byte *)client);
}

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void WriteGame (char *filename, qboolean autosave)
{
	FILE	*f;
	int		i;
	char	str[16];

	if (!autosave)
		SaveClientData ();

	f = fopen (filename, "wb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	memset (str, 0, sizeof(str));
	strcpy (str, __DATE__);
	fwrite (str, sizeof(str), 1, f);

	game.autosaved = autosave;
	fwrite (&game, sizeof(game), 1, f);
	game.autosaved = false;

	for (i = 0 ; i < game.maxclients ; i++)
		WriteClient (f, &game.clients[i]);

	fclose (f);
}

void ReadGame (char *filename)
{
	FILE	*f;
	int		i;
	char	str[16];

	gi.FreeTags (TAG_GAME);

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	fread (str, sizeof(str), 1, f);
	if (strcmp (str, __DATE__))
	{
		fclose (f);
		gi.error ("Savegame from an older version.\n");
	}

	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;

	// JBM 122098 - Initialize all the tnt_seq's for all the ents.
	g_tnt_seqs = gi.TagMalloc (game.maxentities * sizeof(g_tnt_seqs[0]), TAG_GAME);

	fread (&game, sizeof(game), 1, f);
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	for (i = 0 ; i < game.maxclients ; i++)
		ReadClient (f, &game.clients[i]);

	fclose (f);
}

//==========================================================


/*
==============
WriteEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteEdict (FILE *f, edict_t *ent)
{
	field_t		*field;
	edict_t		temp;

	// all of the ints, floats, and vectors stay as they are
	temp = *ent;

	// change the pointers to lengths or indexes
	for (field = fields ; field->name ; field++)	// 3.20
		WriteField1 (f, field, (byte *)&temp);

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field = fields ; field->name ; field++)	// 3.20
		WriteField2 (f, field, (byte *)ent);
}

/*
==============

// RSP 031199
WriteSequencer

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteSequencer(FILE *f, tnt_sequencer_info *seq)
{
	field_t			*field;
	tnt_sequencer_info	temp;

	// all of the ints, floats, and vectors stay as they are
	temp = *seq;

	// change the pointers to lengths or indexes
	for (field = sequencerfields ; field->name ; field++)
		WriteField1(f,field,(byte *)&temp);

	// write the block
	fwrite(&temp,sizeof(temp),1,f);

	// now write any allocated data following the sequencer
	for (field = sequencerfields ; field->name ; field++)
		WriteField2(f,field,(byte *)seq);
}

/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteLevelLocals (FILE *f)
{
	field_t		*field;
	level_locals_t		temp;

	// all of the ints, floats, and vectors stay as they are
	temp = level;

	// change the pointers to lengths or indexes
	for (field = levelfields ; field->name ; field++)
		WriteField1 (f, field, (byte *)&temp);

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field = levelfields ; field->name ; field++)
		WriteField2 (f, field, (byte *)&level);
}


/*
==============
ReadEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadEdict (FILE *f, edict_t *ent)
{
	field_t *field;

	fread (ent, sizeof(*ent), 1, f);

	for (field = fields ; field->name ; field++)	// 3.20
		ReadField (f, field, (byte *)ent);
}

/*
==============

// RSP 031199
ReadSequencer

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadSequencer(FILE *f,tnt_sequencer_info *seq)
{
	field_t	*field;

	fread(seq,sizeof(*seq),1,f);

	for (field = sequencerfields ; field->name ; field++)
		ReadField(f,field,(byte *)seq);
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadLevelLocals (FILE *f)
{
	field_t *field;

	fread (&level, sizeof(level), 1, f);

	for (field = levelfields ; field->name ; field++)
		ReadField (f, field, (byte *)&level);
}

/*
=================
WriteLevel

=================
*/
void WriteLevel (char *filename)
{
	int		i;
	edict_t	*ent;
	FILE	*f;
	void	*base;
	tnt_sequencer_info	*seq;	// RSP 031199

	f = fopen (filename, "wb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// write out edict size for checking
	i = sizeof(edict_t);
	fwrite (&i, sizeof(i), 1, f);

	// write out a function pointer for checking
	base = (void *)InitGame;
	fwrite (&base, sizeof(base), 1, f);

	// write out level_locals_t
	WriteLevelLocals (f);

	// write out all the entities
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		fwrite (&i, sizeof(i), 1, f);
		WriteEdict (f, ent);
	}
	i = -1;
	fwrite (&i, sizeof(i), 1, f);

	// RSP 031199
	// Write out all the sequencers. the number of sequencer items = number of edicts
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		seq = &g_tnt_seqs[i];
		fwrite(&i,sizeof(i),1,f);
		WriteSequencer(f,seq);
	}
	i = -1;
	fwrite (&i, sizeof(i), 1, f);
	// End of writing sequencers

	fclose (f);
}


/*
=================
ReadLevel

SpawnEntities will already have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel (char *filename)
{
	int		entnum,seqnum;	// RSP 031199
	FILE	*f;
	int		i;
	void	*base;
	edict_t	*ent;
	tnt_sequencer_info	*seq;	// RSP 031199

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags (TAG_LEVEL);

	// wipe all the entities
	memset (g_edicts, 0, game.maxentities*sizeof(g_edicts[0]));
	// wipe all the tnt_seq's
	memset (g_tnt_seqs, 0, game.maxentities*sizeof(g_tnt_seqs[0])); // RSP 022499: fix savegame crash
	globals.num_edicts = maxclients->value+1;

	// check edict size
	fread (&i, sizeof(i), 1, f);
	if (i != sizeof(edict_t))
	{
		fclose (f);
		gi.error ("ReadLevel: mismatched edict size");
	}

	// check function pointer base address
	fread (&base, sizeof(base), 1, f);
#ifdef _WIN32
	if (base != (void *)InitGame)
	{	// RSP 041999 - keep going anyway
		gi.dprintf("Functions offset %x\n",((byte *)InitGame) - ((byte *)base));
//		fclose (f);
		// RSP 041599 - print the values so we can see the movement
//		gi.error ("ReadLevel: function pointers have moved from %x to %x",base,(void *)InitGame);
	}
//	else
//		gi.dprintf("Functions stable\n");
#else
	gi.dprintf("Function offsets %d\n", ((byte *)base) - ((byte *)InitGame));
#endif

	// load the level locals
	ReadLevelLocals (f);

	// load all the entities
	while (1)
	{
		if (fread (&entnum, sizeof(entnum), 1, f) != 1)
		{
			fclose (f);
			gi.error ("ReadLevel: failed to read entnum");
		}
		if (entnum == -1)
			break;
		if (entnum >= globals.num_edicts)
			globals.num_edicts = entnum+1;

		ent = &g_edicts[entnum];
		ReadEdict (f, ent);

		// let the server rebuild world links for this ent
		memset (&ent->area, 0, sizeof(ent->area));
		gi.linkentity (ent);
	}

	// RSP 031199
	// load all the sequencers
	while (1)
	{
		if (fread (&seqnum, sizeof(seqnum), 1, f) != 1)
		{
			fclose (f);
			gi.error ("ReadLevel: failed to read sequencer num");
		}
		if (seqnum == -1)
			break;
		seq = &g_tnt_seqs[seqnum];
		ReadSequencer(f,seq);
	}
	// End of loading sequencers

	fclose (f);

	// mark all clients as unconnected
	for (i = 0 ; i < maxclients->value ; i++)
	{
		ent = &g_edicts[i+1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
	}

	// do any load time things at this point
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level triggers
		if (ent->classname)
			if (strcmp(ent->classname, "target_crosslevel_target") == 0)
				ent->nextthink = level.time + ent->delay;
	}
}
