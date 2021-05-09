
#include "g_local.h"
#include "luc_powerups.h"	// WPO 100199 - Added powerups
#include "luc_m_battle.h"

extern edict_t *BoxCar_Passengers;	// Array of transported entities
extern char BoxCarName[];			// RSP 042000

typedef struct
{
	char	*name;
	void	(*spawn)(edict_t *ent);
} spawn_t;

// RSP 032399 - removed ID health items
//void SP_item_health (edict_t *self);
//void SP_item_health_small (edict_t *self);
//void SP_item_health_large (edict_t *self);
//void SP_item_health_mega (edict_t *self);

// RSP 071999 - replaced with LUC health items
void SP_item_berry1 (edict_t *self);
void SP_item_berry2 (edict_t *self);

// RSP 082200 - added health urns
void SP_item_health_mini (edict_t *self);
void SP_item_health_midi (edict_t *self);
void SP_item_health_maxi (edict_t *self);

void SP_info_player_start (edict_t *ent);
void SP_info_player_deathmatch (edict_t *ent);
void SP_info_player_coop (edict_t *ent);
void SP_info_player_intermission (edict_t *ent);

void SP_func_plat (edict_t *ent);
void SP_func_rotating (edict_t *ent);
void SP_func_button (edict_t *ent);
void SP_func_door (edict_t *ent);
void SP_func_door_secret (edict_t *ent);
void SP_func_door_rotating (edict_t *ent);
void SP_func_water (edict_t *ent);
void SP_func_train (edict_t *ent);
void SP_func_rotrain (edict_t *ent);	// RSP 071699
void SP_func_conveyor (edict_t *self);
void SP_func_wall (edict_t *self);
void SP_func_object (edict_t *self);
void SP_func_explosive (edict_t *self);
void SP_func_timer (edict_t *self);
void SP_func_areaportal (edict_t *ent);
void SP_func_clock (edict_t *ent);
void SP_func_killbox (edict_t *ent);

void SP_trigger_always (edict_t *ent);
void SP_trigger_once (edict_t *ent);
void SP_trigger_multiple (edict_t *ent);
void SP_trigger_relay (edict_t *ent);
void SP_trigger_push (edict_t *ent);
void SP_trigger_hurt (edict_t *ent);
void SP_trigger_key (edict_t *ent);
void SP_trigger_counter (edict_t *ent);
void SP_trigger_elevator (edict_t *ent);
void SP_trigger_gravity (edict_t *ent);
void SP_trigger_monsterjump (edict_t *ent);

void SP_target_temp_entity (edict_t *ent);
void SP_target_speaker (edict_t *ent);
void SP_target_explosion (edict_t *ent);
void SP_target_changelevel (edict_t *ent);
void SP_target_secret (edict_t *ent);
void SP_target_goal (edict_t *ent);
void SP_target_splash (edict_t *ent);
void SP_target_spawner (edict_t *ent);
//void SP_target_blaster (edict_t *ent);	// RSP 063000 - None in LUC
void SP_target_crosslevel_trigger (edict_t *ent);
void SP_target_crosslevel_target (edict_t *ent);
void SP_target_laser (edict_t *self);
void SP_target_help (edict_t *ent);
void SP_target_lightramp (edict_t *self);
void SP_target_earthquake (edict_t *ent);
void SP_target_character (edict_t *ent);
void SP_target_string (edict_t *ent);
void SP_target_blinkout (edict_t *);	// RSP 100800

void SP_worldspawn (edict_t *ent);
//void SP_viewthing (edict_t *ent);		// RSP 081400 - None in LUC

void SP_light (edict_t *self);
//void SP_light_mine1 (edict_t *ent);	// RSP 041799 - None in LUC
//void SP_light_mine2 (edict_t *ent);	// RSP 041799 - None in LUC
void SP_info_null (edict_t *self);
void SP_info_notnull (edict_t *self);
void SP_path_corner (edict_t *self);
void SP_point_combat (edict_t *self);

//void SP_misc_explobox (edict_t *self);	// RSP 062299 - changed to item_deco
void SP_misc_teleporter (edict_t *self);
//void SP_misc_teleporter_dest (edict_t *self);

// AJA 19980308 - Forward decl of gwhite spawn function.
void SP_monster_gwhite (edict_t *self);

// AJA 19980319 - Forward decl of bubblegen1's spawn function.
void SP_misc_bubblegen1 (edict_t *self);

// MRB 19980427 - Forward declaration for more LUC stuff
void SP_monster_jelly (edict_t *self);
void SP_kelp_small (edict_t *self);
void SP_kelp_large (edict_t *self);
void SP_monster_landjelly (edict_t *self);
void SP_monster_littleboy (edict_t *self);	// RSP 052199

// AJA 19980503 - Forward decl of the supervisor bot spawn function.
void SP_monster_supervisor (edict_t *self);

void SP_monster_guardbot (edict_t *);	// RSP 100700
void SP_monster_target (edict_t *);		// RSP 100800

// MRB 19980601 - Forward declaration for more LUC stuff
void SP_func_steam (edict_t *self);

// AJA 19980613 - Forward decl of test entity for messing with sprites.
void SP_misc_sprite (edict_t *self);

// MRB 19981001 - Re-added func_teleport
void SP_func_teleport (edict_t *self);

// MRB 19980620 - Forward declaration for more LUC stuff
void SP_monster_bot_workerclaw (edict_t *self);
void SP_monster_bot_workercoil (edict_t *self);
void SP_monster_bot_workerbeam (edict_t *self);
void SP_monster_turret_static (edict_t *self);
void SP_monster_bot_fatman (edict_t *self);
void SP_monster_bot_gbot (edict_t *self);
void SP_monster_repeater (edict_t *self);
// MRB 19980120 - Chained to misc_ entites 
void SP_misc_bot_drill (edict_t *self);
void SP_misc_bot_probe (edict_t *self);

// JBM 19981031 - Forward declaration for more LUC stuff
void SP_func_speaker_sequencer (edict_t *self);

// RSP 042599 - Forward declaration for more LUC stuff
void SP_func_CD (edict_t *self);

// RSP 042999 - Forward declaration for more LUC stuff
void SP_waypoint (edict_t *self);

// RSP 050899 - Added func_event_sequencer
void SP_func_event_sequencer (edict_t *self);

// RSP 093000 - Added trigger_capture for endgame
void SP_trigger_capture (edict_t *);

// RSP 110400 - Added trigger_slowdeath for endgame
void SP_trigger_slowdeath (edict_t *);

// RSP 052699 - Added item_deco
void SP_item_deco (edict_t *);

// RSP 061699 - Added func_BoxCar
void SP_func_BoxCar (edict_t *);

// MRB 060999 - Light entities
void SP_light_floor1(edict_t *self);
void SP_light_ceil1(edict_t *self);
void SP_light_ceil2(edict_t *self);
void SP_light_wall1(edict_t *self);
void SP_light_wall2(edict_t *self);
void SP_light_wall3(edict_t *self);

//MRB 062899 - New Monsters
//void SP_monster_battle(edict_t *self);	// RSP 081400
//void SP_monster_ambush(edict_t *self);	// RSP 081400

//MRB 071099 - few more bots
void SP_monster_battle_disk(edict_t *self);
void SP_monster_battle_bolt(edict_t *self);

//MRB 080999 - and a few more
void SP_monster_ambush_disk(edict_t *self);
void SP_monster_ambush_bolt(edict_t *self);

// WPO 081999 - Dreadlock trigger (assigns a dreadlock to the player)
void SP_trigger_hbot (edict_t *self);

// New scenario_end triggers for the endgame
void SP_trigger_scenario_end (edict_t *);		// RSP 092500
void SP_trigger_end_scenario2 (edict_t *ent);	// RSP 101900

// RSP 101599 - A searchlight entity
void SP_func_searchlight (edict_t *self);

// RSP 072100 - beetle
void SP_Beetle(edict_t *);
void SP_Beetle_top1(edict_t *);
void SP_Beetle_top2(edict_t *);
void SP_Beetle_top3(edict_t *);
void SP_Beetle_spider1(edict_t *);
void SP_Beetle_spider2(edict_t *);
void SP_Beetle_spider3(edict_t *);
void SP_Beetle_spider4(edict_t *);

// RSP 081500 - spear lights
void SP_Spear_Light(edict_t *);
void SP_RocketSpear_Light(edict_t *);

// RSP 102700 - endgame embracer
void SP_embracer(edict_t *);

spawn_t	spawns[] = {

	{"light", SP_light},	// RSP 092100 - moved up front since it's the most often-used entity

	{"monster_jelly", SP_monster_jelly},			// MRB 19980427
	{"monster_landjelly", SP_monster_landjelly},	// MRB 19980427
	{"monster_supervisor", SP_monster_supervisor},	// AJA 19980503 - Supervisor bot.
	{"monster_bot_workerclaw", SP_monster_bot_workerclaw},	// MRB 19980620
	{"monster_bot_workercoil", SP_monster_bot_workercoil},	// MRB 19980620
	{"monster_bot_workerbeam", SP_monster_bot_workerbeam},	// MRB 19980620
	{"monster_bot_gbot", SP_monster_bot_gbot},
	{"monster_repeater", SP_monster_repeater},
	{"monster_battle_disk",SP_monster_battle_disk},	// MRB 071099
	{"monster_battle_bolt",SP_monster_battle_bolt},	// MRB 071099
	{"monster_ambush_disk",SP_monster_ambush_disk},	// MRB 081099
	{"monster_ambush_bolt",SP_monster_ambush_bolt},	// MRB 081099
	{"monster_beetle",SP_Beetle},					// RSP 072100
	{"monster_beetle_top1",SP_Beetle_top1},			// RSP 090800
	{"monster_beetle_top2",SP_Beetle_top2},			// RSP 090800
	{"monster_beetle_top3",SP_Beetle_top3},			// RSP 090800
	{"monster_beetle_spider1",SP_Beetle_spider1},	// RSP 090800
	{"monster_beetle_spider2",SP_Beetle_spider2},	// RSP 090800
	{"monster_beetle_spider3",SP_Beetle_spider3},	// RSP 090800
	{"monster_beetle_spider4",SP_Beetle_spider4},	// RSP 090800
	{"monster_turret_static", SP_monster_turret_static},
	{"monster_gwhite", SP_monster_gwhite},	// AJA 19980307 - Spawn support for the great white shark
	{"monster_bot_fatman", SP_monster_bot_fatman},
	{"monster_littleboy", SP_monster_littleboy},	// RSP 052199
	{"monster_kelp_small", SP_kelp_small},			// MRB 19980427
	{"monster_kelp_large", SP_kelp_large},			// MRB 19980427
	{"monster_guardbot", SP_monster_guardbot},		// RSP 100700
	{"monster_target", SP_monster_target},			// RSP 100800

	// RSP 071999 - new LUC health items
	{"item_berry1", SP_item_berry1},
	{"item_berry2", SP_item_berry2},

	// RSP 082200 - new LUC health urn items
	{"item_health_mini", SP_item_health_mini},
	{"item_health_midi", SP_item_health_midi},
	{"item_health_maxi", SP_item_health_maxi},

	{"item_deco",SP_item_deco},	// RSP 052699 - Spawn support for generic decoration items

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_door_secret", SP_func_door_secret},
	{"func_door_rotating", SP_func_door_rotating},
	{"func_rotating", SP_func_rotating},
	{"func_train", SP_func_train},
	{"func_rotrain", SP_func_rotrain},	// RSP 071699 - new rotating func_trains
	{"func_water", SP_func_water},
	{"func_conveyor", SP_func_conveyor},
	{"func_areaportal", SP_func_areaportal},
	{"func_clock", SP_func_clock},
	{"func_wall", SP_func_wall},
	{"func_object", SP_func_object},
	{"func_timer", SP_func_timer},
	{"func_explosive", SP_func_explosive},
	{"func_killbox", SP_func_killbox},

	{"trigger_always", SP_trigger_always},
	{"trigger_once", SP_trigger_once},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_relay", SP_trigger_relay},
	{"trigger_push", SP_trigger_push},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_key", SP_trigger_key},
	{"trigger_counter", SP_trigger_counter},
	{"trigger_elevator", SP_trigger_elevator},
	{"trigger_gravity", SP_trigger_gravity},
	{"trigger_monsterjump", SP_trigger_monsterjump},
	{"trigger_end_scenario2", SP_trigger_end_scenario2},	// RSP 101900

	{"target_temp_entity", SP_target_temp_entity},
	{"target_speaker", SP_target_speaker},
	{"target_explosion", SP_target_explosion},
	{"target_changelevel", SP_target_changelevel},
	{"target_secret", SP_target_secret},
	{"target_goal", SP_target_goal},
	{"target_splash", SP_target_splash},
	{"target_spawner", SP_target_spawner},
	// WPO 042800 Remove target_blaster reference
//	{"target_blaster", SP_target_blaster},
	{"target_crosslevel_trigger", SP_target_crosslevel_trigger},
	{"target_crosslevel_target", SP_target_crosslevel_target},
	{"target_laser", SP_target_laser},
	{"target_help", SP_target_help},
	{"target_lightramp", SP_target_lightramp},
	{"target_earthquake", SP_target_earthquake},
	{"target_character", SP_target_character},
	{"target_string", SP_target_string},
	{"target_blinkout", SP_target_blinkout},	// RSP 100800

	{"info_null", SP_info_null},
	{"func_group", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"path_corner", SP_path_corner},
	{"point_combat", SP_point_combat},

//	{"misc_explobox", SP_misc_explobox},	// RSP 062299 - changed to item_deco
	// WPO 042800 removed misc_teleporter reference
	{"misc_teleporter", SP_misc_teleporter},
//	{"misc_teleporter_dest", SP_misc_teleporter_dest},

	{"misc_bot_drill",SP_misc_bot_drill},		// MRB 19980122
	{"misc_bot_probe",SP_misc_bot_probe},		// MRB 19980122
	{"misc_bubblegen1", SP_misc_bubblegen1},	// AJA 19980319 - Bubble generator 1.
	{"misc_sprite", SP_misc_sprite},			// AJA 19980613 - Spawning for the sprite entity

	{"func_steam", SP_func_steam},

	{"func_teleport", SP_func_teleport},	// MRB 19981001 - Re-added func_teleport

	// JBM 19981031 - Spawn support for func_speaker_sequencer
	{"func_speaker_sequencer", SP_func_speaker_sequencer},

	// RSP 042599 - Spawn support for func_CD
	{"func_CD", SP_func_CD},

	// RSP 042999 - Spawn support for waypoints
	{"waypoint", SP_waypoint},

	// RSP 050899 - Spawn support for func_event_sequencer
	{"func_event_sequencer", SP_func_event_sequencer},

	// RSP 061699 - Spawn support for func_BoxCar
	{"func_BoxCar",SP_func_BoxCar},

	// MRB 060999 - Light entities
	{"light_floor1", SP_light_floor1},
	{"light_ceil1",  SP_light_ceil1},
	{"light_ceil2",  SP_light_ceil2},
	{"light_wall1",  SP_light_wall1},
	{"light_wall2",  SP_light_wall2},
	{"light_wall3",  SP_light_wall3},

	// WPO 081999 - Dreadlock trigger (assigns a dreadlock to the player)
	{"trigger_hbot", SP_trigger_hbot},

	// RSP 081500 - new spear lights
	{"spearlight",SP_Spear_Light},
	{"rocketlight",SP_RocketSpear_Light},

	// RSP 101599 - new searchlight entity
	{"func_searchlight", SP_func_searchlight},

	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_coop", SP_info_player_coop},
	{"info_player_intermission", SP_info_player_intermission},

	// RSP 092500 - new scenario_end trigger for the endgame
	{"trigger_scenario_end", SP_trigger_scenario_end},

	// RSP 093000 - Spawn support for trigger_capture, for the endgame
	{"trigger_capture", SP_trigger_capture},

	// RSP 110400 - Spawn support for trigger_slowdeath, for the endgame
	{"trigger_slowdeath", SP_trigger_slowdeath},

	// RSP 102700 - embracer for endgame
	{"embracer", SP_embracer},

	{"worldspawn", SP_worldspawn},
//	{"viewthing", SP_viewthing},			// RSP 081400 - None in LUC

	{NULL,NULL}
};

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn (edict_t *ent)
{
	spawn_t	*s;
	gitem_t	*item;
	int		i;

	if (!ent->classname)
	{
		gi.dprintf ("ED_CallSpawn: NULL classname\n");
		return;
	}

	// check item spawn functions
	for (i = 1, item = &itemlist[1] ; i < game.num_items ; i++, item++)	// RSP 092600 - skip NULL entry at start
	{
//		if (!item->classname)	// RSP 092600 - all items have a classname
//			continue;

		if (!strcmp(item->classname, ent->classname))
		{	// found it
			SpawnItem (ent, item);
			return;
		}
	}

	// check normal spawn functions
	for (s = spawns ; s->name ; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{	// found it
			s->spawn (ent);
			return;
		}
	}

	gi.dprintf ("%s doesn't have a spawn function\n", ent->classname);
}

/*
=============
ED_NewString
=============
*/
char *ED_NewString (char *string)
{
	char	*newb, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;

	newb = gi.TagMalloc (l, TAG_LEVEL);

	new_p = newb;

	for (i = 0 ; i < l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}
	
	return newb;
}


/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField (char *key, char *value, edict_t *ent)
{
	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for (f = fields ; f->name ; f++)
	{
		if (!(f->flags & FFL_NOSPAWN) && !Q_stricmp(f->name, key))	// 3.20
		{	// found it
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte *)&st;
			else
				b = (byte *)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*(char **)(b+f->ofs) = ED_NewString (value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			case F_IGNORE:
				break;
			}
			return;
		}
	}
	gi.dprintf ("%s is not a field\n", key);
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
char *ED_ParseEdict (char *data, edict_t *ent)
{
	qboolean	init;
	char		keyname[256];
	char		*com_token;

	init = false;
	memset (&st, 0, sizeof(st));

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		com_token = COM_Parse (&data);
		if (com_token[0] == '}')
			break;
		if (!data)
			gi.error ("ED_ParseEntity: EOF without closing brace");

		strncpy (keyname, com_token, sizeof(keyname)-1);
		
	// parse value	
		com_token = COM_Parse (&data);
		if (!data)
			gi.error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			gi.error ("ED_ParseEntity: closing brace without data");

		init = true;	

	// keynames with a leading underscore are used for utility comments,
	// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		ED_ParseField (keyname, com_token, ent);
	}

	if (!init)
		memset (ent, 0, sizeof(*ent));

	return data;
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams (void)
{
	edict_t	*e, *e2, *chain;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for (i = 1, e = g_edicts+i ; i < globals.num_edicts ; i++, e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for (j = i+1, e2 = e+1 ; j < globals.num_edicts ; j++, e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf ("%i teams with %i entities\n", c, c2);
}

// RSP 061699
// Add in the passenger entities brought over in a func_BoxCar.

// There has to be a matching func_BoxCar in this map. Both the start
// and destination func_BoxCars should have the same targetname.

void Add_BoxCar()
{
	int		i,f,j;
	edict_t	*e,*b;
	vec3_t	absmin_delta;
	float	t;

	e = G_Find(NULL,FOFS(targetname),BoxCarName);	// RSP 042000
	if (!e)
	{
		gi.dprintf("No destination func_BoxCar with a targetname of %s\n",BoxCarName);	// RSP 042000
		BoxCar_Passengers = NULL;	// array memory will be freed at end of game
		return;
	}

	// All passengers must move to the correct location in the destination func_BoxCar

	VectorSubtract(e->absmin,BoxCar_Passengers[0].absmin,absmin_delta);

	// Assign edict slots in g_edicts for all passengers

	for (i = 1 ; i <= BoxCar_Passengers[0].count ; i++)
	{
		b = &BoxCar_Passengers[i];
		if (b->client)	// if client, edict slot already assigned
			e = &g_edicts[b->s.number];
		else
			e = G_Spawn();			// create new edict for this passenger

		// Save edict address. Used below to convert indices back to pointers.

		b->boxcar_index = (int) e;
	}
	
	// Go through the passengers and restore links to entities they pointed to.
	// Also reset the time-based values and the frame-based values, since both
	// time (level.time) and frames (level.framenum) are different in the new
	// level. Also reset the model and sound indices, because these are
	// dependent on the sequence in which the models and sounds are indexed, and
	// the indices could be different in a new level.

	// t is the timedelta that needs to be added to the stored times so they're
	// in sync with the new level.time

	t = level.time - BoxCar_Passengers[0].nextthink;	// nextthink holds the old level's
														// level.time when the boxcar was
														// created

//	t = level.time + 20*FRAMETIME - BoxCar_Passengers[0].nextthink;	// RSP DEBUG

	// f is the framedelta that needs to be added to the stored frames so they're
	// in sync with the new level.framenum

	f = level.framenum - BoxCar_Passengers[0].count2;	// count2 holds the old level's
														// level.framenum when the boxcar
														// was created

	for (i = 1 ; i <= BoxCar_Passengers[0].count ; i++)	// count holds the number of passengers
	{
		// reset the entity pointers

		b = &BoxCar_Passengers[i];

		if (b->locked_to == (edict_t *) -1)	// world is special case
			b->locked_to = world;
		else if (b->locked_to)
			b->locked_to = (edict_t *) BoxCar_Passengers[(int)b->locked_to].boxcar_index;
		if (b->chain)
			b->chain = (edict_t *) BoxCar_Passengers[(int)b->chain].boxcar_index;
		if (b->enemy)
			b->enemy = (edict_t *) BoxCar_Passengers[(int)b->enemy].boxcar_index;
		if (b->oldenemy)
			b->oldenemy = (edict_t *) BoxCar_Passengers[(int)b->oldenemy].boxcar_index;
		if (b->activator)
			b->activator = (edict_t *) BoxCar_Passengers[(int)b->activator].boxcar_index;
		if (b->groundentity)
			b->groundentity = (edict_t *) BoxCar_Passengers[(int)b->groundentity].boxcar_index;
		if (b->teamchain)
			b->teamchain = (edict_t *) BoxCar_Passengers[(int)b->teamchain].boxcar_index;
		if (b->teammaster)
			b->teammaster = (edict_t *) BoxCar_Passengers[(int)b->teammaster].boxcar_index;
		if (b->mynoise)
			b->mynoise = (edict_t *) BoxCar_Passengers[(int)b->mynoise].boxcar_index;
		if (b->mynoise2)
			b->mynoise2 = (edict_t *) BoxCar_Passengers[(int)b->mynoise2].boxcar_index;
		if (b->goalentity)
			b->goalentity = (edict_t *) BoxCar_Passengers[(int)b->goalentity].boxcar_index;
		if (b->movetarget)
			b->movetarget = (edict_t *) BoxCar_Passengers[(int)b->movetarget].boxcar_index;
		if (b->target_ent)
			b->target_ent = (edict_t *) BoxCar_Passengers[(int)b->target_ent].boxcar_index;
		if (b->cell)	// RSP 100800
			b->cell = (edict_t *) BoxCar_Passengers[(int)b->cell].boxcar_index;
		if (b->owner)
			b->owner = (edict_t *) BoxCar_Passengers[(int)b->owner].boxcar_index;
		if (b->curpath)	// RSP 072499
			b->curpath = (edict_t *) BoxCar_Passengers[(int)b->curpath].boxcar_index;
		if (b->dlockOwner)	// RSP 090100
			b->dlockOwner = (edict_t *) BoxCar_Passengers[(int)b->dlockOwner].boxcar_index;
		if (b->decoy)	// RSP 090100
			b->decoy = (edict_t *) BoxCar_Passengers[(int)b->decoy].boxcar_index;
		if (b->teleportgun_victim)	// RSP 090100
			b->teleportgun_victim = (edict_t *) BoxCar_Passengers[(int)b->teleportgun_victim].boxcar_index;

		// Client structure data (game.clients) isn't wiped at the start of a level.
		// The b->client pointer is still good across levels, because game.clients
		// doesn't move and the client always has the same index number.

/* RSP 092100 - not used
		if (b->client && b->client->pers.currenttoy)
			b->client->pers.currenttoy = (edict_t *) BoxCar_Passengers[(int)b->client->pers.currenttoy].boxcar_index;
 */
		// Change absmin,absmax,origin,old_origin to be relative to the destination func_BoxCar
		VectorAdd(b->absmin,absmin_delta,b->absmin);
		VectorAdd(b->absmax,absmin_delta,b->absmax);
		VectorAdd(b->s.origin,absmin_delta,b->s.origin);
		VectorAdd(b->s.old_origin,absmin_delta,b->s.old_origin);	// RSP 072200

		// Certain entities use pos1 as coordinates, and others use it as a relative distance or
		// angle. Convert the ones that are coordinates.

        if ((Q_stricmp(b->classname,"matrix blast") == 0) || (b->identity == ID_PLASMABOT))	// RSP 062199 - use identity flag
			VectorAdd(b->s.origin,absmin_delta,b->s.origin);

		// Reset the next think time based on new level time
		// BoxCar_Passengers[0].nextthink holds the level.time when the
		// BoxCar was filled with passengers, on the previous map. So all current
		// passenger thinktimes are relative to that and we have to make them relative
		// to the new map's level.time. Add extra FRAMETIME ticks so everybody gets
		// settled before they start thinking.

		// Also reset any other time values

		if (b->think && b->nextthink)
			b->nextthink += t;
		if (b->pain_debounce_time)
			b->pain_debounce_time += t;
		if (b->touch_debounce_time)
			b->touch_debounce_time += t;
		if (b->damage_debounce_time)
			b->damage_debounce_time += t;
		if (b->fly_sound_debounce_time)
			b->fly_sound_debounce_time += t;
		if (b->last_move_time)
			b->last_move_time += t;

		// RSP 092000 - and still more values

		if (b->freetime)
			b->freetime += t;
		if (b->timestamp)
			b->timestamp += t;
		if (b->powerarmor_time)
			b->powerarmor_time += t;
		if (b->teleport_time)
			b->teleport_time += t;
		if (b->client)
		{
			if (b->client->flood_locktill)
				b->client->flood_locktill += t;
			if (b->client->pickup_msg_time)
				b->client->pickup_msg_time += t;
			if (b->client->v_dmg_time)
				b->client->v_dmg_time += t;
			if (b->client->next_drown_time)
				b->client->next_drown_time += t;
			if (b->client->fall_time)
				b->client->fall_time += t;
			if (b->client->grenade_time)
				b->client->grenade_time += t;
			for (j = 0 ; j < 10 ; j++)
				if (b->client->flood_when[j])
					b->client->flood_when[j] += t;
			if (b->client->respawn_time)
				b->client->respawn_time += t;
		}

		if (b->monsterinfo.pausetime)
			b->monsterinfo.pausetime += t;
		if (b->monsterinfo.idle_time)
			b->monsterinfo.idle_time += t;
		if (b->monsterinfo.trail_time)
			b->monsterinfo.trail_time += t;
		if (b->monsterinfo.attack_finished)
			b->monsterinfo.attack_finished += t;
		if (b->monsterinfo.search_time)
			b->monsterinfo.search_time += t;

		// RSP 052700 - account for drowning across maps with air_finished

		if (b->air_finished)
			b->air_finished += t;

		// Re-index models because the indexing in the destination map is different
		// from the indexing in the source map. Also re-index sounds where needed.

		if (b->client)
			b->client->ps.gunindex = gi.modelindex(b->client->pers.weapon->view_model);
		else if (b->item)
		{
			if (b->model)
				b->s.modelindex  = gi.modelindex(b->model);
		}
		else
			switch (b->identity)
			{
			case ID_NULL:
				break;
			case ID_SUPERVISOR:
				monster_supervisor_indexing (b);
				break;
			case ID_AMBUSHBOT_DISK:
				monster_ambush_indexing (b);
				b->s.modelindex2 = gi.modelindex("models/monsters/bots/ambush/DiskGun.md2");
				break;
			case ID_AMBUSHBOT_BOLT:
				monster_ambush_indexing (b);
				b->s.modelindex2 = gi.modelindex("models/monsters/bots/ambush/BoltGun.md2");
				break;
			case ID_BATTLEBOT_DISK:
				monster_battle_indexing (b);
				b->s.modelindex2 = gi.modelindex(DISKGUN_RIGHT);
				b->s.modelindex3 = gi.modelindex(DISKGUN_LEFT);
				break;
			case ID_BATTLEBOT_BOLT:
				monster_battle_indexing (b);
				b->s.modelindex2 = gi.modelindex(BOLTGUN_RIGHT);
				b->s.modelindex3 = gi.modelindex(BOLTGUN_LEFT);
				break;
			case ID_BEETLE:
				if (b->message)
				{
					char buffer[MAX_QPATH];

					// Get beetle's model location from 'message' key

					Com_sprintf(buffer,sizeof(buffer),"models/monsters/beetle/%s/tris.md2",b->message);
					b->s.modelindex = gi.modelindex(buffer);
				}
				else
					b->s.modelindex = gi.modelindex(b->model);
				break;
			case ID_BEAMBOT:
				monster_bot_indexing (b);
				b->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Beam.md2");
				break;
			case ID_CLAWBOT:
				monster_bot_indexing (b);
				b->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Claw.md2");
				break;
			case ID_PLASMABOT:
				monster_bot_indexing (b);
				b->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Coil.md2");
				break;
			case ID_FATMAN:
				monster_fatman_indexing(b);
				break;
			case ID_GBOT:
				monster_gbot_indexing (b);
				break;
			case ID_GREAT_WHITE:
				monster_gwhite_indexing (b);
				break;
			case ID_JELLY:
				if (b->flags & FL_SWIM)
					monster_jelly_indexing (b);
				else
					monster_landjelly_indexing (b);
				break;
			case ID_REPEATER:
				monster_repeater_indexing (b);
				break;
			}

		// RSP 090100
		// Resync the frame numbers to the new level.framenum
		if (b->monsterinfo.quad_framenum)
			b->monsterinfo.quad_framenum += f;
		if (b->client)
		{
			if (b->client->quad_framenum)
				b->client->quad_framenum += f;
			if (b->client->superspeed_framenum)
				b->client->superspeed_framenum += f;
			if (b->client->lavaboots_framenum)
				b->client->lavaboots_framenum += f;
			if (b->client->cloak_framenum)
				b->client->cloak_framenum += f;
			if (b->client->vampire_framenum)
				b->client->vampire_framenum += f;
			if (b->client->tri_framenum)
				b->client->tri_framenum += f;
			if (b->client->invincible_framenum)
				b->client->invincible_framenum += f;

			// RSP 092000 - more frame changes

			if (b->client->scuba_next_breathe)
				b->client->scuba_next_breathe += f;
			if (b->client->scuba_last_bubble)
				b->client->scuba_last_bubble += f;

			// RSP 092500 - more changes
			if (b->client->scenario_framenum)
				b->client->scenario_framenum += f;

			// RSP 092900 - more changes
			if (b->client->capture_framenum)
				b->client->capture_framenum += f;

			// RSP 101400 - more changes
			if (b->client->implant_framenum)
				b->client->implant_framenum += f;
		}
		if (b->freeze_framenum)
			b->freeze_framenum += f;
		if (b->teleportgun_framenum)
			b->teleportgun_framenum += f;
		if (b->tportding_framenum)
			b->tportding_framenum += f;

		// RSP 091600 - if this is a beetle, reset his original spawn point
		// and set the next check of his teleport time to 10s into the future.
		// If we'd left this alone, the beetle might try to teleport to a location
		// that makes no sense on this map.
		if (b->identity == ID_BEETLE)
		{
			VectorCopy(b->s.origin,b->move_origin);	// for teleporting when stuck
			b->teleport_time = level.time + 10;
		}

		e = (edict_t *) b->boxcar_index;		// point to g_edicts entry

		// Finished updating the passenger, time to copy him into his edict struct

		*e = *b;			// copy entity information

		e->boxcar_index = 0;					// clear the index

		// let the server rebuild world links for this ent
		memset (&e->area, 0, sizeof(e->area));
		gi.linkentity(e);
	}

	BoxCar_Passengers = NULL;	// array memory will be freed at end of game
}


/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities (char *mapname, char *entities, char *spawnpoint)
{
	edict_t		*ent;
	int			inhibit;
	char		*com_token;
	int			i;
	float		skill_level;

	skill_level = floor (skill->value);
	if (skill_level < 0)
		skill_level = 0;
	else if (skill_level > 3)
		skill_level = 3;
	if (skill->value != skill_level)
		gi.cvar_forceset("skill", va("%f", skill_level));

	SaveClientData ();	// Saves health, etc. so it's not lost in the subsequent g_edicts wipe

	gi.FreeTags (TAG_LEVEL);
	memset (&level, 0, sizeof(level));
	memset (g_edicts, 0, game.maxentities*sizeof (g_edicts[0]));
	// wipe all the tnt_seq's
	memset (g_tnt_seqs, 0, game.maxentities*sizeof(g_tnt_seqs[0])); // RSP 041599: fix savegame crash

	strncpy (level.mapname, mapname, sizeof(level.mapname)-1);
	strncpy (game.spawnpoint, spawnpoint, sizeof(game.spawnpoint)-1);

	// set client fields on player ents
	for (i = 0 ; i < game.maxclients ; i++)
		g_edicts[i+1].client = game.clients + i;

	ent = NULL;
	inhibit = 0;

// parse ents
	while (1)
	{
		// parse the opening brace	
		com_token = COM_Parse (&entities);
		if (!entities)
			break;
		if (com_token[0] != '{')
			gi.error ("ED_LoadFromFile: found %s when expecting {",com_token);

		if (!ent)
			ent = g_edicts;
		else
			ent = G_Spawn ();
		entities = ED_ParseEdict (entities, ent);

		// remove things (except the world) from different skill levels or deathmatch
		if (ent != g_edicts)
		{
			if (deathmatch->value)
			{
				if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
				{
					G_FreeEdict (ent);	
					inhibit++;
					continue;
				}
			}
			else
			{
				if ( /* ((coop->value) && (ent->spawnflags & SPAWNFLAG_NOT_COOP)) || */
					((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill->value == 2) || (skill->value == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD))
					)
				{
					G_FreeEdict (ent);	
					inhibit++;
					continue;
				}
			}

			ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY|SPAWNFLAG_NOT_MEDIUM|SPAWNFLAG_NOT_HARD|SPAWNFLAG_NOT_COOP|SPAWNFLAG_NOT_DEATHMATCH);
		}

		ED_CallSpawn (ent);
	}	

	gi.dprintf ("%i entities inhibited\n", inhibit);

// 3.20
#ifdef DEBUG
	i = 1;
	ent = EDICT_NUM(i);
	while (i < globals.num_edicts) {
		if (ent->inuse != 0 || ent->inuse != 1)
			Com_DPrintf("Invalid entity %d\n", i);
		i++, ent++;
	}
#endif

	G_FindTeams ();

	PlayerTrail_Init ();

	// WPO 100199 Added LUC powerup logic
	luc_powerup_init();
}


//===================================================================

/*
	// cursor positioning
	xl <value>				// X position from the left side of the physical screen
	xr <value>				// X position from the right side of the physical screen
	yb <value>				// Y position from the bottom of the physical screen
	yt <value>				// Y position from the top of the physical screen
	xv <value>				// X position on the virtual screen (virtual screensize is 320x200)
	yv <value>				// Y position on the virtual screen (virtual screensize is 320x200)

	// drawing
	statpic <name>
	pic <stat>				// Icon field, imageindex in stat-array at index 
	num <fieldwidth> <stat>	// Number field, value in stat-array at index 
	string <stat>
	stat_string				// String field, display pickup-string from itemlist-array at 
							// the index that the value in stat-array that index points to.

	// control
	if <stat>
	ifeq <stat> <value>
	ifbit <stat> <value>
	endif
 */

#if 0
char *single_statusbar = 
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
/* RSP 032399 - No armor in LUC
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "
 */

// selected item
"if 6 "
"	xv	250 "	// RSP 091100
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"endif "

// timer
"if 9 "
"	xv	212 "	// RSP 091100
"	num	2	10 "
"	xv	250 "	// RSP 091100
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "

// AJA 19980314 - Scuba air supply
// AJA 19980606 - Repositioned due to slimmer air gauge pics.
"if 19 "		// The index of our STAT value defined in q_shared.h
"	xv	300 "	// X ofset from left of screen for icon	// RSP 091100
"	yb -90 "	// Y offset from bottom of statusbar for icon
"	pic 18 "	// index of picture array to show as icon
"endif "		// Close the if statement

// dreadlock
"if 22 "
  "xr -75 "
  "yt 40 "
  "string \"Dreadlock\" "
"endif "
;
#endif

// New statusbar

char *LUC_single_statusbar = 
"yb	-52 "

// health
"xv	0 "
"pic 0 "
"xv	26 "
"hnum "

// ammo
"if 2 "
"	xv	296 "
"	pic 2 "
"	xv	246 "
"	anum "
"endif "

// picked up item
"if 7 "
"	xv	130 "
"	pic 7 "
"	xv	156 "
"	yb	-44 "
"	stat_string 8 "
"endif "

// timer
"if 9 "
"	xv	0 "
"	yb	-26 "
"	pic	9 "
"	xv	26 "
"	num	3	10 "
"endif "

// Scuba air supply
"if 19 "
"	xv -24 "
"	yb -68 "
"	pic 18 "
"endif "

// dreadlock
"if 22 "
"   xr -75 "
"   yt  40 "
"   string \"Dreadlock\" "
"endif "

"	yb -26 "

// inventory items
"if 24 "
"	xv	140 "
"	pic 24 "
"endif "

"if 25 "
"	xv	166 "
"	pic 25 "
"endif "

"if 26 "
"	xv	192 "
"	pic 26 "
"endif "

"if 27 "
"	xv	218 "
"	pic 27 "
"endif "

"if 21 "	// frame around selected item
"   xv  216 "
"   yb  -28 "
"   pic 21 "
"   yb  -26 "
"endif "

"if 28 "
"	xv	244 "
"	pic 28 "
"endif "

"if 29 "
"	xv	270 "
"	pic 29 "
"endif "

"if 30 "
"	xv	296 "
"	pic 30 "
"endif "
;


char *dm_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
/* RSP 032399 - No armor in LUC
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "
 */

// selected item
"if 6 "
"	xv	250 "	// RSP 091100
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	212 "	// RSP 091100
"	num	2	10 "
"	xv	250 "	// RSP 091100
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "	// WPO 19990628 - fix constant SPECTATOR MODE string bug in DM

// spectator (3.20)
"if 17 "
  "xv 0 "
  "yb -58 "
  "string2 \"SPECTATOR MODE\" "
"endif "

// chase camera (3.20)
"if 16 "
  "xv 0 "
  "yb -68 "
  "string \"Chasing\" "
  "xv 64 "
  "stat_string 16 "
"endif "

// AJA 19980606 - Scuba air supply gauge for the DM status bar.
"if 19 "		// The index of our STAT value defined in q_shared.h
"	xv	300 "	// X ofset from left of screen for icon	// RSP 091100
"	yb -90 "	// Y offset from bottom of statusbar for icon
"	pic 18 "	// index of picture array to show as icon
"endif "		// Close the if statement

// WPO 19990628 - Target Identification
"if 20 "
  "xv 120 "
  "yv 110 "
  "stat_string 20 "
"endif "

//  WPO 281099 deathmatch rank
"if 23 "
  "xv 120 "
  "yt 2 "
  "num 3 23 "
"endif "
;

// RSP 032399
// Read a table of monster hitpoints and weapon damage if it exists.
// For debugging purposes

typedef struct
{
	char *name;
	int   table;	// 0 = hit point table, 1 = weapon damage table
	int	  index;
} hpwd_t;

hpwd_t hpwdt[] =
{
	{"mh_landjelly",	0,HP_LANDJELLY},
	{"mh_waterjelly",	0,HP_WATERJELLY},
	{"mh_gbot",			0,HP_GBOT},
	{"mh_coilbot",		0,HP_COILBOT},
	{"mh_beambot",		0,HP_BEAMBOT},
	{"mh_clawbot",		0,HP_CLAWBOT},
	{"mh_shark",		0,HP_SHARK},
	{"mh_supervisor",	0,HP_SUPERVISOR},
	{"mh_fatman",		0,HP_FATMAN},
	{"mh_repeater",		0,HP_REPEATER},
	{"mh_probebot",		0,HP_PROBEBOT},
	{"mh_turret",		0,HP_TURRET},
	{"mh_littleboy",	0,HP_LITTLEBOY},		// RSP 051799
	{"mh_ambush",		0,HP_AMBUSH},			// RSP 071299
	{"mh_battle",		0,HP_BATTLE},			// RSP 071299
	{"mh_beetle",		0,HP_BEETLE},			// RSP 072400
	{"wd_knife",		1,WD_KNIFE},
	{"wd_spear",		1,WD_SPEARGUN},
	{"wd_plasma",		1,WD_PLASMAWAD},
	{"wd_disk",			1,WD_DISKGUN},
	{"wd_littleboy",	1,WD_LITTLEBOY},
	{"wd_rocketspear",	1,WD_ROCKET_SPEAR},		// RSP 051399
	{"wd_bolt",			1,WD_BOLTGUN},			// RSP 051799
	{"wd_matrix",		1,WD_MATRIX},			// RSP 051999
	{"rd_disk",			1,WD_DISKGUNBLAST},
	{"rd_rocketspear",	1,WD_ROCKET_SPEAR_BLAST},	// RSP 051399
	{"rd_littleboy",	1,WD_LITTLEBOYBLAST},		
	{"wd_battle_kick",	1,WD_BATTLE_KICK},		// MRB 091699
	{"wd_ambush_hit",	1,WD_AMBUSH_HIT},		// MRB 091699
	{NULL,0}
};


void GetHP()
{
	FILE	*f;
	char	str[32];
	int		value;
	char	name[MAX_OSPATH];
	cvar_t	*game;
	int		i,n;

	game = gi.cvar("game", "", 0);

	if (!*game->string)
		sprintf (name, "%s/hpwdtabs.txt", GAMEVERSION);
	else
		sprintf (name, "%s/hpwdtabs.txt", game->string);

	gi.cprintf (NULL, PRINT_HIGH, "Reading hit points and weapon damage from %s\n", name);

	f = fopen (name, "r");
	if (!f)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}

	while (true)
	{
		n = fscanf(f,"%s %d\n",str,&value);
		if (n <= 0)
			break;
		for (i = 0 ; hpwdt[i].name ; i++)
			if (strcmp(hpwdt[i].name,str) == 0)
			{
				if (hpwdt[i].table == 0)
					hp_table[hpwdt[i].index] = value;	// hit point table
				else
					wd_table[hpwdt[i].index] = value;	// weapon damage table
				break;
			}
	}

	fclose(f);
}


/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"	environment map name
"skyaxis"	vector axis for rotating sky
"skyrotate"	speed of rotation in degrees/second
"sounds"	music cd track number
"gravity"	800 is default gravity
"message"	text to print at user logon
*/
void SP_worldspawn (edict_t *ent)
{
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;			// since the world doesn't use G_Spawn()
	ent->s.modelindex = 1;		// world model is always index 1

	//---------------

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue ();

	// RSP 032399 - initialize monster hit points and weapon damage
	// in a user-provided file. File is called luc/hpwdtabs.txt. If it's
	// not there, the defaults will be used. This is to aid us in balancing
	// monsters and weapons.
	//
	// When releasing to the public, comment out the call to GetHP() and the game
	// will not use this file.
	GetHP();

	// set configstrings for items
	SetItemNames ();

	if (st.nextmap)
		strcpy (level.nextmap, st.nextmap);

	// make some data visible to the server

	if (ent->message && ent->message[0])
	{
		gi.configstring (CS_NAME, ent->message);
		strncpy (level.level_name, ent->message, sizeof(level.level_name));
	}
	else
		strncpy (level.level_name, level.mapname, sizeof(level.level_name));

	if (st.sky && st.sky[0])
		gi.configstring (CS_SKY, st.sky);
	else
		gi.configstring (CS_SKY, "luc1");

	gi.configstring (CS_SKYROTATE, va("%f", st.skyrotate) );

	gi.configstring (CS_SKYAXIS, va("%f %f %f",
		st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]) );

	gi.configstring (CS_CDTRACK, va("%i", ent->sounds) );

	gi.configstring (CS_MAXCLIENTS, va("%i", (int)(maxclients->value) ) );

	// status bar program
	gi.configstring (CS_STATUSBAR, LUC_single_statusbar);
	
	//---------------

	// help icon for statusbar
	gi.imageindex ("i_help");
	level.pic_health = gi.imageindex ("i_health");
	gi.imageindex ("help");
	gi.imageindex ("field_3");

	if (!st.gravity)
		gi.cvar_set("sv_gravity", "800");
	else
		gi.cvar_set("sv_gravity", st.gravity);

	// ----------------------------------------------------------------------
	// Index sounds

	snd_fry = gi.soundindex ("player/fry.wav");	// standing in lava / slime

//	PrecacheItem (FindItem ("Blaster"));		// RSP 032399 - Blaster not in LUC

	// sexed sounds
	gi.soundindex ("*death1.wav");
	gi.soundindex ("*death2.wav");
	gi.soundindex ("*death3.wav");
	gi.soundindex ("*death4.wav");
	gi.soundindex ("*fall1.wav");
	gi.soundindex ("*fall2.wav");	
	gi.soundindex ("*gurp1.wav");		// drowning damage
	gi.soundindex ("*gurp2.wav");	
	gi.soundindex ("*jump1.wav");		// player jump
	gi.soundindex ("*pain25_1.wav");
	gi.soundindex ("*pain25_2.wav");
	gi.soundindex ("*pain50_1.wav");
	gi.soundindex ("*pain50_2.wav");
	gi.soundindex ("*pain75_1.wav");
	gi.soundindex ("*pain75_2.wav");
	gi.soundindex ("*pain100_1.wav");
	gi.soundindex ("*pain100_2.wav");

	// player sounds for going in and out of water

	gi.soundindex ("player/gasp1.wav");		// gasping for air
	gi.soundindex ("player/gasp2.wav");		// head breaking surface, not gasping
	gi.soundindex ("player/watr_in.wav");	// feet hitting water
	gi.soundindex ("player/watr_out.wav");	// feet leaving water
	gi.soundindex ("player/watr_un.wav");	// head going under water
	
	// AJA 19980311 - Scuba breathing sounds

	gi.soundindex ("items/scuba/w_breath1.wav");	// wet inhale
	gi.soundindex ("items/scuba/w_breath2.wav");	// wet exhale
	gi.soundindex ("items/scuba/d_breath1.wav");	// dry inhale
	gi.soundindex ("items/scuba/d_breath2.wav");	// dry exhale

	// misc. sounds

	gi.soundindex ("items/pkup.wav");		// bonus item pickup
	gi.soundindex ("misc/pc_up.wav");		// help computer wants attention
	gi.soundindex ("misc/talk1.wav");		// accompanies messages put to screen
	gi.soundindex ("misc/udeath.wav");		// death sound
	gi.soundindex ("misc/h2ohit1.wav");		// landing splash
	gi.soundindex ("misc/lasfly.wav");		// blaster bolt flying by
	gi.soundindex ("player/lava1.wav");		// player stuck in lava
	gi.soundindex ("player/lava2.wav");		// player stuck in lava, second version
	gi.soundindex ("weapons/noammo.wav");	// out of ammo
	gi.soundindex ("world/land.wav");		// landing thud
	gi.soundindex ("world/electro.wav");	// sparks

	// ----------------------------------------------------------------------
	// Index models

	// sexed models (3.20)
	// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
	// you can add more, max 15
	// these are the weapon models another player is holding when he goes by
	gi.modelindex ("#w_knife.md2");
	gi.modelindex ("#w_speargun.md2");
	gi.modelindex ("#w_diskgn.md2");
	gi.modelindex ("#w_plasmawad.md2");
	gi.modelindex ("#w_boltgn.md2");		// RSP 051599
	gi.modelindex ("#w_shotgun.md2");		// RSP 081800
	gi.modelindex ("#w_teleportgun.md2");	// RSP 081800
	gi.modelindex ("#w_frezgn.md2");		// RSP 081800
	gi.modelindex ("#w_rifle.md2");			// RSP 081800
	gi.modelindex ("#w_matrix.md2");		// RSP 051999
	gi.modelindex ("#a_grenades.md2");		// RSP 081800

	// gibs

//	sm_meat_index = gi.modelindex ("models/objects/gibs/sm_meat/tris.md2");	// RSP 092100 - no longer needed
	gi.modelindex ("models/objects/gibs/bone/tris.md2");
	gi.modelindex ("models/objects/gibs/skull/tris.md2");
	gi.modelindex ("models/objects/gibs/head2/tris.md2");
	gi.modelindex ("models/objects/gibs/tissue/tris.md2");
	gi.modelindex ("models/objects/gibs/gear/tris.md2");
	gi.modelindex ("models/objects/gibs/sm_metal/tris.md2");

	// RSP 052899 - matrix ball
	gi.modelindex ("models/items/ammo/matrix/tris.md2");

	// ----------------------------------------------------------------------
	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.

	// 0 normal
	gi.configstring(CS_LIGHTS+0, "m");
	
	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS+1, "mmnmmommommnonmmonqnmmo");
	
	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS+2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS+3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS+4, "mamamamamama");
	
	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS+5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS+6, "nmonqnmomnmomomno");
	
	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS+7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS+8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS+9, "aaaaaaaazzzzzzzz");
	
	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS+10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring(CS_LIGHTS+11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configstring(CS_LIGHTS+63, "a");

	// RSP 091500 - styles 70-120 are assigned to killable lights

	// ----------------------------------------------------------------------
	// WPO 100199 Powerups
	gi.configstring(CS_POWERUPS,   POWERUP_MSHIELD_NAME);
	gi.configstring(CS_POWERUPS+1, POWERUP_LAVABOOTS_NAME);
	gi.configstring(CS_POWERUPS+2, POWERUP_DECOY_NAME);
	gi.configstring(CS_POWERUPS+3, POWERUP_WARP_NAME);			// WPO 071199 - Space Warp
	gi.configstring(CS_POWERUPS+4, POWERUP_TELEPORTER_NAME);	// WPO 301099 - Teleporter
	gi.configstring(CS_POWERUPS+5, POWERUP_SUPERSPEED_NAME);	// WPO 061199 - Super Speed
	gi.configstring(CS_POWERUPS+6, POWERUP_CLOAK_NAME);			// WPO 061199 - Cloak
	gi.configstring(CS_POWERUPS+7, POWERUP_VAMPIRE_NAME);		// WPO 231099 - Vampire
}

