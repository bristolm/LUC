#include "g_local.h"
#include "m_player.h"
#include "luc_hbot.h"		// WPO 081099 - dreadlock
#include "luc_powerups.h"	// WPO 100199 - Added powerups
#include "luc_decoy.h"		// WPO 171099 - Holographic decoy
#include "luc_teleporter.h" // WPO 301099 - personal teleporter

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	// 3.20
	if (cl->chase_target)
	{
		ChaseNext(ent);
		return;
	}
	// 3.20 end

	// scan  for the next valid one
	for (i = 1 ; i <= MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	// 3.20
	if (cl->chase_target)
	{
		ChasePrev(ent);
		return;
	}
	// 3.20 end

	// scan  for the next valid one
	for (i = 1 ; i <= MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/

// RSP 091200
// Added the IT_DONTGIVE flag to items so that when you do a 'give all' cheat, the
// items that you normally wouldn't encounter are not given to you. This was
// causing strange things like "Health" to show up in the inventory
// list when you hit the TAB key.

void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	// Special handling for health

	if (give_all || (Q_stricmp(gi.argv(1), "health") == 0))
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	// Give weapons

	if (give_all || (Q_stricmp(name, "weapons") == 0))
	{
		for (i = 0 ; i < game.num_items ; i++)
		{
			it = itemlist + i;
			if (it->flags & IT_DONTGIVE)	// RSP 091200
				continue;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] = 1;	// RSP 091200 - you only get one
		}
		if (!give_all)
			return;
	}

	// Give ammo

	if (give_all || (Q_stricmp(name, "ammo") == 0))
	{
		for (i = 0 ; i < game.num_items ; i++)
		{
			it = itemlist + i;
			if (it->flags & IT_DONTGIVE)	// RSP 091200
				continue;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	// Give All, for things other than health, weapons, or ammo

	if (give_all)
	{
		for (i = 0 ; i < game.num_items ; i++)
		{
			it = itemlist + i;
			if (it->flags & IT_DONTGIVE)	// RSP 091200
				continue;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_WEAPON|IT_AMMO))	// RSP 032399 - No armor in LUC
				continue;
			if (Q_stricmp(it->classname,"item_matrixshield") == 0)	// RSP 091200 - don't give matrixshield
				continue;
			if (Q_stricmp(it->classname,"item_holodecoy") == 0)	// RSP 090900 - special handling for decoy
				ent->client->decoy_timeleft = 600;		// RSP 091700
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	// Give individual items

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");	// 3.20
			return;
		}
	}

	if (it->flags & IT_DONTGIVE)	// RSP 091200
	{
		gi.cprintf (ent, PRINT_HIGH, "can't give this item\n");	// 3.20
		return;
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");	// 3.20
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char *msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_KillAll_f

Kills all monsters on the map.

argv(0) killall
==================
*/
void Cmd_KillAll_f (edict_t *ent)
{
	char	*msg;
	edict_t	*item;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	for (item = g_edicts ; item < &g_edicts[globals.num_edicts] ; item++)
		if (item->inuse)
			if (((item->svflags & SVF_MONSTER) && (!(item->identity == ID_PROBEBOT))) || (item->identity == ID_LITTLEBOY))
				T_Damage(item,ent,ent,vec3_origin,ent->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);

	msg = "Kiss 'em goodbye!\n";
	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
// RSP 032299
==================
Cmd_Location_f

Print player location for debugging purposes

argv(0) location
==================
*/
void Cmd_Location_f (edict_t *ent)
{
	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_PRINT_LOCATION;	// No need for a message; you can tell when it's on.
}

/*
// RSP 032699
==================
Cmd_AllBetter_f

Set player health to max allowed.

argv(0) allbetter
==================
*/
void Cmd_AllBetter_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->health = ent->max_health;	// Turn up the health
	msg = "you're feeling better now\n";
	gi.cprintf (ent,PRINT_HIGH,msg);
}

/*
// RSP 061499
==================
Cmd_NoBlend_f

Disable the screen blends for damage, armor. This is a toggle.

argv(0) noblend
==================
*/
void Cmd_NoBlend_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOBLEND;	// Toggle the flag

	if (ent->flags & FL_NOBLEND)
		msg = "damage blends disabled\n";
	else
		msg = "damage blends enabled\n";
	gi.cprintf (ent,PRINT_HIGH,msg);
}

/* RSP 070400 - disabled
// RSP 051899 - temporary warping command

void Cmd_WarpDown_f (edict_t *self)
{

	if (self->client->ps.fov <= 10)
	{
		self->client->ps.fov--;
		if (self->client->ps.fov < 1)
			self->client->ps.fov = 1;
	}
	else if (self->client->ps.fov > 170)
	{
		self->client->ps.fov--;
	}
	else
	{
		self->client->ps.fov -= 10;
		if (self->client->ps.fov < 10)
			self->client->ps.fov = 10;
	}
	gi.dprintf("FOV is %5.2f\n",self->client->ps.fov);
}

// RSP 051899 - temporary warping command

void Cmd_WarpUp_f (edict_t *self)
{

	if (self->client->ps.fov >= 170)
	{
		self->client->ps.fov++;
		if (self->client->ps.fov > 179)
			self->client->ps.fov = 179;
	}
	else if (self->client->ps.fov < 10)
	{
		self->client->ps.fov++;
	}
	else
	{
		self->client->ps.fov += 10;
		if (self->client->ps.fov > 170)
			self->client->ps.fov = 170;
	}
	gi.dprintf("FOV is %5.2f\n",self->client->ps.fov);
}
 */

/*
// RSP 051299
==================
Cmd_Flashlight_f

Toggle flashlight on/off.

argv(0) flashlight
==================
*/
void Cmd_Flashlight_f (edict_t *self)
{
	// RSP 100800 - if captured in the endgame, the flashlight goes off
	if (self->capture_state && (self->endgame_flags & CURRENT_SCENARIO_01))	// RSP 102400
		self->client->pers.flashlight_on = false;
	else
		self->client->pers.flashlight_on = !self->client->pers.flashlight_on;	// toggle

	// Spawn a temporary light entity if flashlight is on

	if (self->client->pers.flashlight_on)
	{
		if (!self->target_ent)
			spawn_beacon(self);
	}
	else if (self->target_ent)	// RSP 061499 - possible flashlight crash fix
	{
		G_FreeEdict(self->target_ent);
		self->target_ent = NULL;
	}
}

/*
// RSP 070400
==================
Cmd_scope_f

Bump up sniper scope

argv(0) scope
==================
 */
void Cmd_Scope_f (edict_t *self)
{
	if (Q_stricmp(self->client->pers.weapon->classname,"weapon_rifle") == 0)
	{
		self->client->ps.fov -= 10;
		if (self->client->ps.fov < 10)
			self->client->ps.fov = 10;
	}
}

/*
// RSP 070400
==================
Cmd_Reset_f

Reset sniper scope

argv(0) reset
==================
 */
void Cmd_Reset_f (edict_t *self)
{
	self->client->ps.fov = 90;
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char *msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char *msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
		if (ent->locked_to)	// RSP 030999 Free from monster's grasp
			if (ent->locked_to->identity == ID_GREAT_WHITE)	// Shark?
				gwhite_ungrab(ent->locked_to);
			else			// Claw
				botUnlockFrom(ent->locked_to, ent);
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();

	// WPO 100199 Added drop powerups option
	if (Q_stricmp(s, "powerups") == 0) 
	{
		luc_drop_powerups(ent);
		return;
	}

	it = FindItem (s);

	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't drop this item.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i = 0 ; i < MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t *it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i = 1 ; i <= MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;
	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);
	// scan  for the next valid one
	for (i = 1 ; i <= MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (!(it->flags & IT_WEAPON))
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t *it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item can't be dropped.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if ((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
	// don't even bother waiting for death frames
	ent->deadflag = DEAD_DEAD;
	respawn (ent);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i,j;	// 3.20
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;		// 3.20

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	// 3.20
	if (flood_msgs->value)
	{
		cl = ent->client;

        if (level.time < cl->flood_locktill)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			(level.time - cl->flood_when[i] < flood_persecond->value))
		{
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}
	// 3.20 end

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
			if (!OnSameTeam(ent, other))
				continue;
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

// AJA 19980308 - This is a toggle variable like god and notarget that turns
// drowning on and off. If won't even make the pain sound when you're underwater
// for extended periods of time like god mode does. This will be useful when
// developing levels with underwater areas. Just type "nodrown" at the console
// to toggle it.
/*
==================
Cmd_Nodrown_f

Toggles client's drowning flag.

argv(0) nodrown
==================
*/
void Cmd_Nodrown_f (edict_t *ent)
{
	char *msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	// Toggle the flag
	ent->flags ^= FL_NODROWN;

	// Check if the flag is set. By default it is off (nodrown == false, drowning ON).
	// Man, inverse logic sucks.
	if (ent->flags & FL_NODROWN)
		// nodrown on, drowning off
		msg = "Drowning OFF\n";
	else
		// nodrown off, drowning on
		msg = "Drowning ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


// 3.20
void Cmd_PlayerList_f(edict_t *ent)
{
	int		i;
	char	st[80];
	char	text[1400];
	edict_t	*e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value ; i++, e2++)
	{
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50)
		{
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}
// 3.20 end


/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char *cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}

/* RSP 091600 - no more help computer
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}
 */

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "flashlight") == 0)// RSP 051299 - add LUC flashlight
		Cmd_Flashlight_f (ent);
	else if (Q_stricmp (cmd, "dreadlock") == 0)	// WPO 081099 - invoke dreadlock
		Cmd_Dreadlock_f (ent);
	else if (Q_stricmp (cmd, "scope") == 0)		// RSP 070400 - add sniper rifle scope
		Cmd_Scope_f(ent);
	else if (Q_stricmp (cmd, "reset") == 0)		// RSP 070400 - add sniper rifle scope reset
		Cmd_Reset_f(ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	// AJA 19980308 - The "drown" console toggle that turns drowning on and off.
	else if (Q_stricmp (cmd, "nodrown") == 0)
		Cmd_Nodrown_f (ent);
	else if (Q_stricmp (cmd, "location") == 0)	// RSP 032299 debugging aid
		Cmd_Location_f (ent);
	else if (Q_stricmp (cmd, "allbetter") == 0)	// RSP 032699 debugging aid
		Cmd_AllBetter_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)	// 3.20
		Cmd_PlayerList_f(ent);					// 3.20
/* RSP 070400 - disabled
	else if (Q_stricmp (cmd, "warpup") == 0)	// RSP 051899 - add warp effect (temporary)
		Cmd_WarpUp_f (ent);
	else if (Q_stricmp (cmd, "warpdown") == 0)	// RSP 051899 - add warp effect (temporary)
		Cmd_WarpDown_f (ent);
 */
	else if (Q_stricmp (cmd, "noblend") == 0)	// RSP 061499 - disable screen blends
		Cmd_NoBlend_f (ent);
	else if (Q_stricmp (cmd, "storeteleport") == 0)	// WPO 301099 - personal teleporter
		Cmd_Store_Teleport_f (ent);
	else if (Q_stricmp (cmd, "killall") == 0)	// RSP 080400 - kill all monsters
		Cmd_KillAll_f(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
