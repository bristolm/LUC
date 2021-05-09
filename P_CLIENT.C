#include "g_local.h"
#include "m_player.h"
#include "luc_hbot.h"
#include "luc_powerups.h"  // WPO 100199 Added powerups

extern edict_t *BoxCar_Passengers;	// Array of transported entities
extern char *LUC_single_statusbar;

void ClientUserinfoChanged (edict_t *ent, char *userinfo);

void SP_misc_teleporter_dest (edict_t *ent);

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	if (!coop->value)
		return;
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	SP_misc_teleporter_dest (self);
}


/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/
void SP_info_player_coop(edict_t *self)
{
	if (!coop->value)
	{
		G_FreeEdict (self);
		return;
	}
}


/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}


qboolean IsFemale (edict_t *ent)
{
	char *info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");	// 3.20
	if ((info[0] == 'f') || (info[0] == 'F'))
		return true;
	return false;
}


// 3.20
qboolean IsNeutral (edict_t *ent)
{
	char *info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if ((info[0] != 'f') && (info[0] != 'F') && (info[0] != 'm') && (info[0] != 'M'))
		return true;
	return false;
}

void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int			mod;
	char		*message;
	char		*message2;
	qboolean	ff;

	if (coop->value && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	if (deathmatch->value || coop->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides";
			break;
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			break;
		}
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_HELD_GRENADE:
				message = "tried to put the pin back in";
				break;
			case MOD_HG_SPLASH:
			case MOD_G_SPLASH:
				if (IsNeutral(self))	// 3.20
					message = "tripped on its own grenade";
				else if (IsFemale(self))
					message = "tripped on her own grenade";
				else
					message = "tripped on his own grenade";
				break;
			case MOD_R_SPLASH:
				if (IsNeutral(self))	// 3.20
					message = "blew itself up";
				else if (IsFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			default:
				if (IsNeutral(self))	// 3.20
					message = "killed itself";
				else if (IsFemale(self))
					message = "killed herself";
				else
					message = "killed himself";
				break;
			}
		}
		if (message)
		{
			gi.bprintf (PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			if (deathmatch->value)
				self->client->resp.score--;
			self->enemy = NULL;
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client)
		{
			switch (mod)
			{
			case MOD_SSHOTGUN:
				message = "was blown into tiny chunks by";
				message2 = "'s shotgun";
				break;
			case MOD_GRENADE:
				message = "was popped by";
				message2 = "'s grenade";
				break;
			case MOD_G_SPLASH:
				message = "was shredded by";
				message2 = "'s shrapnel";
				break;
			case MOD_R_SPLASH:
				message = "almost dodged";
				message2 = "'s rocketspear";
				break;
			case MOD_MATRIX_EFFECT:				// RSP 051999
				message = "couldn't hide from";
				message2 = "'s Matrix";
				break;
			case MOD_HANDGRENADE:
				message = "caught";
				message2 = "'s handgrenade";
				break;
			case MOD_HG_SPLASH:
				message = "didn't see";
				message2 = "'s handgrenade";
				break;
			case MOD_HELD_GRENADE:
				message = "feels";
				message2 = "'s pain";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
				break;
			case MOD_SPEARGUN:
				message = "was harpooned by";
				message2 = "'s spear";
				break;
			// RSP 121298 - Support for Disk Gun
			case MOD_DISKGUN:
				message = "was seared by";	// RSP 030999 Disk is now energy ball
				message2 = "'s energy blast";
				break;
			// MRB 022299: Support for knife
			case MOD_KNIFE:
				message = "was gutted by";
				message2 = "'s knife";
				break;
			// MRB 022299: Support for Plasmawad
			case MOD_PLASMAWAD:
				message = "was chewed apart by";
				message2 = "'s plasmawad";
				break;
			// RSP 051399: Support for rocketspears
			case MOD_ROCKET_SPEAR:
				message = "was blasted by";
				message2 = "'s rocket spear";
				break;
			// RSP 051799: Support for boltgun
			case MOD_BOLTGUN:
				message = "was punctured by";
				message2 = "'s boltgun";
				break;
			// WPO 070100: Support for sniper rifle
			case MOD_RIFLE:
				if (IsNeutral(self))
					message = "has a new hole in its body thanks to";
				else if (IsFemale(self))
					message = "has a new hole in her body thanks to";
				else
					message = "has a new hole in his body thanks to";

				message2 = "'s sniper rifle";
				break;
			}
			if (message)
			{
				gi.bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				if (deathmatch->value)
				{
					if (ff)
						attacker->client->resp.score--;
					else
						attacker->client->resp.score++;
				}
				return;
			}
		}
	}

	gi.bprintf (PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);
	if (deathmatch->value)
		self->client->resp.score--;
}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	float		spread;

	if (!deathmatch->value)
		return;

	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp(item->pickup_name,"Knife") == 0))	// RSP 082300 - changed from blaster to knife
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + 10));

	if (item && quad)
		spread = 22.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t dir;

	if (attacker && (attacker != world) && (attacker != self))
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	else if (inflictor && (inflictor != world) && (inflictor != self))
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	// 3.20
	if (dir[0])
		self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
	else
	{
		self->client->killer_yaw = 0;
		if (dir[1] > 0)
			self->client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->client->killer_yaw = -90;
	}
	if (self->client->killer_yaw < 0)
		self->client->killer_yaw += 360;
}

/*
==================
player_die
==================
*/
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// remove linked weapon model

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 1.0;
		LookAtKiller (self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);

		// WPO 100299 drop powerups if we have any
		// RSP 091300 but only if in deathmatch or coop
		if (deathmatch->value || coop->value)
			luc_powerups_drop_dying(self);

		TossClientWeapon (self);
		if (deathmatch->value)
			Cmd_Help_f (self);		// show scores

		// clear inventory (3.20)
		// this is kind of ugly, but it's how we want to handle keys in coop
		for (n = 0 ; n < game.num_items ; n++)
		{
			if (coop->value && itemlist[n].flags & IT_KEY)
				self->client->resp.coop_respawn.inventory[n] = self->client->pers.inventory[n];
			self->client->pers.inventory[n] = 0;
		}
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->tri_framenum = 0;				// RSP 082300 megahealth powerup

	self->client->invincible_framenum = 0;		// used by megahealth
//	self->client->breather_framenum = 0;		// RSP 082400 - not used
//	self->client->enviro_framenum = 0;			// RSP 082400 - not used

	self->flags &= ~FL_POWER_ARMOR;				// 3.20
	self->client->superspeed_framenum = 0;		// WPO 241099 Superspeed powerup
	self->freeze_framenum = 0;					// WPO 241099 Freeze gun
	self->s.effects &= ~EF_QUAD;				// WPO 251099 freeze gun
	self->client->lavaboots_framenum = 0;		// WPO 061199 Lava Boots powerup
	self->client->cloak_framenum = 0;			// WPO 061199 Cloak powerup
	self->client->vampire_framenum = 0;			// WPO 141199 Vampire powerup

	// RSP 092500 - variables used for endgame scenario
	self->client->scenario_framenum = 0;
	self->scenario_state = 0;
	self->endgame_flags = 0;	// RSP 102400

	// RSP 092900 - variables used for endgame scenario
	self->client->capture_framenum = 0;
	self->capture_state = 0;	

	// RSP 101400 - variables used for dreadlock implant
	self->client->implant_framenum = 0;
	self->implant_state = 0;	

	// RSP 090500 - clear teleportgun info
	if (self->teleportgun_victim)
	{
		G_FreeEdict(self->teleportgun_victim);
		self->teleportgun_victim = NULL;
	}
	self->teleportgun_framenum = 0;             
	self->tportding_framenum = 0;

	// clear inventory
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	if (self->health < -40)
	{	// gib
		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0 ; n < 4 ; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowClientHead (self, damage);

		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1-1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death101-1;
				self->client->anim_end = FRAME_death106;
				break;
			case 1:
				self->s.frame = FRAME_death201-1;
				self->client->anim_end = FRAME_death206;
				break;
			case 2:
				self->s.frame = FRAME_death301-1;
				self->client->anim_end = FRAME_death308;
				break;
			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

	self->deadflag = DEAD_DEAD;

	// WPO 281099 compute player rank
	if (deathmatch->value)
	{
		int		i, j, k;
		int		sorted[MAX_CLIENTS];
		int		sortedscores[MAX_CLIENTS];
		edict_t	*cl_ent;
		int		score, total;

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

		for (i = 0 ; i < game.maxclients ; i++)
		{
			for (j = 0 ; j < total ; j++)
			{
				if (sorted[j] == i)
				{
					game.clients[i].resp.rank = j + 1;
					break;
				}
			}
		}
	}

	gi.linkentity (self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client)
{
	gitem_t *item;

	memset (&client->pers, 0, sizeof(client->pers));

	item = FindItem("Knife");			// MRB 021599 - added knife
	client->pers.inventory[ITEM_INDEX(item)] = 1;	// Player always gets a knife

	client->pers.weapon = item;

	client->pers.health			= 100;
	client->pers.max_health		= 100;

	client->pers.max_grenades	= 50;
	client->pers.max_spears		= 64;	// JBM 083198 - Added Speargun Support
	client->pers.max_disks		= 100;	// RSP 121298 - Added Disk Gun Support
	client->pers.max_plasma		= 100;	// MRB 031599 - Added PlasmaWAd support
	client->pers.max_rocket_spears	= 64;	// RSP 051399 - Added rocket spear support
	client->pers.max_bolts		= 200;	// RSP 051799 - Added bolt support
	client->pers.max_icepellets	= 10;	// WPO 111199 - Add ice pellets
	client->pers.max_bullets	= 20;	// WPO 070100 - Add bullets
	client->pers.max_rods		= 10;	// RSP 090100 - Add teleport rods

//	client->pers.max_shells		= 100;	// RSP 082400 - not used
//	client->pers.max_rockets	= 50;	// RSP 082400 - not used
//	client->pers.max_cells		= 200;	// RSP 082400 - not used
//	client->pers.max_slugs		= 50;	// RSP 082400 - not used

	client->pers.dreadlock = DLOCK_UNAVAILABLE;		// WPO 081099 - dreadlock default to not available
	client->pers.dreadlock_health = DLOCK_MAX_HEALTH;

	client->pers.connected = true;
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;
}

/*
==================
SaveClientData

Some information that should be persistant, like health, 
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i = 0 ; i < game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags = (ent->flags & (FL_GODMODE|FL_NOTARGET|FL_POWER_ARMOR));	// 3.20
		if (coop->value)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;	// 3.20
	if (coop->value)
		ent->client->resp.score = ent->client->pers.score;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;

	bestplayerdistance = 9999999;

	for (n = 1 ; n <= maxclients->value ; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if ((spot == spot1) || (spot == spot2))
			selection++;
	} while (selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint (void)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;


	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot (spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

edict_t *SelectDeathmatchSpawnPoint (void)
{
	if ((int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint ();
	else
		return SelectRandomDeathmatchSpawnPoint ();
}


edict_t *SelectCoopSpawnPoint (edict_t *ent)
{
	int		index;
	edict_t	*spot = NULL;
	char	*target;

	index = ent->client - game.clients;

	// player 0 starts in normal player spawn point
	if (!index)
		return NULL;

	spot = NULL;

	// assume there are four coop spots at each spawnpoint
	while (1)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_coop");
		if (!spot)
			return NULL;	// we didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";
		if ( Q_stricmp(game.spawnpoint, target) == 0 )
		{	// this is a coop spawn point for one of the clients here
			index--;
			if (!index)
				return spot;		// this is it
		}
	}

	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t	*spot = NULL;

	if (deathmatch->value)
		spot = SelectDeathmatchSpawnPoint ();
	else if (coop->value)
		spot = SelectCoopSpawnPoint (ent);

	// find a single player start spot
	if (!spot)
	{
		while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL)
		{
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}

		if (!spot)
		{
			if (!game.spawnpoint[0])
			{	// there wasn't a spawnpoint without a target, so use any
				spot = G_Find (spot, FOFS(classname), "info_player_start");
			}
			if (!spot)
				gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);
}

//======================================================================


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i = 0 ; i < BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;

	if (self->health < -40)
	{
		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self->s.origin[2] -= 48;
		ThrowClientHead (self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

void CopyToBodyQue (edict_t *ent)
{
	edict_t *body;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;
	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity (body);
}


void respawn (edict_t *self)
{
	if (deathmatch->value || coop->value)
	{
		// spectator's don't leave bodies (3.20)
		if (self->movetype != MOVETYPE_NOCLIP)
			CopyToBodyQue (self);
		self->svflags &= ~SVF_NOCLIENT;

		PutClientInServer (self);

		// add a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}

	// restart the entire server
	gi.AddCommandString ("menu_loadgame\n");
}

// 3.20
/* 
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn (edict_t *ent)
{
	int i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator) {
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "spectator");
		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0 ; i <= maxclients->value ; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	}
	else
	{
		// he was a spectator and wants to join the game
		// he must have the right password
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "password");

		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent->client->resp.score = ent->client->pers.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator)
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator) 
		gi.bprintf (PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	else
		gi.bprintf (PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}

//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer (edict_t *ent)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (ent, spawn_origin, spawn_angles);

	index = ent-g_edicts-1;
	client = ent->client;

	// deathmatch wipes most client data every spawn
	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		ClientUserinfoChanged (ent, userinfo);
	}
	else if (coop->value)
	{
//		int		n;	// 3.20
		char	userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		// this is kind of ugly, but it's how we want to handle keys in coop
// 3.20
//		for (n = 0 ; n < game.num_items ; n++)
//		{
//			if (itemlist[n].flags & IT_KEY)
//				resp.coop_respawn.inventory[n] = client->pers.inventory[n];
//		}
		resp.coop_respawn.game_helpchanged = client->pers.game_helpchanged;	// 3.20
		resp.coop_respawn.helpchanged = client->pers.helpchanged;		// 3.20
		client->pers = resp.coop_respawn;
		ClientUserinfoChanged (ent, userinfo);
		if (resp.score > client->pers.score)
			client->pers.score = resp.score;
	}
	else
	{
		memset (&resp, 0, sizeof(resp));
	}

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	if (client->pers.health <= 0)
		InitClientPersistant(client);
	client->resp = resp;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = WL_DRY;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;

	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);

	// clear playerstate values
	memset (&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	// clear entity state values
	ent->s.effects = 0;

	// 3.20
	ent->s.modelindex = 255;		// will use the skin specified model
	ent->s.modelindex2 = 255;		// custom gun model
	// s.skinnum is player num and weapon number
	// weapon number will be added in changeweapon
	ent->s.skinnum = ent - g_edicts - 1;
	// 3.20 end

	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	// spawn a spectator (3.20)
	if (client->pers.spectator)
	{
		client->chase_target = NULL;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		return;
	}
	else
		client->resp.spectator = false;

	if (!KillBox (ent))
	{	// could't spawn in?
	}

	gi.linkentity (ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);

	// RSP 061699 - spawn a flashlight if we need to
	if (client->pers.flashlight_on)
		spawn_beacon(ent);

    // RSP 070600 - spawn a Dreadlock if we need to
    if (client->pers.dreadlock == DLOCK_LAUNCHED)
        Launch_Dreadlock(ent,false);	// RSP 092900 - added boolean

	// RSP 070600 - correctly initialize the player trail
	// ID's code just left the first point at [0,0,0].
	PlayerTrail_Add(ent->s.origin);

	// RSP 091900 - initialize the inventory display by forcing a "select next item"
	// followed by a "select prev item". The current selected item in the player's
	// inventory isn't set until one of these occurs.
	SelectNextItem(ent,-1);
	SelectPrevItem(ent,-1);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch (edict_t *ent)
{
	G_InitEdict (ent);

	InitClientResp (ent->client);

	// locate ent at a spawn point
	PutClientInServer (ent);

	// 3.20
	if (level.intermissiontime)
	{
		MoveClientToIntermission (ent);
	}
	else
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	// 3.20 end

	gi.bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{
	int i;

	// RSP 061999 - Add BoxCar passengers if there are any
	if (BoxCar_Passengers)
		Add_BoxCar();	// RSP 042000 - moved Add_BoxCar() to here

	ent->client = game.clients + (ent - g_edicts - 1);

	if (deathmatch->value)
	{
		ClientBeginDeathmatch (ent);
		return;
	}

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch

	if (ent->inuse == true)
	{
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		for (i = 0 ; i < 3 ; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
	}
	else
	{
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		G_InitEdict (ent);
		ent->classname = "player";
		InitClientResp (ent->client);
		PutClientInServer (ent);
	}

	if (level.intermissiontime)
	{
		MoveClientToIntermission (ent);
	}
	else
	{
		// send effect if in a multiplayer game
		if (game.maxclients > 1)
		{
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (ent-g_edicts);
			gi.WriteByte (MZ_LOGIN);
			gi.multicast (ent->s.origin, MULTICAST_PVS);

			gi.bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
		}
	}

	ClientEndServerFrame (ent);	// make sure all view stuff is valid
}


/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	char	*s;
	int		playernum;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy (userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");
	strncpy (ent->client->pers.netname, s, sizeof(ent->client->pers.netname)-1);

	// set spectator (3.20)
	s = Info_ValueForKey (userinfo, "spectator");
	// spectators are only supported in deathmatch
	if (deathmatch->value && *s && strcmp(s, "0"))
		ent->client->pers.spectator = true;
	else
		ent->client->pers.spectator = false;

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// WPO - target identification
	// just the player name for target identification
	gi.configstring(CS_PLAYERS+playernum, va("%s", ent->client->pers.netname));

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char *value;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");

	// 3.20
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a spectator
	value = Info_ValueForKey (userinfo, "spectator");
	if (deathmatch->value && *value && strcmp(value, "0")) {
		int i, numspec;

		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < maxclients->value; i++)
			if (g_edicts[i+1].inuse && g_edicts[i+1].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	} else {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}
	// 3.20

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		InitClientResp (ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant (ent->client);
	}

	ClientUserinfoChanged (ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintf ("%s connected\n", ent->client->pers.netname);

	ent->svflags = 0; // make sure we start with known default (3.20)
	ent->client->pers.connected = true;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int playernum;

	if (!ent->client)
		return;

	// WPO 100299 drop powerups if we have any
	// RSP 091300 but only if in deathmatch or coop
	if (deathmatch->value || coop->value)
		luc_powerups_drop_dying(ent);

	gi.bprintf (PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");
	gi.configstring (CS_PLAYERS+playernum, "");		// WPO - target identification
}


//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

unsigned CheckBlock (void *b, int c)
{
	int	v,i;

	v = 0;
	for (i = 0 ; i < c ; i++)
		v+= ((byte *)b)[i];
	return v;
}


void PrintPmove (pmove_t *pm)
{
	unsigned c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}


void print_pm(pmove_t *pm)
{
	printf("pm_type is %d\n",pm->s.pm_type);
	printf("origin is (%d %d %d)\n",pm->s.origin[0],pm->s.origin[1],pm->s.origin[2]);
	printf("velocity is (%d %d %d)\n",pm->s.velocity[0],pm->s.velocity[1],pm->s.velocity[2]);
	printf("flags is %x\n",pm->s.pm_flags);		// ducked, jump_held, etc
	printf("pm_time is %d\n",pm->s.pm_time);	// each unit = 8 ms
	printf("gravity is %d\n",pm->s.gravity);
	printf("delta_angles is (%d %d %d)\n",pm->s.delta_angles[0],pm->s.delta_angles[1],pm->s.delta_angles[2]);
	printf("cmd.msec is %d\n",pm->cmd.msec);
	printf("cmd.buttons is %d\n",pm->cmd.buttons);
	printf("cmd.angles is (%d %d %d)\n",pm->cmd.angles[0],pm->cmd.angles[1],pm->cmd.angles[2]);
	printf("cmd.forwardmove is %d\n",pm->cmd.forwardmove);
	printf("cmd.sidemove is %d\n",pm->cmd.sidemove);
	printf("cmd.upmove is %d\n",pm->cmd.upmove);
	printf("snapinitial is %s\n",pm->snapinitial ? "TRUE" : "FALSE");	// if s has been changed outside pmove
	printf("numtouch is %d\n",pm->numtouch);
	printf("viewangles is %s\n",vtos(pm->viewangles));
	printf("viewheight is %5.2f\n",pm->viewheight);
	printf("mins is %s\n",vtos(pm->mins));
	printf("maxs is %s\n",vtos(pm->maxs));
}


void Handle_Blackouts(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client = ent->client;
	int			i;

	// RSP 092500 - check scenario state
	// This is when the player has finished an endgame scenario and is returning
	// either to the saved return point in the level, or--if they've finished all
	// the scenarios--is moving on to the real end of the game.
	switch (ent->scenario_state)
	{
	case 1:
		if (client->scenario_framenum <= level.framenum)
		{
			ent->scenario_state = 2;
			client->scenario_framenum = level.framenum + SCENARIO_BLACKOUT_LENGTH;
		}
		break;
	case 2:
		if (client->scenario_framenum <= level.framenum)
		{
			gi.unlinkentity(ent);

			// At this point, if the player has completed all the endgame scenarios,
			// we move them forward. If they still have one or more to play, then we
			// move them back to the return point. The code is the same, since
			// ent->cell was set to the correct destination when the player touched
			// the trigger_scenario_end.

			// Move player if ent->cell is valid. If not, then the trigger_scenario_end
			// was missing the location to move to. Fix map and retry.

			if (ent->cell)
			{
				VectorCopy (ent->cell->s.origin, ent->s.origin);
				VectorCopy (ent->cell->s.origin, ent->s.old_origin);
				VectorClear (ent->velocity);
				client->ps.pmove.pm_time = 160>>3;		// hold time
				client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

				// It took about a week to figure out that:
				//
				// client->ps.viewangles = client->ps.pmove.delta_angles + ucmd->angles
				//
				// So when you play, the ucmd->angles get changed when you use the rotational
				// keys and mouse, and you're always baselining off the same ps.pmove.delta_angles value
				// to come up with the new ps.viewangles to give the player a new view.
				//
				// Since we're resetting the data at this point, we have to put ps.viewangles back to
				// where it was at the return point. We have no control over what the engine is going to
				// feed us in ucmd->angles. As far as the engine is concerned, we're just continuing
				// on from the point where we were before we restored the game, so we have to compensate
				// by changing ps.pmove.delta_angles.
				//
				// So then:
				//
				// client->ps.pmove.delta_angles = client->ps.viewangles - ucmd->angles
				//
				// You also see this type of angle compensation when the player teleports.
				for (i = 0 ; i < 3 ; i++)	// Reset player view angles
					client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->cell->s.angles[i] - SHORT2ANGLE(ucmd->angles[i]));

				VectorClear (ent->s.angles);
				VectorClear (client->ps.viewangles);
				VectorClear (client->v_angle);
			}

			gi.linkentity (ent);
			ent->scenario_state = 3;
			client->scenario_framenum = level.framenum + SCENARIO_FADEIN_LENGTH;
		}
		break;
	case 3:
		if (client->scenario_framenum <= level.framenum)
		{
			ent->flags &= ~FL_NODROWN;			// can drown again
			ent->scenario_state = 0;
			gi.configstring (CS_STATUSBAR, LUC_single_statusbar);	// bring back status bar
		}
		break;
	default:
		break;
	}

	// RSP 092900
	// This section takes care of blacking out the player when they're captured in the well,
	// and waking them up again, somewhere else.
	switch (ent->capture_state)
	{
	case 1:
		if (client->capture_framenum <= level.framenum)
		{
			ent->capture_state = 2;
			client->capture_framenum = level.framenum + CAPTURE_BLACKOUT_LENGTH;
		}
		break;
	case 2:
		if (client->capture_framenum <= level.framenum)
		{
			gi.unlinkentity(ent);

			// Move player to destination
			VectorCopy (ent->cell->s.origin, ent->s.origin);
			VectorCopy (ent->cell->s.origin, ent->s.old_origin);
			VectorClear (ent->velocity);
			client->ps.pmove.pm_time = 160>>3;		// hold time
			client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

			// Set orientation angles
			for (i = 0 ; i < 3 ; i++)
				client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->cell->s.angles[i] - SHORT2ANGLE(ucmd->angles[i]));

			VectorClear (ent->s.angles);
			VectorClear (client->ps.viewangles);
			VectorClear (client->v_angle);

			// RSP 102400 - scenario 1 is the one where you're captured and put
			// in a holding cell, with the absorbing laser beam coming your way.
			// This requires that you lose any current powerup effects, turn
			// off the flashlight if on, put down your weapon.
			if (ent->endgame_flags & CURRENT_SCENARIO_01)
			{
				client->pers.scuba_active = false;	// not active

				// turn off the flashlight
				client->pers.flashlight_on = false;
				if (ent->target_ent)
				{
					G_FreeEdict(ent->target_ent);
					ent->target_ent = NULL;
				}

				// Put down weapon
				ent->s.modelindex2 = 0;
				client->weapon_sound = 0;
				client->ps.gunframe = 0;
				client->newweapon = NULL;
				ChangeWeapon (ent);

				// remove powerups
				client->quad_framenum = 0;
				client->tri_framenum = 0;			// megahealth powerup
				client->invincible_framenum = 0;	// used by megahealth
				ent->flags &= ~FL_POWER_ARMOR;
				client->superspeed_framenum = 0;	// Superspeed powerup
				ent->freeze_framenum = 0;			// Freeze gun
				ent->s.effects &= ~EF_QUAD;			// freeze gun
				client->lavaboots_framenum = 0;		// Lava Boots powerup
				client->cloak_framenum = 0;			// Cloak powerup
				client->vampire_framenum = 0;		// Vampire powerup

				// clear teleportgun info
				if (ent->teleportgun_victim)
				{
					G_FreeEdict(ent->teleportgun_victim);
					ent->teleportgun_victim = NULL;
				}
				ent->teleportgun_framenum = 0;             
				ent->tportding_framenum = 0;

			}

			ent->health = 100;						// Give player 100% health
			ent->waterlevel = WL_DRY;
			ent->watertype = 0;
			ent->flags &= ~FL_INWATER;
			ent->flags &= ~FL_NODROWN;
			gi.linkentity (ent);

			ent->capture_state = 3;
			client->capture_framenum = level.framenum + CAPTURE_FADEIN_LENGTH;

			if (client->pers.dreadlock == DLOCK_LAUNCHED) 
        		Dreadlock_recall(client->pers.Dreadlock,false); // recall the dreadlock
		}
		break;
	case 3:
		if (client->capture_framenum <= level.framenum)
			if (ent->endgame_flags & CURRENT_SCENARIO_01)
				ent->capture_state = 4;	// moved to holding cell (scenario 1)
			else
			{
				ent->capture_state = 0;	// free to move around (scenarios 2 and 3)
				ent->cell = NULL;
				gi.configstring (CS_STATUSBAR, LUC_single_statusbar);	// bring back status bar
			}
		break;
	case 4:
		// At this point, the player is trapped inside an energy prison cell.
		// ent->cell still points to the path_corner that brought him here.
		// ent->cell->target is the targetname of the cell location.
		// when the player launches the dreadlock, the dreadlock blows away some switches, which are
		// killtargeted at the path_corner. when it goes away, the player is free to move.

		if (!ent->cell->inuse)	// has path_corner gone away?
		{
			ent->capture_state = 0;		// Free to leave
			ent->cell = NULL;

			// Bring up weapon
			client->newweapon = client->pers.lastweapon;
			ChangeWeapon (ent);
			gi.configstring (CS_STATUSBAR, LUC_single_statusbar);	// bring back status bar
		}
		break;
	default:
		break;
	}

	// RSP 101400
	// This section takes care of blacking out the player when they pick up their
	// first dreadlock. They are implanted with a communications device that allows the
	// CoreAI to talk to them.
	switch (ent->implant_state)
	{
	case 1:
		if (client->implant_framenum <= level.framenum)
		{
			ent->implant_state = 2;
			client->implant_framenum = level.framenum + IMPLANT_BLACKOUT_LENGTH;
			gi.sound(ent, CHAN_VOICE, gi.soundindex("lucworld/implant.wav"), 1, ATTN_NORM, 0);
		}
		break;
	case 2:
		if (client->implant_framenum <= level.framenum)
		{
			ent->implant_state = 3;
			client->implant_framenum = level.framenum + IMPLANT_FADEIN_LENGTH;
			HelpMessage(ent,"5 CoreAI: Implant successful.\nWe can communicate.\n");
		}
		break;
	case 3:
		if (client->implant_framenum <= level.framenum)
		{
			ent->implant_state = 0;
			gi.configstring (CS_STATUSBAR, LUC_single_statusbar);	// bring back status bar
		}
		break;
	default:
		break;
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/

// WPO 141099 - Various changes for Lava Boots
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t		*other;
	int			i,j;
	pmove_t		pm;
	qboolean	lavacheck = false;		// WPO 211099 Lava Boots
	// this should not be a problem especialy for SP
	static int	counter = 0;

	// WPO 281099 Super speed bug fix
	// WPO 241099 super speed powerup
	// PlayerSpeed is the controlling parameter
	// 1.0 = normal speed
	float		PlayerSpeedModifer;
	vec3_t		velo;
	vec3_t		end,forward,right,up;

	// WPO 241099 freeze gun
	if ((ent->freeze_framenum > level.framenum) ||
		 ent->scenario_state	||	// RSP 092600 - for endgame
		 ent->capture_state		||	// RSP 092600 - for endgame
		 ent->implant_state)		// RSP 101400 - for dreadlock implant
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
	}
	else		// no speedup when jumping or in liquid
	if ((ent->client->PlayerSpeed != 1.0) && (ucmd->upmove == 0) && (ent->waterlevel == WL_DRY) && G_CheckGround(ent))
	{
		PlayerSpeedModifer = ent->client->PlayerSpeed * 0.15;
		//Figure out speed
		VectorClear(velo);
		AngleVectors(ent->client->v_angle, forward, right, up);
		VectorScale(forward, ucmd->forwardmove * PlayerSpeedModifer, end);
		VectorAdd(end,velo,velo);
		AngleVectors(ent->client->v_angle, forward, right, up);
		VectorScale(right, ucmd->sidemove * PlayerSpeedModifer, end);
		VectorAdd(end,velo,velo);
		VectorAdd(velo,ent->velocity,ent->velocity);
	}
	// end super speed powerup

	// RSP 083100 - for teleportgun, see if victim storage time is up
	// You can't put this in Weapon_teleportgun(), which is the think routine
	// for the gun, because that only gets called if the teleportgun is the
	// current weapon, and you could still run out of time if you brought
	// the weapon down before releasing the stored victim.
	if (ent->teleportgun_victim && (level.framenum >= ent->teleportgun_framenum))
	{
		edict_t *victim = ent->teleportgun_victim;

		// Kill stored victim and the player

		VectorCopy(ent->s.origin,victim->s.origin);

		ent->teleportgun_victim = NULL;
		ent->teleportgun_framenum = 0;
		ent->tportding_framenum = 0;

		// Kill the player
		T_Damage(ent,victim,victim,vec3_origin,victim->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);

		// Kill the stored entity
		T_Damage(victim,victim,victim,vec3_origin,victim->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);
		return;
	}

	level.current_entity = ent;
	client = ent->client;

	// RSP 092900 - When the player has passed through a trigger_capture brush, he is in the endgame
	// and is currently swimming around in a well. We subtract one unit of health each second, until
	// his health hits 16, when we black him out and move him to an endgame scenario.
	if ((ent->flags & FL_CAPTURED) && (client->capture_framenum <= level.framenum))
	{
		ent->health -= 1;

		client->capture_framenum = level.framenum + 10;

		// If this is scenario 1, launch dreadlock if health is less than 30 and one isn't already launched
		if ((ent->endgame_flags & CURRENT_SCENARIO_01) && (ent->health <= 30))
			if ((client->pers.dreadlock == DLOCK_UNAVAILABLE) || (client->pers.dreadlock == DLOCK_DOCKED))
				Launch_Dreadlock(ent,true);	// give the player a dreadlock and activate it

		if (ent->health <= 16)
		{
			// Start blacking the player out, and prepare to move him

			client->capture_framenum = level.framenum + CAPTURE_FADEOUT_LENGTH;	// frames to fade out
			ent->capture_state = 1;	// 1 = fading out
									// 2 = moving to new location
									// 3 = fading in
									// 4 = locked in energy cell (scenario 1 only)

			gi.configstring (CS_STATUSBAR, "");	// Turn off the status bar
			ent->flags &= ~ FL_CAPTURED;		// done with this section of code
			ent->flags |= FL_NODROWN;			// can't drown at this point
		}
	}

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if ((level.time > level.intermissiontime + 5.0) && (ucmd->buttons & BUTTON_ANY))
			level.exitintermission = true;
		return;
	}

	pm_passent = ent;

	// 3.20
	if (client->chase_target)
	{
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
	}
	else
	{
		// set up for pmove
		memset (&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;
		
		// MRB 11/10/98 Lock off lerping 'cause this is single player - keeps mask centered
		client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;	

		// WPO 211099 Lava Boots
		// WPO 061199 Modified the check
		if (client->lavaboots_framenum > level.framenum)
			lavacheck = G_ContentsCheck(ent, CONTENTS_LAVA);

		if (ent->locked_to && (ent->movetype != MOVETYPE_NOCLIP))	// RSP 030999 NOCLIP out of there!
		{	// No 'lerp'ing
			//client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

			// Clear out movement wishes
			ucmd->forwardmove = 0;
			ucmd->sidemove = 0;
			ucmd->upmove = 0;
		}
		else
		{
			//client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

			// WPO 211099 Lava Boots
			// RSP 100800 - endgame
			if (!lavacheck && (ent->capture_state < 2))
				client->ps.pmove.gravity = sv_gravity->value;
			else
				client->ps.pmove.gravity = 0;
		}
		pm.s = client->ps.pmove;

		// WPO 211099 Lava Boots
		if (!lavacheck)
		{
			for (i = 0 ; i < 3 ; i++)
			{
				pm.s.origin[i] = ent->s.origin[i]*8;
				pm.s.velocity[i] = ent->velocity[i]*8;
			}
		}
		else			// WPO 211099 Lava Boots
		{
			if (ucmd->upmove)
			{
				pm.s.velocity[2] = 1000;
			}
			else
			{
				for (i = 0; i < 3; i++)
				{
					pm.s.origin[i] = ent->s.origin[i]*8;
					
					if (++counter == 20)
					{
						counter = 0;
						if (i != 2)
							pm.s.velocity[i] = 0.0;
					}
					else
						if (i != 2)
							pm.s.velocity[i] = ent->velocity[i]*8;
				}
			}
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapinitial = true;

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		// perform a pmove
		gi.Pmove (&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		// WPO 211099 Lava Boots
		if (lavacheck)
		{
			// super knarly hack - stop z velocity
			// when we touch the lava ...
			if (pm.waterlevel != WL_DRY)
				pm.s.velocity[2] = 0.0;

			// then lie to the engine and say we aren't 
			// *really* in the lava
			pm.waterlevel = WL_DRY;
		}

		for (i = 0 ; i < 3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			ent->velocity[i] = pm.s.velocity[i]*0.125;
		}

		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == WL_DRY))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;

		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}

		// WPO 061199 if cloaked, make invisible
		if (client->cloak_framenum > level.framenum)
			ent->svflags |= SVF_NOCLIENT;
		else
			ent->svflags &= ~SVF_NOCLIENT;

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		// touch other objects
		for (i = 0 ; i < pm.numtouch ; i++)
		{
			other = pm.touchents[i];
			for (j = 0 ; j < i ; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;

			other->touch (other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (client->resp.spectator)
		{
			client->latched_buttons = 0;

			if (client->chase_target)
			{
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				GetChaseTarget(ent);
		}
		else if (!client->weapon_thunk)
		{			
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

	// MRB 08131998 - lock down 'toy' positions whlie we are in here
	// MRB 03061999 - comment out to remove mask from view
	//if (client->pers.currenttoy)
	//	client->pers.currenttoy->use (client->pers.currenttoy, NULL, NULL);

	// 3.20
	if (client->resp.spectator)
	{
		if (ucmd->upmove >= 10)
		{
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (i = 1 ; i <= maxclients->value ; i++)
	{
		other = g_edicts + i;
		if (other->inuse && (other->client->chase_target == ent))
			UpdateChaseCam(other);
	}
	// 3.20 end

	// WPO 082899 - Regen dreadlock health

	if (client->pers.dreadlock == DLOCK_DOCKED)   // dreadlock is inactive
		if (client->pers.dreadlock_health < DLOCK_MAX_HEALTH)
			if (random() < 0.005)
				client->pers.dreadlock_health++;

	Handle_Blackouts(ent,ucmd);
}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	// 3.20
	if (deathmatch->value &&
		(client->pers.spectator != client->resp.spectator) &&
		((level.time - client->respawn_time) >= 5))
	{
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)	// 3.20
		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if (level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
			if (deathmatch->value)
				buttonMask = BUTTON_ATTACK;
			else
				buttonMask = -1;

			if ((client->latched_buttons & buttonMask) ||
				(deathmatch->value && ((int)dmflags->value & DF_FORCE_RESPAWN)))
			{
				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->value)
		if (!visible (ent, PlayerTrail_LastSpot()))
			PlayerTrail_Add (ent->s.old_origin);

	// RSP 091900 - When the player has picked up a megahealth urn, they get extra health. If that
	// health is above 100, it decrements down to 100 at the rate of 1 unit per second.

	if (ent->flags & FL_HEALTH_DECREMENT)
		if (ent->health > ent->max_health)
			{
			if (level.framenum % 10 == 0)
				ent->health -= 1;
			}
		else
			ent->flags &= ~FL_HEALTH_DECREMENT;

	// RSP 110400 - slow death for endgame. Used when asked by Core AI to choose to join the
	// collective and the player wastes time wandering around instead of choosing. This is
	// initiated by passing through a trigger_slowdeath;
	if (ent->flags & FL_SLOWDEATH)
	{
		if (ent->health > 0)
			if (level.framenum % 10 == 0)
				ent->health -= 1;
		if (ent->health <= 0)
		{
			ent->flags &= ~FL_SLOWDEATH;
			ent->flags |= FL_NO_KNOCKBACK;
			ent->die(ent, NULL, NULL, 0, vec3_origin);
		}
	}

	client->latched_buttons = 0;
}
