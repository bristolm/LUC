#include "g_local.h"
#include "luc_powerups.h"	// WPO 100199 Powerup indication
#include "luc_hbot.h"		// RSP 070700

#define DISPLAYED_INVENTORY_ITEMS 7					// RSP 091800 - this should be an odd #
#define SELECTED_INVENTORY_INDEX  3					// RSP 091800
int inventory_status[DISPLAYED_INVENTORY_ITEMS];	// RSP 091800


/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if (deathmatch->value || coop->value)
		ent->client->showscores = true;
	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->tri_framenum = 0;			// RSP 082300 - megahealth

	ent->client->invincible_framenum = 0;	// used by megahealth
//	ent->client->breather_framenum = 0;		// RSP 082400 - not used
//	ent->client->enviro_framenum = 0;		// RSP 082400 - not used

	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;
	ent->client->pers.bolt_heat = 0;		// RSP 051799 - for bolt gun
	ent->client->superspeed_framenum = 0;	// WPO 251099 - for super speed powerup
	ent->freeze_framenum = 0;				// WPO 251099 - for freeze gun
	ent->s.effects &= ~EF_QUAD;				// WPO 251099 - for freeze gun
	ent->client->lavaboots_framenum = 0;	// WPO 061199 Lava Boots powerup
	ent->client->cloak_framenum = 0;		// WPO 061199 Cloak powerup
	ent->client->vampire_framenum = 0;		// WPO 141199 Vampire powerup

	// RSP 092500 - variables used for endgame scenario
	ent->client->scenario_framenum = 0;
	ent->scenario_state = 0;	
	ent->endgame_flags = 0;	// RSP 102400

	// RSP 092900 - variables used for endgame scenario
	ent->client->capture_framenum = 0;
	ent->capture_state = 0;	

	// RSP 101400 - variables used for dreadlock implant
	ent->client->implant_framenum = 0;
	ent->implant_state = 0;	

	// RSP 090500 - clear teleportgun info
	if (ent->teleportgun_victim)
	{
		G_FreeEdict(ent->teleportgun_victim);
		ent->teleportgun_victim = NULL;
	}
	ent->teleportgun_framenum = 0;             
	ent->tportding_framenum = 0;

	ent->viewheight = 0;
	ent->s.modelindex  = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex4 = 0;	// RSP 071599 - typo wasn't catching modelindex4
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value || coop->value)
	{
		DeathmatchScoreboardMessage (ent, NULL);
		gi.unicast (ent, true);
	}
}

void BeginIntermission (edict_t *targ)
{
	int		i, n;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i = 0 ; i < maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			respawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i = 0 ; i < maxclients->value ; i++)
			{
				client = g_edicts + 1 + i;
				if (!client->inuse)
					continue;
				// strip players of all keys between units
				for (n = 0 ; n < MAX_ITEMS ; n++)
				{
					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1;		// go immediately to the next level
			return;
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i = 0 ; i < maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}
}


/*
==================
DeathmatchScoreboardMessage

==================
*/

void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer) 
{ 
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;

	// sort the clients by score
	total = 0;
	
	for (i = 0 ; i < game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;

		score = game.clients[i].resp.score;

		for (j = 0 ; j < total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}

		for (k = total ; k > j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}

		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// print level name and exit rules
	string[0] = 0;

	stringlength = strlen(string);
 
	Com_sprintf(entry, sizeof(entry),
		"xv 107 yv 1 string \"LUC Deathmatch \" "
		"xv 32 yv 9 string2 \"Map: %s \" ",
		level.level_name);

	j = strlen(entry); 
	if (stringlength + j < 1024) 
	{
		strcpy (string + stringlength, entry); 
	
		stringlength += j; 
	}

	Com_sprintf(entry, sizeof(entry), 
		"xv 18 yv 17 string2 \"Pos\" "
		"xv 48 yv 17 string2 \"Player\" " 
		"xv 190 yv 17 string2 \"Frags  Time Ping\" "
		"xv 18 yv 19 string2 \"____________________________________\" ");

	j = strlen(entry); 

	if (stringlength + j < 1024) 
	{ 
		strcpy (string + stringlength, entry); 
		stringlength += j; 
	} 
	
	// The screen is only so big
	if (total > 12) 
		total = 12; 
	
	 for (i = 0 ; i < total ; i++)
	 {
		cl = &game.clients[sorted[i]]; 
		cl_ent = g_edicts + 1 + sorted[i]; 
		
		x = 32; 
		y = 32 + 10 * i; 

		if (cl_ent == ent)
			Com_sprintf(entry, sizeof(entry),
				"xv 18 yv %i string2 \"%2i\" "
				"xv 48 yv %i string2 \"%s\" "
				"xv 175 yv %i string2 \"%3i    %3i  %4i\" ",
				y, i + 1,
				y, cl->pers.netname,
				y, cl->resp.score,
				(level.framenum - cl->resp.enterframe) / 600,
				cl->ping);
		else
			Com_sprintf(entry, sizeof(entry),
				"xv 18 yv %i string \"%2i\" "
				"xv 48 yv %i string \"%s\" "
				"xv 175 yv %i string \"%3i    %3i  %4i\" ",
				y, i + 1,
				y, cl->pers.netname,
				y, cl->resp.score,
				(level.framenum - cl->resp.enterframe) / 600,
				cl->ping);

		j = strlen(entry); 
		if (stringlength + j > 1024) 
			break; 
		strcpy (string + stringlength, entry); 
		stringlength += j; 
	} 
	
	gi.WriteByte (svc_layout); 
	gi.WriteString (string); 
}



// id scoreboard
#if 0
void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total;
	int		picnum;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char	*tag;

	// sort the clients by score
	total = 0;
	for (i = 0 ; i < game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator)	// 3.20
			continue;
		score = game.clients[i].resp.score;
		for (j = 0 ; j < total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k = total ; k > j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// print level name and exit rules
	string[0] = 0;

	stringlength = strlen(string);

	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i = 0 ; i < total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		picnum = gi.imageindex ("i_fixme");
		x = (i >= 6) ? 160 : 0;
		y = 32 + 32 * (i%6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = NULL;
		if (tag)
		{
			Com_sprintf (entry, sizeof(entry),
				"xv %i yv %i picn %s ",x+32, y, tag);
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		// send the layout
		Com_sprintf (entry, sizeof(entry),
			"client %i %i %i %i %i %i ",
			x, y, sorted[i], cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600);
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}
#endif

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
	DeathmatchScoreboardMessage (ent, ent->enemy);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;

	if (!deathmatch->value && !coop->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard (ent);
}

// RSP 092500 - help computer no longer in use
#if 0
/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer (edict_t *ent)
{
	char	string[1024];
	char	*sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// send the layout
	Com_sprintf (string, sizeof(string),
		"xv 32 yv 8 picn help "			// background
		"xv 202 yv 12 string2 \"%s\" "		// skill
		"xv 0 yv 24 cstring2 \"%s\" "		// level name
		"xv 0 yv 54 cstring2 \"%s\" "		// help 1
		"xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ", 
		sk,
		level.level_name,
		game.helpmessage1,
		game.helpmessage2,
		level.killed_monsters, level.total_monsters, 
		level.found_goals, level.total_goals,
		level.found_secrets, level.total_secrets);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}
#endif

/*
==================
// RSP 091600

Help Message & DisplayHelpMessage

Draw a help message, or information, or Core AI ramblings. This is usually a message
that is attached to target brushes or entities.
==================
*/

// Examine first word to see if it's a number. If it is, it's the amount of time
// to display the message. Returns two pieces of information. n = the amount of
// time to display the message, and s_out is the beginning of the remainder of
// the message.
char *Parse_Msg(char *s_in, int *n)
{
	char *s_out = s_in;

	*n = 0;
	while (true)
	{	// if first word is not a number, return n = 0 and the initial string
		if ((*s_out < '0') || (*s_out > '9'))
			if (*s_out == ' ')
				return(++s_out);
			else
				return (s_out);

		*n *= 10;
		*n += *s_out - '0';
		s_out++;
	}
}

// Store the message
void HelpMessage (edict_t *ent, char *message)
{
	int		timer;
	float	ftimer;

	strcpy(game.helpmessage1,Parse_Msg(message, &timer));
	ent->client->showhelp = true;
	if (timer)
		ftimer = (float) timer;	// display message for this many seconds
	else
	{
		ftimer = ((float) strlen(message))/10;	// display message for this many seconds
		if (ftimer < 3)
			ftimer = 3;
	}
	ent->client->help_timer = ftimer;
}

// Display the message
void DisplayHelpMessage (edict_t *ent)
{
	char	string[1024];
	int		n = strlen(game.helpmessage1);

	// send the layout

	if (n <= 30)
		Com_sprintf (string, sizeof(string),
			"xv 0 yb -125 picn helpmess2 "	// background
			"xv 0 yb  -96 cstring2 \"%s\" ",	// message format
			game.helpmessage1);				// message
	else if (n <= 60)
		Com_sprintf (string, sizeof(string),
			"xv 0 yb -125 picn helpmess2 "	// background
			"xv 0 yb -98 cstring2 \"%s\" ",	// message format
			game.helpmessage1);				// message
	else if (n <= 90)
		Com_sprintf (string, sizeof(string),
			"xv 0 yb -125 picn helpmess2 "	// background
			"xv 0 yb  -92 cstring2 \"%s\" ",	// message format
			game.helpmessage1);				// message
	else
		Com_sprintf (string, sizeof(string),
			"xv 0 yb -125 picn helpmess2 "	// background
			"xv 0 yb  -86 cstring2 \"%s\" ",	// message format
			game.helpmessage1);				// message

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))	// 3.20
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;	// 3.20
//	HelpComputer (ent);	// RSP 092500 - no more help computer
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
//	int			index;
//	int			cells;
//	int			power_armor_type;
	// AJA 19980312 - Var for air percentage calculation
	int			scuba_air;
	// AJA 19980314 - Buffer to assemble icon name for air gauge.
	char		air_supply[15];

	// WPO 19990622 - target identification
	vec3_t		start, forward, end;
	trace_t		tr;
	static char message[100];
	gclient_t	*cl = ent->client;	// RSP 091200 - simplify
	int			i,j,*ptr;			// RSP 091800
	gitem_t		*iptr;				// RSP 091800

	//
	// health
	//
	cl->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	cl->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//

	if (!cl->ammo_index /* || !cl->pers.inventory[cl->ammo_index] */)
	{
		cl->ps.stats[STAT_AMMO_ICON] = 0;
		cl->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[cl->ammo_index];

		cl->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		cl->ps.stats[STAT_AMMO] = cl->pers.inventory[cl->ammo_index];
	}
	
	//
	// armor
	//

	/* RSP 032399 - No armor in LUC
	power_armor_type = PowerArmorType (ent);
	if (power_armor_type)
	{
		cells = cl->pers.inventory[ITEM_INDEX(FindItem ("cells"))];
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;
		}
	}

	index = ArmorIndex (ent);
	if (power_armor_type && (!index || (level.framenum & 8) ) )
	{	// flash between power armor and other armor icon
		cl->ps.stats[STAT_ARMOR_ICON] = gi.imageindex ("i_powershield");
		cl->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex (index);
		cl->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		cl->ps.stats[STAT_ARMOR] = cl->pers.inventory[index];
	}
	else
	{
		cl->ps.stats[STAT_ARMOR_ICON] = 0;
		cl->ps.stats[STAT_ARMOR] = 0;
	}
	 */

	//
	// pickup message
	//
	if (level.time > cl->pickup_msg_time)
	{
		cl->ps.stats[STAT_PICKUP_ICON]   = 0;
		cl->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// scuba air supply
	//
	// AJA 19980314 - Scuba air supply stat. Displayed as a graphic. Show
	// only if the player has the scuba gear activated, even if it's empty.
	//

	item = FindItem("Scuba Gear");

	if (cl->pers.show_air_gauge && cl->pers.inventory[ITEM_INDEX(item)])
	{
		// AJA 19980315 - Jacked capacity up to 5 minutes.
		// Assumes max air capacity of 5 minutes (3000 frames)
		scuba_air = cl->pers.inventory[ITEM_INDEX(FindItem("Air"))] / 30; // RSP 010399
		cl->ps.stats[STAT_SCUBA_AIR] = (scuba_air == 0) ? -1 : scuba_air;
		sprintf(air_supply, "p_airgauge_%d", (int)((float)scuba_air/10.0+0.5));
		cl->ps.stats[STAT_SCUBA_ICON] = gi.imageindex (air_supply);
	}
	else
	{
		// If we aren't going to draw the air gauge, clear the scuba air count.
		cl->ps.stats[STAT_SCUBA_AIR]  = 0;
		cl->ps.stats[STAT_SCUBA_ICON] = 0;	// RSP 091100
	}

/* RSP 091100 - it's ok to show this now, since the air gauge has been moved
   away from the timer icon
   // Clear the timer values so that the timer numbers aren't drawn.
		cl->ps.stats[STAT_TIMER_ICON] = 0;	// RSP 091100 - have to clear this as well
		cl->ps.stats[STAT_TIMER] = 0;
 */

	//
	// timers
	//
//	else	// RSP 091100

	// There's only room to display one timer at a time, so this is a priority list of
	// timers. The teleportgun storage timer is at the top, since if you can't see that
	// one, it can lead to your death.

	// RSP 083100 teleportgun storage timer
	if (ent->teleportgun_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_teleport");
		cl->ps.stats[STAT_TIMER] = (ent->teleportgun_framenum - level.framenum)/10;
	}
	// WPO 231199 quad timer
	else if (cl->quad_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
		cl->ps.stats[STAT_TIMER] = (cl->quad_framenum - level.framenum)/10;
	}
	// RSP 082300 tri timer
	else if (cl->tri_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_tri");
		cl->ps.stats[STAT_TIMER] = (cl->tri_framenum - level.framenum)/10;
	}
	// WPO 141199 vampire timer
	else if (cl->vampire_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex (POWERUP_VAMPIRE_ICON);
		cl->ps.stats[STAT_TIMER] = (cl->vampire_framenum - level.framenum)/10;
	}
	// WPO 061199 cloak timer
	else if (cl->cloak_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex (POWERUP_CLOAK_ICON);
		cl->ps.stats[STAT_TIMER] = (cl->cloak_framenum - level.framenum)/10;
	}
	// WPO 241099 superspeed timer
	else if (cl->superspeed_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex (POWERUP_SUPERSPEED_ICON);
		cl->ps.stats[STAT_TIMER] = (cl->superspeed_framenum - level.framenum)/10;
	}
	// WPO 061199 lava boots timer
	else if (cl->lavaboots_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex (POWERUP_LAVABOOTS_ICON);
		cl->ps.stats[STAT_TIMER] = (cl->lavaboots_framenum - level.framenum)/10;
	}
	// WPO 241099 freeze timer
	else if (ent->freeze_framenum > level.framenum)
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_freeze");
		cl->ps.stats[STAT_TIMER] = (ent->freeze_framenum - level.framenum)/10;
	}
	// RSP 091700 holo decoy timer
	else if (ent->decoy && (ent->decoy->timestamp > level.time))
	{
		cl->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_decoy");
		cl->ps.stats[STAT_TIMER] = ent->decoy->timestamp - level.time;
	}
	else
	{
		cl->ps.stats[STAT_TIMER_ICON] = 0;
		cl->ps.stats[STAT_TIMER] = 0;
	}

// RSP 091800 - new inventory display

	// Initialize inventory_status[]
	// inventory_status is an array that holds up to DISPLAYED_INVENTORY_ITEMS indices.
	// Each index can be used with itemlist[] to find the icon of the item to be
	// displayed. The center of the display gets the current selected item. Items
	// prior to that in itemlist[] that the player has in his inventory get displayed
	// to the left. Items after that in itemlist[] that the player has in his inventory
	// get displayed to the right.

	for (j = 0 ; j < DISPLAYED_INVENTORY_ITEMS ; j++)
		inventory_status[j] = -1;

	if (cl->pers.selected_item > 0)
	{
		// Set the selected item in the middle of inventory_status[]

		inventory_status[SELECTED_INVENTORY_INDEX] = cl->pers.selected_item;

		// Search back through inventory to build the icons to the left of the selected item

		i = cl->pers.selected_item - 1;
		j = SELECTED_INVENTORY_INDEX - 1;
		ptr = &(cl->pers.inventory[i]);
		iptr = &itemlist[i];
		for ( ; j >= 0 ; i--, ptr--, iptr--)
		{
			if (i == 0)
				break;
			if (*ptr && !(iptr->flags & IT_NOTININV))
				inventory_status[j--] = i;
		}

		// Search forward through inventory to build the icons to the right of the selected item

		i = cl->pers.selected_item + 1;
		j = SELECTED_INVENTORY_INDEX + 1;
		ptr = &(cl->pers.inventory[i]);
		iptr = &itemlist[i];
		for ( ; j < DISPLAYED_INVENTORY_ITEMS ; i++, ptr++, iptr++)
		{
			if (i == game.num_items)
				break;
			if (*ptr && !(iptr->flags & IT_NOTININV))
				inventory_status[j++] = i;
		}

		// Show the 'item selected' frame
	
		cl->ps.stats[STAT_INV_FRAME] = gi.imageindex("invframe");
	}
	else
		cl->ps.stats[STAT_INV_FRAME] = 0;	// No item selected, selection frame is off

	// Set up the status icons based on what's in inventory_status[]

	for (j = 0 ; j < DISPLAYED_INVENTORY_ITEMS ; j++)
		if (inventory_status[j] > 0)
			cl->ps.stats[STAT_INV1_ICON + j] = gi.imageindex(itemlist[inventory_status[j]].icon);
		else
			cl->ps.stats[STAT_INV1_ICON + j] = 0;

	//
	// layouts
	//
	cl->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if ((cl->pers.health <= 0) || level.intermissiontime || cl->showscores)
			cl->ps.stats[STAT_LAYOUTS] |= 1;
		if (cl->showinventory && (cl->pers.health > 0))
			cl->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (cl->showscores || cl->showhelp)
			cl->ps.stats[STAT_LAYOUTS] |= 1;
		if (cl->showinventory && (cl->pers.health > 0))
			cl->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags
	//
	cl->ps.stats[STAT_FRAGS] = cl->resp.score;

	//
	// help icon / current weapon if not shown
	//
	if (cl->pers.helpchanged && (level.framenum & 8))	// 3.20
		cl->ps.stats[STAT_HELPICON] = gi.imageindex ("i_help");
	else if (((cl->pers.hand == CENTER_HANDED) || (cl->ps.fov > 91)) && cl->pers.weapon)
		cl->ps.stats[STAT_HELPICON] = gi.imageindex (cl->pers.weapon->icon);
	else
		cl->ps.stats[STAT_HELPICON] = 0;

	cl->ps.stats[STAT_SPECTATOR] = 0;	// 3.20

	// WPO 19990622 - target identification
	cl->ps.stats[STAT_TARGETING] = 0;	// RSP 091200 - simplify: moved this up here
	if (deathmatch->value && !cl->showscores)
	{
		cvar_t *tinfo = gi.cvar("target_ident", "1", CVAR_SERVERINFO);
		
		if ((tinfo != NULL) && ((int)tinfo->value > 0))
		{
			// locate our start point by adding our viewheight to our origin
			VectorCopy(ent->s.origin, start);
			start[2] += ent->viewheight;

			// give us our forward viewing angle
			AngleVectors(cl->v_angle, forward, NULL, NULL);

			// multiply this forward unit vector by 8192, 
			// add it to the start position, and store this
			// endpoint in end
			VectorMA(start, 8192, forward, end);

			// trace a straight line, ignoring our entity
			// stop on solid wall or player
			tr = gi.trace(start, NULL, NULL, end, ent, MASK_PLAYERSOLID);

			// if a viable client was found, display the name
			if (tr.ent->client && !(tr.ent->svflags & SVF_NOCLIENT))
				cl->ps.stats[STAT_TARGETING] = CS_PLAYERS + (tr.ent - g_edicts) - 1;
		}
	}

	// end of target identification

	// WPO 100199 Dreadlock indication

	if (cl->pers.dreadlock == DLOCK_UNAVAILABLE)
		cl->ps.stats[STAT_DLOCK] = 0;
	else
		cl->ps.stats[STAT_DLOCK] = 1;

	// WPO 281099 Player rank

	if (deathmatch->value)
		cl->ps.stats[STAT_RANK] = cl->resp.rank;
}

/*
===============
G_CheckChaseStats (3.20)
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	for (i = 1 ; i <= maxclients->value ; i++)
	{
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || (cl->chase_target != ent))
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats (3.20)
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if ((cl->pers.health <= 0) || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && (cl->pers.health > 0))
		cl->ps.stats[STAT_LAYOUTS] |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + (cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}


