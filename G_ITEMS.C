#include "g_local.h"
#include "luc_powerups.h"
#include "luc_decoy.h"		// RSP 091700


qboolean	Pickup_Weapon (edict_t *ent, edict_t *other);
void		Use_Weapon (edict_t *ent, gitem_t *inv);
void		Drop_Weapon (edict_t *ent, gitem_t *inv);

void Weapon_Speargun (edict_t *ent);	// JBM 082998 - Adding support for speargun... 
void Weapon_PlasmaWad (edict_t *ent);	// MRB 121598 - Adding PlasmaWad gun in ..
void Weapon_DiskGun (edict_t *ent);		// RSP 121298 - Adding support for diskgun... 
void Weapon_Knife (edict_t *ent);		// MRB 021599 - Adding in knife support
void Weapon_Boltgun (edict_t *ent);		// RSP 051799 - Adding boltgun support
void Weapon_Matrix (edict_t *ent);		// RSP 051999 - Adding matrix support
void Weapon_Grenade (edict_t *ent);		// WPO 071699 - Adding grenade support
void Weapon_Teleportgun (edict_t *ent);	// WPO 231099 - Adding teleport gun support
void Weapon_Freezegun (edict_t *ent);	// WPO 241099 - Adding freeze gun support
void Weapon_LUCShotgun (edict_t *ent);	// WPO 021199 - Adding LUC shotgun support
void Weapon_Rifle (edict_t *ent);		// WPO 070100 - Adding sniper rifle

/* RSP 082500 - this armor is not in LUC
gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};
static int	jacket_armor_index;
static int	combat_armor_index;
static int	body_armor_index;
static int	power_screen_index;
static int	power_shield_index;
 */

// RSP 032399 - Hit Point table for LUC monsters
int hp_table[] =
{
	200,	// HP_LANDJELLY
	200,	// HP_WATERJELLY
	 50,	// HP_GBOT
	 75,	// HP_COILBOT
	 75,	// HP_BEAMBOT
	 75,	// HP_CLAWBOT
	100,	// HP_SHARK
	150,	// HP_SUPERVISOR
	900,	// HP_FATMAN
	125,	// HP_REPEATER
	200,	// HP_PROBEBOT
	200,	// HP_TURRET
	100,	// HP_LITTLEBOY
	200,	// HP_AMBUSH	// RSP 071299
	200,	// HP_BATTLE	// RSP 071299
	 40		// HP_BEETLE	// RSP 091300
};

// RSP 032399 - Weapon damage table for LUC weapons
int wd_table[] =
{
	 10,	// WD_KNIFE
	 40,	// WD_SPEARGUN
	 50,	// WD_DISKGUN
	  1,	// WD_PLASMAWAD
	 40,    // WD_DISKGUNBLAST
	 80,	// WD_LITTLEBOY
	 80,	// WD_LITTLEBOYBLAST
	 80,	// WD_ROCKET_SPEAR
	 80,	// WD_ROCKET_SPEAR_BLAST
	  8,	// WD_BOLTGUN
	  0,	// WD_MATRIX				// RSP 051999 (here by MRB 091699)
	 10,	// WD_BATTLE_KICK			// MRB 091699
	  5,	// WD_AMBUSH_HIT			// MRB 091699
	100		// WD_RIFLE					// RSP 091900
};

#define HEALTH_IGNORE_MAX	1	// RSP 082300 - restored for megahealth
#define HEALTH_TIMED		2	// RSP 082300 - restored for megahealth


//static int	quad_drop_timeout_hack; // WPO 241099 not used in LUC

//======================================================================


/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if ((index == 0) || (index >= game.num_items))
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i = 0 ; i < game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_stricmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i = 0 ; i < game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

// RSP 082400 - be sure to reset the thinking routines for items that
// have skin and/or frame animations

#define ITEM_TELEPORTER_FRAMES	40	// RSP 082200
#define ITEM_SUPERSPEED_SKINS	18	// RSP 082200
#define ITEM_HEALTH_FRAMES		20	// RSP 082400

// RSP 082200 - item_teleporter now thinks so it can have frame animation

void item_teleporter_think(edict_t *ent)
{
    // Control model animations

	if (++ent->s.frame == ent->count2)
		ent->s.frame = 0;						// reset model frame
    ent->nextthink = level.time + FRAMETIME;
}

// RSP 082200 - item_superspeed now thinks so it can have skin animation

void item_superspeed_think(edict_t *ent)
{
    // Control skin animations

	if (++ent->s.skinnum == ent->count)
		ent->s.skinnum = 0;						// reset skin number
    ent->nextthink = level.time + FRAMETIME;
}

// RSP 082400 - item_health_{mini,midi,maxi} thinks so it can have frame animation

void item_health_think(edict_t *ent)
{
    // Control frame animations

	if (++ent->s.frame == ent->count2)
		ent->s.frame = 0;						// reset frame number
    ent->nextthink = level.time + FRAMETIME;
}


void Setup_thinkers(edict_t *ent)
{
	// RSP 082200 - Add frame animation to item_teleporter
	if (Q_stricmp(ent->classname,"item_teleporter") == 0)
	{
		ent->s.frame = 0;
		ent->count2 = ITEM_TELEPORTER_FRAMES;
		ent->think = item_teleporter_think;
		ent->nextthink = level.time + FRAMETIME;
	}
	// RSP 082200 - Add skin animation to item_superspeed
	else if (Q_stricmp(ent->classname,"item_superspeed") == 0)
	{
		ent->s.skinnum = 0;
		ent->count = ITEM_SUPERSPEED_SKINS;
		ent->think = item_superspeed_think;
		ent->nextthink = level.time + FRAMETIME;
	}
	// RSP 082400 - Add frame animation to item_health
	else if (Q_stricmp(ent->classname,"item_health_urn") == 0)
	{
		ent->s.frame = 0;
		ent->count2 = ITEM_HEALTH_FRAMES;
		ent->think = item_health_think;
		ent->nextthink = level.time + FRAMETIME;
		ent->s.renderfx |= RF_TRANSLUCENT;
		ent->s.renderfx &= ~RF_GLOW;	// Glow must be off for translucency to work
	}
}


void DoRespawn (edict_t *ent)
{
	if (ent->team)
	{
		edict_t	*master;
		int		count;
		int		choice;

		master = ent->teammaster;

		for (count = 0, ent = master ; ent ; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master ; count < choice ; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;

	Setup_thinkers(ent);	// RSP 082400 - handle thinking items

	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


void Drop_General (edict_t *ent, gitem_t *item)
{
	Drop_Item (ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
}


// AJA 19980310 - Pickup function for the scuba gear. Only allow one set of scuba
// gear to be picked up. Air refills will come by picking up new tanks.
// Otherwise this function behaves similar to Pickup_Powerup.
// AJA 19980311 - Pick up other sets of scuba gear if we're low on air.
// RSP 091500 - the scuba gear entity knows how much air it has (ent->count)

qboolean Pickup_ScubaGear (edict_t *ent, edict_t *other)
{
	int	*scubas;	// inventory pointer to the number of scuba gear sets in the player's inventory
	int	*air;		// inventory pointer to the amount of air in the player's scuba tank

	air = &(other->client->pers.inventory[ITEM_INDEX(FindItem("Air"))]);
	scubas = &(other->client->pers.inventory[ITEM_INDEX(ent->item)]);

	// If we already have a scuba, and we have enough air ( >= AIR_LOW), then don't pick up this new one.

	if ((*scubas > 0) && (*air >= AIR_LOW))
		return false;

	// One set of scuba gear in player's inventory.

	*scubas = 1;

	// Add the amount of air this scuba tank is carrying

	*air += ent->count;
	if (*air > AIR_QUANTITY*2)
		*air = AIR_QUANTITY*2;

	if (deathmatch->value)
	{
		// If this isn't a dropped item, set it to respawn.
		if (!(ent->spawnflags & DROPPED_ITEM))
			SetRespawn (ent, ent->item->quantity);
	}

	return true;
}


// RSP 091500 - Drop function for the scuba gear.
// We have to store in the entity the amount of air that was in the tank
// when we dropped.

void Drop_ScubaGear (edict_t *ent, gitem_t *item)
{
	edict_t *scuba = Drop_Item (ent, item);
	int		*air;	// amount of air in scuba tank

	if (!scuba)
		return;

	air = &(ent->client->pers.inventory[ITEM_INDEX(FindItem("Air"))]);
	scuba->count = *air;	// fill up the dropped tank
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;	// no tank
	*air = 0;											// no air
	ent->client->pers.scuba_active = false;				// not active
	ent->client->pers.show_air_gauge = false;			// turn off air gauge
	ValidateSelectedItem (ent);
}


// RSP 091700 - Drop function for the holo decoy.
// We have to store in the entity the amount of activation time that was left
// when we dropped.

void Drop_HoloDecoy (edict_t *ent, gitem_t *item)
{
	edict_t *decoy = Drop_Item (ent, item);

	if (!decoy)
		return;

	if (ent->decoy)					// is decoy currently deployed?
		decoy_remove(ent->decoy);	// if so, bring it back
	decoy->count = (int) ent->client->decoy_timeleft;	// You get this amount next time you pick it up
	ent->client->decoy_timeleft = 0;					// no decoy in inventory
	ent->client->pers.inventory[luc_powerups[POWERUP_DECOY].itemIndex] = 0;	// no decoy in inventory
	ValidateSelectedItem (ent);
}


//======================================================================
// AJA 19980315 - Pickup function for the air tank item. If the scuba
// tank isn't already full, add 2.5 minutes of air. Max air is 5 minutes.

// RSP 010399: Changed 1500 to AIR_QUANTITY and 3000 to AIR_QUANTITY*2

qboolean Pickup_AirTank (edict_t *ent, edict_t *other)
{
	int *air = &(other->client->pers.inventory[ITEM_INDEX(FindItem("Air"))]);

	if (*air >= AIR_QUANTITY*2)
		return false;

	*air += AIR_QUANTITY;
	if (*air > AIR_QUANTITY*2)
		*air = AIR_QUANTITY*2;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

// MRB 031599
qboolean Pickup_Plasma (edict_t *ent, edict_t *other)
{
	gitem_t *plasma = FindItem("Plasma");
	
	if (ent->count > 0)
		// We may be a partially-used ball.
		return (Add_Ammo(other,plasma,ent->count));
	else
		return (Add_Ammo(other,plasma,ent->item->quantity));
}

// MRB 031599
qboolean Pickup_Normal_Spear (edict_t *ent, edict_t *other)
{
	gitem_t *spear = FindItem("Spears");
	
	return (Add_Ammo(other,spear,ent->item->quantity));
}

// MRB 031599
qboolean Pickup_Rocket_Spear (edict_t *ent, edict_t *other)
{
	gitem_t *spear = FindItem("Rocket Spears");
	
	return (Add_Ammo(other,spear,ent->item->quantity));
}

/* RSP 072200 - not using mask
// Try to keep the mask happy
void mask_align(edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t	u,v;
	int		i;

//	VectorSet(offset,0,0,self->goalentity->viewheight);

//	AngleVectors(self->goalentity->client->ps.viewangles, face, right, NULL);
//	G_ProjectSource(self->goalentity->s.origin, offset, face, right, self->s.origin);

	self->light_level = self->goalentity->light_level;

	for (i = 0 ; i < 3 ; i++)
	{
		u[i] = self->pos1[i] - self->pos2[i];
		self->pos2[i] = self->pos1[i];

		self->s.angles[i] = self->goalentity->client->ps.viewangles[i]
								+ self->goalentity->client->ps.kick_angles[i];
		v[i] = self->s.angles[i] - self->pos1[i];
		self->pos1[i] = self->s.angles[i];
		
		if (v[i] != 0 && u[i] != 0)
			// changing angle - do some guesswork.
			self->s.angles[i] += ((v[i] * 4) - (u[i] * 2));

		self->s.origin[i] = self->goalentity->s.origin[i];
							//	+ self->goalentity->client->ps.viewoffset[i];
	}
	self->s.origin[2] += self->goalentity->viewheight;

	VectorCopy(self->goalentity->velocity,self->velocity);
	VectorCopy(self->goalentity->avelocity,self->avelocity);

	gi.linkentity(self);
}
 */

//======================================================================
// AJA 19980310 - Function called when the player uses his scuba gear.
// AJA 19980311 - If the scuba is already active, deactiviate it, and vice versa.
// AJA 19980312 - Turned off the "you put on/take off the scuba gear" announcements.
// MRB 19980813 - Added in a model for the mask.
// RSP 072200   - Took mask out
void Use_ScubaGear (edict_t *ent, gitem_t *item)
{
//	edict_t *toy;	// RSP 072200 - mask not used

	ValidateSelectedItem (ent);

	if (ent->client->pers.scuba_active)	// RSP 032299
//	if (ent->flags & FL_SCUBA_GEAR)
	{
		// De-activate the scuba gear
		ent->client->pers.scuba_active = false;	// RSP 032299
		
/* RSP 072200 - mask not used
		//MRB remove the mask entity
		ent->client->pers.currenttoy->use = NULL;	// RSP 032299 - moved to persistant
		G_FreeEdict(ent->client->pers.currenttoy);	// RSP 032299 - moved to persistant
		ent->client->pers.currenttoy = NULL;		// RSP 032299 - moved to persistant
 */
	}
	else
	{
		// Activate the scuba gear
		if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Air"))] > 0) // RSP 010399
		{
			ent->client->pers.scuba_active = true;	// RSP 032299
			ent->client->scuba_next_breathe = level.framenum + 5;
			ent->client->scuba_breathe_sound = 0;

/* RSP 072200 - mask not used
			//MRB create the mask entity
			toy = G_Spawn();

			// MRB 030899 - make mask invisible to client
			toy->svflags = SVF_NOCLIENT;

			toy->use = mask_align;
			toy->movetype = MOVETYPE_NONE;
			toy->solid = SOLID_NOT;
			toy->s.renderfx |= (RF_WEAPONMODEL | RF_DEPTHHACK| RF_FRAMELERP);
			toy->goalentity = ent;
			toy->s.modelindex = gi.modelindex (item->view_model);

			VectorSet(toy->mins,-8,-8,-8);
			VectorSet(toy->maxs,8,8,8);
			VectorCopy(ent->s.origin,toy->s.origin);
			toy->s.origin[2] = ent->viewheight;

			VectorCopy(ent->s.angles,toy->s.angles);
			VectorCopy(ent->velocity,toy->velocity);
			VectorCopy(ent->avelocity,toy->avelocity);

			VectorCopy(ent->s.angles,toy->pos1);	// try for prediction
			VectorCopy(ent->s.angles,toy->pos2);

			gi.linkentity(toy);
			ent->client->pers.currenttoy = toy;		// RSP 032299 - moved to persistant
 */
		}
		else
		{
			ent->client->pers.scuba_active = false;	// RSP 032299

			//AJA 19990109 - What if we try to start using the scuba but
			// there's no air, and therefore no currenttoy? We deactivate
			// immediately and check for a NULL currenttoy.
/* RSP 072200 - mask not used
			if (ent->client->pers.currenttoy)	// RSP 032299
			{
				//MRB remove the mask entity
				ent->client->pers.currenttoy->use = NULL;	// RSP 032299 - moved to persistant
				G_FreeEdict(ent->client->pers.currenttoy);	// RSP 032299 - moved to persistant
				ent->client->pers.currenttoy = NULL;		// RSP 032299 - moved to persistant
			}
 */
		}
	}

	// RSP 041199 - display the air gauge if the scuba gear is active.

	ent->client->pers.show_air_gauge = ent->client->pers.scuba_active;
}


qboolean Pickup_Key (edict_t *ent, edict_t *other)
{
	if (coop->value)
	{
		// AJA 19990116 - Don't need this custom power cube code for the moment,
		// but it might be useful later.
		/*
		if (strcmp(ent->classname, "key_power_cube") == 0)
		{
			if (other->client->pers.power_cubes & ((ent->spawnflags & 0x0000ff00)>> 8))
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->client->pers.power_cubes |= ((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else
		{
		*/
			if (other->client->pers.inventory[ITEM_INDEX(ent->item)])
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		//}
		return true;
	}
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}


// RSP 091900 - just a placeholder so a key can be selected using the [ and ] keys
void Use_Key (edict_t *ent, gitem_t *item)
{
}


//======================================================================

qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count)
{
	int index;
	int max;

	if (!ent->client)
		return false;

	if (item->tag == AMMO_SPEARS)
		max = ent->client->pers.max_spears;		// JBM 083198 - Add spears for spearguns

	else if (item->tag == AMMO_DISKS)
		max = ent->client->pers.max_disks;		// RSP 121298 - Add disks for diskguns

	else if (item->tag == AMMO_PLASMA)
		max = ent->client->pers.max_plasma;		// MRB 031599 - Add plasma for Plasmawad and Matrix

	else if (item->tag == AMMO_ROCKET_SPEARS)
		max = ent->client->pers.max_rocket_spears;	// RSP 051399 - Add spears for rocket spears

	else if (item->tag == AMMO_BOLTS)
		max = ent->client->pers.max_bolts;		// RSP 051799 - Add bolts for boltgun

	else if (item->tag == AMMO_GRENADES)
		max = ent->client->pers.max_grenades;	// WPO 072699 - Add grenades

	else if (item->tag == AMMO_ICEPELLETS)
		max = ent->client->pers.max_icepellets;	// WPO 072699 - Add ice pellets

	else if (item->tag == AMMO_BULLETS)
		max = ent->client->pers.max_bullets;	// WPO 070100 - Add bullets

	else if (item->tag == AMMO_RODS)
		max = ent->client->pers.max_rods;		// RSP 090100 - Add teleport rods

	else
		return false;

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
		return false;

	ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
		ent->client->pers.inventory[index] = max;

	return true;
}

qboolean Pickup_Ammo (edict_t *ent, edict_t *other)
{
	int			oldcount;
	int			count;
	qboolean	weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if (weapon && ((int)dmflags->value & DF_INFINITE_AMMO))
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo (other, ent->item, count))
		return false;

	if (weapon && !oldcount)
		if ((other->client->pers.weapon != ent->item) &&
			(!deathmatch->value || (other->client->pers.weapon == FindItem("Knife"))))
		{
			other->client->newweapon = ent->item;
		}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->value))
		SetRespawn (ent, 30);

	return true;
}


// RSP 091900 - just a placeholder so ammo can be selected using the [ and ] keys
void Use_Ammo (edict_t *ent, gitem_t *item)
{
}


void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	int		index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item (ent, item);
	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


// RSP 082300 - modified to allow for megahealth
// When you pick up megahealth, you get 5 seconds of GOD Mode.
// After that, your health deteriorates back down to 100, 1 unit per second.
// You also get 30 seconds of tridamage.

// RSP 090800 - Changed Tridamage timeout to match Quaddamage timeout
#define GODMODE_TIME	 5
#define TRIDAMAGE_TIME	30

qboolean Pickup_Health (edict_t *ent, edict_t *other)
{
	// RSP 081999 - don't take the health item if you're at full health
	// unless it's the megahealth

	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health > other->max_health)
			other->health = other->max_health;

	if (ent->style & HEALTH_TIMED)
	{
		if (other->health > other->max_health)
			other->flags |= FL_HEALTH_DECREMENT;	// RSP 091900 - decrement health to 100 if set
		other->client->tri_framenum = level.framenum + TRIDAMAGE_TIME*10;		// tridamage
		other->client->invincible_framenum = level.framenum + GODMODE_TIME*10;	// god mode
		if (deathmatch->value)
			SetRespawn (ent, 60);	// wait twice as long, since there's an effect to run out
	}
	else
	{
		if (deathmatch->value)
			SetRespawn (ent, 30);
	}

	return true;
}


int PowerArmorType (edict_t *ent)
{
/* 	RSP 082800 - No PowerArmor in LUC

	if (!ent->client)
		return POWER_ARMOR_NONE;

	if (!(ent->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (ent->client->pers.inventory[power_shield_index] > 0)
		return POWER_ARMOR_SHIELD;

	if (ent->client->pers.inventory[power_screen_index] > 0)
		return POWER_ARMOR_SCREEN;
 */

	return POWER_ARMOR_NONE;
}


/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;	

		// show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(ent->item);
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		// 3.20

		if (ent->item->pickup_sound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);

		// 3.20 end
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (!((coop->value) &&  (ent->item->flags & IT_STAY_COOP)) || (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

// RSP 091800
// This is when a dropped teleportgun blows up.
// Kill the stored victim.
void Teleportgun_think(edict_t *ent)
{
	if (ent->teleportgun_victim)
	{
		edict_t *victim = ent->teleportgun_victim;
		int sound = (gi.pointcontents(ent->s.origin) & MASK_WATER) ? EXPLODE_LITTLEBOY_WATER : EXPLODE_LITTLEBOY;

		VectorCopy(ent->s.origin,victim->s.origin);
		T_Damage(victim,victim,victim,vec3_origin,victim->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);
		SpawnExplosion(ent,TE_EXPLOSION1,MULTICAST_PVS,sound);
	}

	G_FreeEdict(ent);
}


//======================================================================

static void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->value)
	{
		ent->nextthink = level.time + 29;
		ent->think = G_FreeEdict;
	}

	// RSP 091800 - If this is the teleportgun, and it's carrying a victim,
	// then you have to blow it up when its timer expires. You don't have to
	// worry about the deathmatch code above, since the teleportgun isn't
	// allowed in deathmatch.

	if ((Q_stricmp(ent->classname,"weapon_teleportgun") == 0) && ent->teleportgun_victim)
	{
		ent->think = Teleportgun_think;
		ent->nextthink = level.time + (ent->teleportgun_framenum - level.framenum)/10;
	}
}


edict_t *Drop_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);

// RSP DEBUG - why is this using gi.setmodel when it should be using dropped->model = "something", followed
// by dropped->s.modelindex = gi.modelindex(dropped->model) ? Huh?

	gi.setmodel (dropped, dropped->item->world_model);

	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;  
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;
	dropped->flags = ent->flags & FL_PASSENGER;	// RSP 061899

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace(ent->s.origin,dropped->mins,dropped->maxs,dropped->s.origin,ent,CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	// RSP 091800 - If this is the teleportgun, and it currently has a stored victim,
	// you have to save how much is left on the timer, so that the gun will still
	// explode when the timer expires.

	if ((Q_stricmp(dropped->classname,"weapon_teleportgun") == 0) && ent->teleportgun_victim)
	{
		dropped->teleportgun_victim = ent->teleportgun_victim;
		dropped->teleportgun_framenum = ent->teleportgun_framenum;
		ent->teleportgun_victim = NULL;
		ent->teleportgun_framenum = 0;             
		ent->tportding_framenum = 0;
	}

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

//======================================================================


/*
================
droptofloor
================
*/
void droptofloor(edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;  
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	Setup_thinkers(ent);	// RSP 082400 - handle thinking items

	gi.linkentity (ent);
}

// RSP 091900 - just a placeholder so the matrix shield can be selected using the [ and ] keys
void Use_MatrixShield (edict_t *ent, gitem_t *item)
{
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && (*s != ' '))
			s++;

		len = s-start;
		if ((len >= MAX_QPATH) || (len < 5))
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "pcx"))
		{
			data[len-4] = 0;	// RSP 032499 - clip the ".pcx" off
			gi.imageindex (data);
		}
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
	PrecacheItem (item);

	if (ent->spawnflags)
		// WPO 230100 - Changed name from key_power_cube
		if (strcmp(ent->classname, "key_explosive") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", ent->classname, vtos(ent->s.origin));
		}

	// RSP 091500 - Scuba Gear gets a half-tank of air when spawned
	if (strcmp(ent->classname, "item_scubagear") == 0)
		ent->count = AIR_QUANTITY;

	// RSP 091700 - Decoy gets a full 60 second timer when spawned
	if (strcmp(ent->classname, "item_holodecoy") == 0)
		ent->count = 600;	// frames

	// RSP 091200 - matrix shield not needed in single player
	if (!deathmatch->value)
	{
		if (strcmp(ent->classname, "item_matrixshield") == 0)
		{
			G_FreeEdict (ent);
			return;
		}
	}
	// some items will be prevented in deathmatch
	else
	{
		if ((int)dmflags->value & DF_NO_HEALTH)
		{
			if (item->pickup == Pickup_Health)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ((int)dmflags->value & DF_INFINITE_AMMO)
		{
			if ((item->flags == IT_AMMO) || (strcmp(ent->classname, "weapon_matrix") == 0))	// RSP 051999
			{
				G_FreeEdict (ent);
				return;
			}
		}

		// RSP 090100 - teleportgun not allowed in DM
		if (strcmp(ent->classname, "weapon_teleportgun") == 0)
			{
				G_FreeEdict (ent);
				return;
			}

		// RSP 090100 - teleportgun rod ammo not allowed in DM
		if (strcmp(ent->classname, "ammo_rod") == 0)
			{
				G_FreeEdict (ent);
				return;
			}
	}

	// WPO 230100 - Changed name from key_power_cube
	if (coop->value && (strcmp(ent->classname, "key_explosive") == 0))
	{
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP))
		item->drop = NULL;

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;

	if (ent->model == NULL)
		ent->model = item->world_model;
	ent->s.modelindex = gi.modelindex(ent->model);

	ent->flags |= FL_PASSENGER;		// RSP 061899 - BoxCar Passenger candidate
}

//======================================================================

gitem_t	itemlist[] = 
{
	{
		NULL
	},	// leave index 0 alone

	//
	// LUC WEAPONS
	//

	// AJA 19990116 - Reorganized the itemlist. NOTE: The order of weapons
	// is significant. The "weapnext" command (bound to "/" by default)
	// uses the order in this list to determine which weapon to switch to.


// MRB 021599 - Adding in knife support
/* weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_knife", 
		NULL,						// There's no knife in the world to pick up
		Use_Weapon,
		NULL,						// Can't drop the knife
		Weapon_Knife,
		"misc/w_pkup.wav",
		NULL,
		0,
		"models/weapons/v_knife/tris.md2",
		"w_knife",					// icon 
		"Knife",					// pickup name
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_KNIFE,					// 3.20
		NULL,
		0,
		// RSP 032399 - precache knife sounds
		// WPO 071599 - Added swish 
		"weapons/knife/hitsolid.wav weapons/knife/hitmeat.wav weapons/knife/hitwater.wav weapons/knife/swing.wav"	// precaches
	},


// JBM 082998 - Support for Speargun
/*QUAKED weapon_speargun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_speargun",			// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_Speargun,			// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_speargun/tris.md2",	// world model
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_speargun/tris.md2",	// view model
		"w_speargun",				// icon
		"Speargun",					// pickup name (for printing on pickup)
		0,							// count width (number of digits)
		1,							// quantity
		"Spears",					// ammo
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_SPEARGUN,				// 3.20
		NULL,						// info
		0,							// tag
		// RSP 032399 - precache spear sounds
		// WPO 042800 - Changed sounds
		"weapons/speargun/spear.wav weapons/speargun/firespear.wav weapons/speargun/rockspear.wav weapons/speargun/spearbrk.wav"		//precaches
	},

// RSP 121298 - Disk gun.
/*QUAKED weapon_diskgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_diskgun",			// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_DiskGun,				// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_disk/tris.md2",	// world model
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_disk/tris.md2",	// view model RSP 122498: vweap added
		"w_diskgn",					// icon
		"Diskgun",					// pickup name (for printing on pickup) // RSP 032699
		0,							// count width (number of digits)
		1,							// quantity
		"Disks",					// ammo
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_DISKGUN,				// 3.20
		NULL,						// info
		0,							// tag
		"weapons/diskgun/diskfire.wav weapons/diskgun/diskfly.wav"		// precaches RSP 122498: removed reload sound
	},

// MRB 12/15/98 - attempt to put in plasma weapon
/*QUAKED weapon_plasmawad (0 .5 .8) (-16 -16 -16) (16 16 16)
PlasmaWad player weapon
*/
	{
		"weapon_plasmawad",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_PlasmaWad,
		"misc/w_pkup.wav",
		"models/monsters/bots/worker/w_Coil.md2",
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_plasmawad/tris.md2",
		"w_plasmawad",				// icon		
		"Plasmawad",				// pickup	// RSP 030999	
		0,
		1,
		"Plasma",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_PLASMAWAD,				// 3.20
		NULL,
		0,
		// RSP 032399 - precache plasma sounds
		"weapons/plasma/plasma_hum.wav weapons/plasma/plasfire.wav weapons/plasma/plasfly.wav weapons/plasma/plasburn.wav weapons/plasma/plaschrg.wav"// precaches 
	},


// RSP 051799 - The return of the machinegun, as a boltgun

/*QUAKED weapon_boltgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_boltgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Boltgun,
		"misc/w_pkup.wav",
		"models/weapons/g_bolt/tris.md2",
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_bolt/tris.md2",
		"w_boltgn",					// icon
		"Boltgun",					// pickup
		0,
		1,
		"Bolts",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BOLTGUN,
		NULL,
		0,
		""  						// WPO 042800 sounds removed
	},

	

// WPO 021199 - Added LUC shotgun
/*QUAKED weapon_lucshotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_lucshotgun",		// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_LUCShotgun,			// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_shotgun/tris.md2",	// world model
		0,							// world model flags 
		"models/weapons/v_shotgun/tris.md2",	// view model
		"w_shotgun",				// icon
		"LUC Shotgun",				// pickup name (for printing on pickup)
		0,							// count width (number of digits)
		2,							// quantity
		"Plasma",					// ammo
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_LUCSHOTGUN,			// 3.20
		NULL,						// info
		0,							// tag
		""							// precaches 
	},


// WPO 231099 - Added teleport gun
/*QUAKED weapon_teleportgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_teleportgun",		// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_Teleportgun,			// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_telegun/tris.md2",	// world model
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_telegun/tris.md2",	// view model
		"w_teleportgun",			// icon
		"TeleportGun",				// pickup name (for printing on pickup)
		0,							// count width (number of digits)
		2,							// RSP 090100 - quantity used when fired
		"Rods",						// ammo	// RSP 090100 - use rods instead of plasma
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_TELEPORTGUN,			// 3.20
		NULL,						// info
		0,							// tag
		"misc/tele.wav weapons/teleportgun/tport_fail.wav weapons/teleportgun/tport_fire.wav weapons/teleportgun/tport_launch.wav weapons/teleportgun/tport_store.wav weapons/teleportgun/tport_ding.wav"				// precaches 
	},

// WPO 111199 - Modified freeze gun
/*QUAKED weapon_freezegun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_freezegun",			// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_Freezegun,			// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_freezegun/tris.md2",	// world model
		0,							// world model flags 
		"models/weapons/v_freezegun/tris.md2",	// view model
		"w_frezgn",					// icon
		"FreezeGun",				// pickup name (for printing on pickup)
		0,							// count width (number of digits)
		1,							// quantity
		"Ice Pellets",				// ammo
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_FREEZEGUN,				// 3.20
		NULL,						// info
		0,							// tag
		// WPO 151199 Added freezehit sound
		// RSP 070300 Added freezeball flyby sound
		"sprites/s_freeze.sp2 items/quaddamage.wav weapons/freeze/freeze.wav weapons/freeze/freezefire.wav weapons/freeze/freezehit.wav" // precaches 
	},

// WPO 070100 -
/*QUAKED weapon_rifle (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_rifle",				// classname
		Pickup_Weapon,				// pickup function
		Use_Weapon,					// use function
		Drop_Weapon,				// drop function
		Weapon_Rifle,				// weaponthink function
		"misc/w_pkup.wav",			// pickup sound
		"models/weapons/g_rifle/tris.md2",	// world model
		0,							// world model flags 
		"models/weapons/v_rifle/tris.md2",	// view model
		"w_rifle",					// icon
		"Sniper Rifle",				// pickup name (for printing on pickup)
		0,							// count width (number of digits)
		1,							// quantity
		"Bullets",					// ammo
		IT_WEAPON|IT_STAY_COOP,		// flags
		WEAP_RIFLE,					// 3.20
		NULL,						// info
		0,							// tag
		""							// precaches 
	},


// RSP 051999 - Added Matrix
/*QUAKED weapon_matrix (.3 .3 1) (-12 -12 -8) (12 12 8)
*/
	{
		"weapon_matrix",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Matrix,
		"misc/w_pkup.wav",
		"models/weapons/g_matrix/tris.md2",
		0,							// world model flags (RSP 102999 - LUC doesn't rotate weapons)
		"models/weapons/v_matrix/tris.md2",
		"w_matrix",					// icon
		"Matrix",					// pickup name
		0,
		50,							// quantity used when fired // RSP 121899
		"Plasma",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_MATRIX,
		NULL,
		0,
		"weapons/matrix/matrixfly.wav weapons/matrix/matrix_hum.wav weapons/matrix/mat_fire.wav weapons/matrix/mat_whine.wav"	// precache
	},


//
// LUC AMMO
//

// WPO: 7/16/99  added grenades - these are both ammo and a weapon you can switch to
/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"misc/am_pkup.wav",
		"models/items/ammo/grenade/tris.md2",
		0,
		"models/weapons/v_grenade/tris.md2",
		"a_grenades",				// icon
		"Grenades",					// pickup name
		3,							// width
		5,
		"grenades",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
		"weapons/grenade/hgrent1a.wav weapons/grenade/hgrena1b.wav weapons/grenade/hgrenc1b.wav weapons/grenade/hgrenb1a.wav weapons/grenade/hgrenb2a.wav "
	},

	
// JBM 082998 - Spears for Speargun
	{
		"ammo_spears",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/spears/tris.md2",
		0,
		NULL,
		"a_spears",					// icon
		"Spears",					// pickup name
		3,							// width
		8,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_SPEARS,
		""							// precache
	},

// RSP 051399 - Rocket Spears for Speargun
	{
		"ammo_rocket_spears",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/spears/tris.md2",
		0,
		NULL,
		"a_r_spears",				// icon
		"Rocket Spears",			// pickup name
		3,							// width
		8,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_ROCKET_SPEARS,
		""							// precache
	},


// RSP 121298 - Ammo for Disk Gun
/*QUAKED ammo_disks (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_disks",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/disks/tris.md2",
		0,
		NULL,
		"a_disks",					// icon
		"Disks",					// pickup name
		3,							// width
		5,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_DISKS,
		""							// precache
	},

// MRB 031599 - Some plasma for the PlasmaWad.
/*QUAKED ammo_plasma (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_plasma",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/plasma/tris.md2",
		0,
		NULL,
		"a_plasma",					// icon
		"Plasma",					// pickup name
		2,							// width
		10,
		NULL,
		IT_AMMO,
		0,	// 3.20
		NULL,
		AMMO_PLASMA,
		""							// precache
	},

// WPO 111199 - Ice pellets for the freeze gun.
/*QUAKED ammo_icepellets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_icepellets",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/icepellets/tris.md2",
		0,
		NULL,
		"a_pellets",				// icon
		"Ice Pellets",				// pickup name
		2,							// width
		1,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_ICEPELLETS,
		""							// precache
	},

// RSP 051799 - Box of bolts for BoltGun.
/*QUAKED ammo_bolts (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bolts",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bolts/tris.md2",	// RSP 090300
		0,
		NULL,
		"a_bolts",					// icon RSP 071999 - added correct icon name
		"Bolts",					// pickup name
		3,							// width
		50,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_BOLTS,
		""							// precache
	},

// WPO 070100 - Bullets for the sniper rifle.
/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/tris.md2",
		0,
		NULL,
		"a_bullets", 				// icon
		"Bullets",					// pickup name
		3,							// width
		10,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_BULLETS,
		""							// precache
	},

// RSP 090100 - Teleport Rod for Teleportgun
/*QUAKED ammo_rod (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_rod",
		Pickup_Ammo,
		Use_Ammo,					// RSP 091900
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rods/tris.md2",
		0,
		NULL,
		"a_rods",					// icon
		"Rods",						// pickup name
		3,							// width
		1,
		NULL,
		IT_AMMO,
		0,							// 3.20
		NULL,
		AMMO_RODS,
		""							// precache
	},



//
// LUC KEYS
//

// MRB 11/21/98 - Drake's keys
// AJA 19990117 - Modified the existing key and added the remaining 22.
// RSP 091900   - Deleted keys 7->12
/*QUAKED key_glyph01 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph01",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph01/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph01",
		"Glyph 1",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,	// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph01b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph01b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph01b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph01b",
		"Glyph 1b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph02 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph02",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph02/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph02",
		"Glyph 2",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph02b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph02b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph02b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph02b",
		"Glyph 2b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph03 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph03",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph03/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph03",
		"Glyph 3",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph03b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph03b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph03b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph03b",
		"Glyph 3b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph04 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph04",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph04/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph04",
		"Glyph 4",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph04b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph04b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph04b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph04b",
		"Glyph 4b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph05 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph05",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph05/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph05",
		"Glyph 5",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph05b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph05b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph05b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph05b",
		"Glyph 5b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph06 (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph06",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph06/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph06",
		"Glyph 6",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},

/*QUAKED key_glyph06b (0 .5 .8) (-16 -16 -16) (16 16 16)
*/
	{
		"key_glyph06b",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/glyph06b/tris.md2", 
		EF_ROTATE,
		NULL,
		"k_glyph06b",
		"Glyph 6b",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,							// 3.20
		NULL,
		0,
		""							// precache
	},


	// WPO 161099 - Put power cubes back in
	// WPO 230100 - Changed name from key_power_cube
/*QUAKED key_explosive (0 .5 .8) (-12 -12 0) (12 12 32) TRIGGER_SPAWN NO_TOUCH
warehouse circuits
*/
	{
		"key_explosive",
		Pickup_Key,
		Use_Key,					// RSP 091900
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2",
		EF_ROTATE,
		NULL,
		"Tile8",
//		"k_explosive",
		"Explosive",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
		""							// precache
	},

	//
	// LUC Powerups
	// 
	//
	// WPO 100199 Added LUC powerup logic

/*QUAKED item_matrixshield (.3 .3 1) (-18 -24 0) (18 24 36)
*/
	{
		"item_matrixshield",
		luc_powerup_pickup,
		Use_MatrixShield,			// RSP 091900 - 
		luc_powerup_drop,
		NULL,
		"items/pkup.wav",
		"models/items/matrxshld/tris.md2",
		EF_ROTATE,					// RSP 121899
		NULL,
		POWERUP_MSHIELD_ICON,		// icon
		POWERUP_MSHIELD_NAME,		// pickup
		1,							// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""							// precache
	},

/*QUAKED item_lavaboots (.3 .3 1) (-18 -22 0) (18 22 32)
*/
	{
		"item_lavaboots",
		luc_powerup_pickup,
		luc_powerup_use,
		luc_powerup_drop,
		NULL,
		"items/pkup.wav",
		"models/items/lavaboots/tris.md2",
		EF_ROTATE,
		NULL,
		POWERUP_LAVABOOTS_ICON,		// icon
		POWERUP_LAVABOOTS_NAME,		// pickup
		1,							// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""							// precache
	},

/*QUAKED item_holodecoy (.3 .3 1) (-16 -16 0) (16 16 48)
*/
	{
		"item_holodecoy",
		luc_powerup_pickup,
		luc_powerup_use,
		Drop_HoloDecoy,						// RSP 091700
		NULL,
		NULL,
		"models/items/holodecoy/tris.md2",	// RSP 081400 - correct misspelling
		EF_ROTATE,
		NULL,
		POWERUP_DECOY_ICON,					// icon
		POWERUP_DECOY_NAME,					// pickup
		2,									// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""									// precache
	},

// WPO 071199 - Space Warp
/*QUAKED item_warp (.3 .3 1) (-8 -8 -8) (8 8 8)
*/
	{
		"item_warp",
		luc_powerup_pickup,
		luc_powerup_use,
		luc_powerup_drop,
		NULL,
		NULL,
		"models/items/warp/tris.md2",
		EF_ROTATE,
		NULL,
		POWERUP_WARP_ICON,				// icon
		POWERUP_WARP_NAME,				// pickup
		2,								// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""								// precache
	},

// WPO 301099 - Teleporter
/*QUAKED item_teleporter (.3 .3 1) (-8 -8 -8) (8 8 8)
*/
	{
		"item_teleporter",
		luc_powerup_pickup,
		luc_powerup_use,
		luc_powerup_drop,
		NULL,
		NULL,
		"models/items/teleporter/tris.md2",
		0,								// RSP 081700 - rotation turned off for teleporter
		NULL,
		POWERUP_TELEPORTER_ICON,		// icon
		POWERUP_TELEPORTER_NAME,		// pickup
		2,								// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""								// precache
	},

// WPO 061199 Modified as per new powerup policy
/*QUAKED item_superspeed (.3 .3 1) (-16 -16 0) (16 16 32)
*/
	{
		"item_superspeed", 
		luc_powerup_pickup,		
		luc_powerup_use,		
		luc_powerup_drop,		
		NULL,
		"items/pkup.wav",
		"models/items/superspeed/tris.md2",
		EF_ROTATE,
		NULL,
		POWERUP_SUPERSPEED_ICON,		// icon
		POWERUP_SUPERSPEED_NAME,		// pickup
		2,								// width
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""								// precache
	},

// WPO 061199 - Cloak
/*QUAKED item_cloak (.3 .3 1) (-8 -8 -8) (8 8 8)
*/
	{
		"item_cloak",
		luc_powerup_pickup,
		luc_powerup_use,
		luc_powerup_drop,
		NULL,
		NULL,
		"models/items/cloak/tris.md2",
		EF_ROTATE,
		NULL,
		POWERUP_CLOAK_ICON,				// icon
		POWERUP_CLOAK_NAME,				// pickup
		2,								// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""								// precache
	},

/*QUAKED item_vampire (.3 .3 1) (-16 -16 0) (16 16 32)
*/
	{
		"item_vampire",
		luc_powerup_pickup,
		luc_powerup_use,
		luc_powerup_drop,
		NULL,
		NULL,
		"models/items/vampire/tris.md2",
		EF_ROTATE,
		NULL,
		POWERUP_VAMPIRE_ICON,			// icon
		POWERUP_VAMPIRE_NAME,			// pickup
		2,								// width
		0,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""								// precache
	},


//
// LUC ITEMS
//

// AJA 19980310 - Scuba gear.
// AJA 19980326 - Modified QuakEd entity size, and added the EF_ROTATE
// flag 'cause it looks better with the new model.
/*QUAKED item_scubagear (.3 .3 1) (-12 -12 0) (12 12 40)
*/
	{
		"item_scubagear",			// classname
		Pickup_ScubaGear,			// pickup function
		Use_ScubaGear,				// use function
		Drop_ScubaGear,				// drop function
		NULL,						// weaponthink function
		"items/pkup.wav",			// pickup sound
		"models/items/scuba/tris.md2",		// world model
		EF_ROTATE,							// world model flags
//		"models/personal/mask/tris.md2",	// view model
		NULL,						// view model RSP 082600 - mask deleted
		"p_scubagear",				// icon
		"Scuba Gear",				// pickup name (for printing on pickup)
		1,							// count width (number of digits)
		60,							// quantity
		NULL,						// ammo
		IT_STAY_COOP|IT_POWERUP,	// flags
		0,	// 3.20
		NULL,						// info
		0,							// tag
		"items/airout.wav p_airgauge_0.pcx p_airgauge_1.pcx p_airgauge_2.pcx p_airgauge_3.pcx p_airgauge_4.pcx p_airgauge_5.pcx p_airgauge_6.pcx p_airgauge_7.pcx p_airgauge_8.pcx p_airgauge_9.pcx p_airgauge_10.pcx items/scuba/d_breath1.wav items/scuba/d_breath2.wav items/scuba/w_breath1.wav items/scuba/w_breath2.wav"	// precaches
	},

// AJA 19980315 - Air tank, refill for scuba gear.
/*QUAKED item_airtank (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_airtank",				// classname
		Pickup_AirTank,				// pickup function
		NULL,						// use function
		NULL,						// drop function
		NULL,						// weaponthink function
		"items/pkup.wav",			// pickup sound
		"models/items/airtank/tris.md2",	// world model
		EF_ROTATE,					// world model flags
		NULL,						// view model
		"p_scubagear",				// icon
		"Air",						// pickup name RSP 010399: make printable name more generic
		1,							// count width (number of digits)
		AIR_QUANTITY,				// quantity
		NULL,						// ammo
		IT_NOTININV,				// flags
		0,	// 3.20
		NULL,						// info
		0,							// tag
		""							// precache
	},

// MRB 031399 - A single spear for the player to pick up
	{	// Typically thrown by something
		"item_normal_spear",
		Pickup_Normal_Spear,
		NULL,
		NULL,
		NULL,
		"misc/am_pkup.wav",
		"models/objects/spears/normal.md2",
		0,
		NULL,
		"a_spears",					// icon
		"Single Normal Spear",		// pickup
		3,							// width
		1,
		NULL,
		IT_AMMO|IT_DONTGIVE|IT_NOTININV,	// RSP 091200
		0,	// 3.20
		NULL,
		AMMO_SPEARS,
		""							// precache
	},

// RSP 081500 - A single rocketspear for the player to pick up
	{	// Typically thrown by something
		"item_rocket_spear",
		Pickup_Rocket_Spear,
		NULL,
		NULL,
		NULL,
		"misc/am_pkup.wav",
		"models/objects/spears/rocket.md2",
		0,
		NULL,
		"a_r_spears",				// icon
		"Single Rocket Spear",		// pickup
		3,							// width
		1,
		NULL,
		IT_AMMO|IT_DONTGIVE|IT_NOTININV,	// RSP 091200
		0,	// 3.20
		NULL,
		AMMO_ROCKET_SPEARS,
		""							// precache
	},

// MRB 031599 - A single wad of plasma for the player to pick up
	{
		"item_plasma_ball",
		Pickup_Plasma,
		NULL,
		NULL,
		NULL,
		"misc/am_pkup.wav",
		"models/objects/plasma/tris.md2",
		0,
		NULL,
		"a_plasma",					// icon
		"Small Plasma Ball",		// pickup
		3,							// width
		10,							// quantity may be less than full
		NULL,
		IT_AMMO|IT_DONTGIVE|IT_NOTININV,	// RSP 091200
		0,	// 3.20
		NULL,
		AMMO_PLASMA,
		""							// precache
	},

	{
		"item_health",				// RSP 082400
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/health.wav",
		NULL,
		0,
		NULL,
		"i_health",					// icon
		"Health",					// pickup
		3,							// width
		0,
		NULL,
		IT_DONTGIVE|IT_NOTININV,	// RSP 091200
		0,							// 3.20
		NULL,
		0,
		"items/health.wav"			// precache
	},

	// end of list marker
	{NULL}
};


void InitItems (void)
{
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}


/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i = 0 ; i < game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}
}


//=====================================================================
// RSP 071999 - Added LUC health items

/*QUAKED item_berry1 (.3 .3 1) (-8 -8 0) (8 8 16)

Increases health by 10
*/
void SP_item_berry1 (edict_t *self)
{
	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/health/h_berry1/tris.md2";
	self->count = 10;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/health.wav");
}

/*QUAKED item_berry2 (.3 .3 1) (-8 -8 0) (8 8 16)

Increases health by 20
*/
void SP_item_berry2 (edict_t *self)
{
	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/health/h_berry2/tris.md2";
	self->count = 20;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/health.wav");
}

/*QUAKED item_health_mini (.3 .3 1) (-8 -8 0) (8 8 16)

Increases health by 30
*/
void SP_item_health_mini (edict_t *self)
{
	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/health/h_mini/tris.md2";
	self->count = 30;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/health.wav");
	self->classname = "item_health_urn";
}

/*QUAKED item_health_midi (.3 .3 1) (-8 -8 0) (8 8 16)

Increases health by 50
*/
void SP_item_health_midi (edict_t *self)
{
	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/health/h_midi/tris.md2";
	self->count = 50;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/health.wav");
	self->classname = "item_health_urn";
}

/*QUAKED item_health_maxi (.3 .3 1) (-8 -8 0) (8 8 16)

This is the megahealth urn.
It gives you 50 health, plus GOD mode and tri-damage for 5 seconds
*/
void SP_item_health_maxi (edict_t *self)
{
	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/health/h_maxi/tris.md2";
	self->count = 50;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
	self->classname = "item_health_urn";
}

//=====================================================================


