// p_weapon.c

#include "g_local.h"
#include "m_player.h"

qboolean is_quad;
qboolean is_tri;			// RSP 082300 - tri-damage from megahealth

// MRB 122098 - I changed this to NONstatic because I wanted to see it outside this function.
//    If there is a better way I should be doing this, please tweak away.
void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t _distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t *noise;

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;

	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		noise->flags |= FL_PASSENGER;	// RSP 061999 - potential BoxCar passenger
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		noise->flags |= FL_PASSENGER;	// RSP 061999 - potential BoxCar passenger
		who->mynoise2 = noise;
	}

	if ((type == PNOISE_SELF) || (type == PNOISE_WEAPON))
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy (where, noise->s.origin);
	VectorSubtract (where, noise->maxs, noise->absmin);
	VectorAdd (where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity (noise);
}


qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int     index;
	gitem_t	*ammo;

	index = ITEM_INDEX(ent->item);

	if ((((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) &&
			other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;	// leave the weapon for others to pickup
	}

	// RSP 091500 - can't have more than one instance of each weapon
	if (other->client->pers.inventory[index] == 0)
		other->client->pers.inventory[index] = 1;

	if (!(ent->spawnflags & DROPPED_ITEM))
	{
		// give them some ammo with it
		ammo = FindItem (ent->item->ammo);

		if ((int)dmflags->value & DF_INFINITE_AMMO)
			Add_Ammo (other, ammo, 1000);
		else
			Add_Ammo (other, ammo, ammo->quantity);

		if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn (ent, 30);
			}
			if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	// If you didn't possess this weapon before, then switch to it if you're not playing DM
	// or you are playing deathmatch and you have the knife raised.

	if ((other->client->pers.weapon != ent->item) && 
		(other->client->pers.inventory[index] == 1) &&
		(!deathmatch->value || (other->client->pers.weapon == FindItem("Knife"))))
	{
		other->client->newweapon = ent->item;
	}

	return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
	int i;	// 3.20

	// RSP 101500 - if captured, can't raise a weapon
	if (ent->capture_state > 2)
		return;

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->ps.fov = 90;	// RSP 070400 - in case you're dropping a scoped rifle
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 4;	// start in middle of "bow" if raising the boltgun

	// set visible model (3.20)
	if (ent->s.modelindex == 255)
	{
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
	{
		// RSP 051399 - check for speargun and ammo type
		if (Q_stricmp(ent->client->pers.weapon->classname,"weapon_speargun") == 0)
		{
			// toggle speargun between normal and rocket spears
			if (ent->client->pers.ammo_type == AMMO_TYPE_ROCKET_SPEAR)
				ent->client->ammo_index = ITEM_INDEX(FindItem("Rocket Spears"));
			else
				ent->client->ammo_index = ITEM_INDEX(FindItem("spears"));
		}
		else
			ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	}
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	// 3.20
	ent->client->anim_priority = ANIM_PAIN;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crpain1;
		ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
		ent->s.frame = FRAME_pain301;
		ent->client->anim_end = FRAME_pain304; 
	}
	// 3.20 end
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
	// AJA 19990116 - Removed id weapons

	// RSP 051999 - Q2 never changed to its most potent weapon, the BFG. We shouldn't
	// change to our most potent weapon, the Matrix.

	// RSP 0704 - Added support for LUC sniper rifle w/bullets
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("Sniper rifle"))] )
	{
		ent->client->newweapon = FindItem ("Sniper rifle");
		return;
	}

	// RSP 0704 - Added support for LUC freezegun w/ice pellets
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Ice Pellets"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("freezegun"))] )
	{
		ent->client->newweapon = FindItem ("freezegun");
		return;
	}

	// RSP 0704 - Added support for LUC teleportgun w/rods
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Rod"))] > 1	// RSP 090100 - changed plasma to rods
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("teleportgun"))] )
	{
		ent->client->newweapon = FindItem ("teleportgun");
		return;
	}

	// RSP 0704 - Added support for LUC shotgun w/plasma
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Plasma"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("LUC shotgun"))] )
	{
		ent->client->newweapon = FindItem ("LUC shotgun");
		return;
	}

	// RSP 051799 - Added support for Bolt Gun w/ bolts
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Bolts"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("boltgun"))] )
	{
		ent->client->newweapon = FindItem ("boltgun");
		return;
	}

	// RSP 030999 - Added support for Plasma Wad with plasma
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Plasma"))] > 1		// MRB 031599
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("plasmawad"))] )
	{
		ent->client->newweapon = FindItem ("plasmawad");
		return;
	}

	// RSP 121298 - Added support for Disk Gun with disks
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("disks"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("Diskgun"))] )	// RSP 032699
	{
		ent->client->newweapon = FindItem ("Diskgun");	// RSP 032699
		return;
	}

	// RSP 070400 - Added support for Grenades
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("grenades"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("grenades"))] )
	{
		ent->client->newweapon = FindItem ("grenades");
		return;
	}

	// RSP 051399 - Added support for Spear Gun w/ rocket spears
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Rocket Spears"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("speargun"))] )
	{
		ent->client->newweapon = FindItem ("speargun");
		ent->client->pers.ammo_type = AMMO_TYPE_ROCKET_SPEAR;
		return;
	}

	// JBM 012398 - Added support for Spear Gun w/ normal spears
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("spears"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("speargun"))] )
	{
		ent->client->newweapon = FindItem ("speargun");
		ent->client->pers.ammo_type = AMMO_TYPE_NORMAL_SPEAR;
		return;
	}

	ent->client->newweapon = FindItem ("knife");	// RSP 030999: Knife exists now
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		is_tri  = (ent->client->tri_framenum > level.framenum);		// RSP 082300

		// WPO 241099 Superspeed powerup
		if (ent->client->superspeed_framenum > level.framenum)
		{
			// frame nums are 10 per second
			// with 5 seconds left, start ramping down speed
			if ((ent->client->superspeed_framenum - level.framenum) > 50)
				ent->client->PlayerSpeed = 2.0;
			else
				ent->client->PlayerSpeed = 1.0 + (((ent->client->superspeed_framenum - level.framenum)) / 100);
		}
		else
			ent->client->PlayerSpeed = 1.0;
				
		ent->client->pers.weapon->weaponthink (ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int     ammo_index;
	gitem_t	*ammo_item;
	char    *ammo_name;     // RSP 051399

	// see if we're already using it
	if (item == ent->client->pers.weapon)
	{
		if (Q_stricmp(item->classname,"weapon_speargun") == 0)
		{
			// RSP 071399 - If the ent->client->newweapon is already set,
			// then this call is part of a next weapon or prev weapon
			// command. We can toggle between normal and rocket spears
			// only under certain conditions. Given 1 = knife, 2N = normal spears,
			// 2R = rocket spears, and 3 = diskgun, then moving to the next
			// weapon will follow the sequence 1,2N,2R,3. Moving to the previous
			// weapon will follow the sequence 3,2R,2N,1.

			// So, is there a newweapon, and what is it?

			if (ent->client->newweapon)
			{
				// Figure out which way we're going. Up or down through the list.
				if (Q_stricmp(ent->client->newweapon->classname,"weapon_knife") == 0)
				{

					// Moving down. Now, are we currently using normal or
					// rocket spears?
					if (ent->client->pers.ammo_type == AMMO_TYPE_NORMAL_SPEAR)
						return;	// all set, no need to change anything
					else
					{	// switch to normal spears
						ent->client->pers.ammo_type = AMMO_TYPE_NORMAL_SPEAR;
						ammo_name = "Spears";
					}
				}
				else
				{
					// Moving up. Now, are we currently using normal or
					// rocket spears?
					if (ent->client->pers.ammo_type == AMMO_TYPE_NORMAL_SPEAR)
					{
						ent->client->pers.ammo_type = AMMO_TYPE_ROCKET_SPEAR;
						ammo_name = "Rocket Spears";
					}
					else
						return;	// all set, no need to change anything
				}
			}

			// toggle speargun between normal and rocket spears
			// RSP 121899 - don't allow toggle if no ammo
			else if (ent->client->pers.ammo_type == AMMO_TYPE_NORMAL_SPEAR)
			{
				if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Rocket Spears"))])
				{
					gi.cprintf (ent, PRINT_HIGH, "No Rocket Spears for Speargun.\n");
					return;
				}
				ent->client->pers.ammo_type = AMMO_TYPE_ROCKET_SPEAR;
				ammo_name = "Rocket Spears";
			}
			else
			{
				if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Spears"))])
				{
					gi.cprintf (ent, PRINT_HIGH, "No Spears for Speargun.\n");
					return;
				}
				ent->client->pers.ammo_type = AMMO_TYPE_NORMAL_SPEAR;
				ammo_name = "Spears";
			}
		}
		else if (Q_stricmp(item->classname,"weapon_diskgun") == 0)
		{
			if (ent->client->newweapon)
				if (Q_stricmp(ent->client->newweapon->classname,"weapon_speargun") == 0)
					ent->client->pers.ammo_type = AMMO_TYPE_ROCKET_SPEAR;
			return;

		}
		else
			return;	// already using the requested weapon
	}
	else
	{
		ammo_name = item->ammo;

		// RSP 051399 - make sure next raise of speargun loads normal spears
		if (Q_stricmp(item->classname,"weapon_speargun") == 0)
			ent->client->pers.ammo_type = AMMO_TYPE_NORMAL_SPEAR;
	}

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item  = FindItem(ammo_name);
		ammo_index = ITEM_INDEX(ammo_item);

		// RSP 121899 - if no normal spears available, are there rocket spears?
		if (Q_stricmp(item->classname,"weapon_speargun") == 0)
		{
			if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Spears"))])
  			   	if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Rocket Spears"))])
  				{
  					gi.cprintf (ent, PRINT_HIGH, "No Spears for Speargun.\n");
  					return;
  				}
  				else	// switch to rocket spears
  				{
  					ent->client->pers.ammo_type = AMMO_TYPE_ROCKET_SPEAR;
  					ammo_name = "Rocket Spears";
  				}
		}
  	    else if (!ent->client->pers.inventory[ammo_index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
		else if (ent->client->pers.inventory[ammo_index] < item->quantity)
 		{
			gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	int index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);

	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) &&
		 (ent->client->pers.inventory[index] == 1))
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item (ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST        (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST        (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
	int			n;
	gclient_t	*client = ent->client;	// RSP 090100 - simplify

	if (ent->deadflag || (ent->s.modelindex != 255)) // VWep animations screw up corpses (3.20)
		return;

	// WPO 241099 no shooting while you are frozen
	if (ent->freeze_framenum > level.framenum)
		return;

	if (client->weaponstate != WEAPON_FIRING)
	{
		// RSP 051799: Weapon is idle. Cool off the bolt gun, even if you don't have it.
		if (client->pers.bolt_heat > 0)
		{
			client->pers.bolt_heat -= FRAMETIME;
			if (client->pers.bolt_heat < 0)
				client->pers.bolt_heat = 0;
		}
	}

	if (client->weaponstate == WEAPON_DROPPING)
	{
		if (client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - client->ps.gunframe) == 4)	// 3.20
		{
			client->anim_priority = ANIM_REVERSE;
			if (client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				client->anim_end = FRAME_pain301;
			}
		}

		client->ps.gunframe++;
		return;
	}

	if (client->weaponstate == WEAPON_ACTIVATING)
	{
		if (client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			client->weaponstate = WEAPON_READY;
			client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		client->ps.gunframe++;
		return;
	}

	if ((client->newweapon) && (client->weaponstate != WEAPON_FIRING))
	{
		client->weaponstate = WEAPON_DROPPING;
		client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		// 3.20
		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			client->anim_priority = ANIM_REVERSE;
			if (client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				client->anim_end = FRAME_pain301;
			}
		}

		// 3.20 end
		return;
	}

	if (client->weaponstate == WEAPON_READY)
	{
		if (((client->latched_buttons|client->buttons) & BUTTON_ATTACK))
		{
			client->latched_buttons &= ~BUTTON_ATTACK;
			if ((!client->ammo_index) ||
				(client->pers.inventory[client->ammo_index] >= client->pers.weapon->quantity) ||
				((Q_stricmp(client->pers.weapon->classname,"weapon_teleportgun") == 0) && ent->teleportgun_victim))	// RSP 090100 - allow tportgun to discharge
			{	// MRB 092600 Loaded teleport gun uses a different 'fire' frame
				if (ent->teleportgun_victim && (Q_stricmp(client->pers.weapon->classname,"weapon_teleportgun") == 0))
					client->ps.gunframe = fire_frames[1];
				else
					client->ps.gunframe = FRAME_FIRE_FIRST;

				client->weaponstate = WEAPON_FIRING;

				// start the animation
				client->anim_priority = ANIM_ATTACK;
				if (client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1 - 1;
					client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1 - 1;
					client->anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else if ((client->ps.gunframe == FRAME_IDLE_FIRST -3) &&
				(Q_stricmp(client->pers.weapon->classname,"weapon_teleportgun") == 0))
		{	// MRB 092600 Loaded teleport gun cycles on different frames
			client->ps.gunframe = FRAME_FIRE_FIRST + 2;
		}
		else
		{
			if (client->ps.gunframe == FRAME_IDLE_LAST)
			{
				client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
				for (n = 0 ; pause_frames[n] ; n++)
					if (client->ps.gunframe == pause_frames[n])
						if (rand() & 15)
							return;

			client->ps.gunframe++;
			return;
		}
	}

	if (client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0 ; fire_frames[n] ; n++)
			if (client->ps.gunframe == fire_frames[n])
			{
				fire (ent);
				break;
			}

		if (!fire_frames[n])
			client->ps.gunframe++;

		// MRB 092600 Keep the teleport gun cycling if it is loaded with a target
		if (ent->teleportgun_victim && (client->ps.gunframe + 3 == FRAME_IDLE_FIRST) &&
			(Q_stricmp(client->pers.weapon->classname,"weapon_teleportgun") == 0))
		{
			client->ps.gunframe = FRAME_FIRE_FIRST + 2;
		}

		else if (client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			client->weaponstate = WEAPON_READY;
	}
}
