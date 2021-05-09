#include "g_local.h"
#include "luc_powerups.h"
#include "luc_decoy.h"
#include "luc_teleporter.h"
#include "luc_warp.h"

LUCPowerups luc_powerups[POWERUP_COUNT] =
{
	{ 0, POWERUP_MSHIELD_NAME, POWERUP_MSHIELD_ICON, 0 },
	{ 0, POWERUP_LAVABOOTS_NAME, POWERUP_LAVABOOTS_ICON, 0 },
	{ 0, POWERUP_DECOY_NAME, POWERUP_DECOY_ICON, 0 },
	{ 0, POWERUP_WARP_NAME, POWERUP_WARP_ICON, 0 },				// WPO 071199 - Space Warp
	{ 0, POWERUP_TELEPORTER_NAME, POWERUP_TELEPORTER_ICON, 0 },	// WPO 301099 - Teleporter
	{ 0, POWERUP_SUPERSPEED_NAME, POWERUP_SUPERSPEED_ICON, 0 },	// WPO 061199 - Super Speed
	{ 0, POWERUP_CLOAK_NAME, POWERUP_CLOAK_ICON, 0 },			// WPO 061199 - Cloak
	{ 0, POWERUP_VAMPIRE_NAME, POWERUP_VAMPIRE_ICON, 0 }		// WPO 231099 - Vampire 
};


// called once to get all the powerup item indices
// instead of doing the lookup every freaking time
qboolean luc_powerup_init(void)
{
	int			i;
	qboolean	results = true;	// RSP 091200

	for (i = 0 ; i < POWERUP_COUNT ; i++)
		if ((luc_powerups[i].itemIndex = ITEM_INDEX(FindItem(luc_powerups[i].name))) == 0)
		{
			gi.bprintf(PRINT_HIGH, "luc_powerup item %s is not in the item list\n", luc_powerups[i].name);
			results = false;
		}

	return results;
}

// true if a person has a specific powerup
qboolean luc_ent_has_powerup(edict_t *ent, int type) 
{
	if (!ent->client) 
		return false;

	return (ent->client->pers.inventory[luc_powerups[type].itemIndex]);
}


// WPO 061199  rewritten
// a live client has touched a powerup
qboolean luc_powerup_pickup(edict_t *self, edict_t *other) 
{
	qboolean playsound = true;      // RSP 062900

// RSP 091100 - added cloak to "instant use" list

	if (self->item == FindItem(POWERUP_CLOAK_NAME))
	{
		int     timeout = 600;
		int     i;
		edict_t	*monster;

		if (other->client->cloak_framenum > level.framenum)
			other->client->cloak_framenum += timeout;
		else
		{
			other->client->cloak_framenum = level.framenum + timeout;

			// RSP 110799
			// find all monsters that currently have this player
			// as an enemy, if not in deathmatch, where there are no monsters

			if (!deathmatch->value)
				for (i = 0 ; i < globals.num_edicts ; i++)
				{
					monster = &g_edicts[i];

					// If this monster has this player as an enemy, the monster
					// has to forget the player because the player is now cloaked.
					if ((monster->svflags & SVF_MONSTER) && (monster->enemy == other))
						Forget_Player(monster);
				}
		}
	}
	// WPO 141199 Added vampire pickup logic
	// This is an instant use powerup
	else if (self->item == FindItem(POWERUP_VAMPIRE_NAME))
	{
		int timeout = 600;

		if (other->client->vampire_framenum > level.framenum)
			other->client->vampire_framenum += timeout;
		else
			other->client->vampire_framenum = level.framenum + timeout;
	}
	// WPO 141199 Added decoy pickup logic
	else if (self->item == FindItem(POWERUP_DECOY_NAME))
	{	// RSP 091200 - if you already have a decoy, don't take this one
		if (other->client->pers.inventory[ITEM_INDEX(self->item)] == 0)
		{
			other->client->decoy_timeleft = self->count;	// RSP 091700 - get whatever was left of the original 60 seconds time
			other->client->pers.inventory[ITEM_INDEX(self->item)] = 1;
		}
		else
			playsound = false;
	}
	else
	{
		// add this powerup to the inventory
		other->client->pers.inventory[ITEM_INDEX(self->item)]++;
	}

	// RSP 062900 - play sound?
	if (playsound)
		gi.sound(other,CHAN_ITEM,gi.soundindex("items/powerup.wav"),1,ATTN_NORM,0);

	// setup to respawn the powerup
	if (deathmatch->value)
		SetRespawn(self, 60);

	return true;
}


// makes the powerup touchable again after being dropped
void luc_powerup_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
//	ent->nextthink = level.time + 120;
//	ent->think = luc_powerup_move;
}

void luc_powerup_use (edict_t *ent, gitem_t *item) 
{	
	// WPO 061199 add use commands for everything that can be 'used'
	if (item == FindItem(POWERUP_DECOY_NAME))
		SP_Decoy(ent);
	else if (item == FindItem(POWERUP_TELEPORTER_NAME))
	{
		Cmd_Teleport_f(ent);
		ent->client->pers.inventory[ITEM_INDEX(item)]--;
		ValidateSelectedItem (ent);
	}
	else if (item == FindItem(POWERUP_WARP_NAME))
	{
		if (Warp_Use(ent))
		{
			ent->client->pers.inventory[ITEM_INDEX(item)]--;
			ValidateSelectedItem(ent);
		}
	}

// RSP 091100 - make superspeed an "activate later" item

	else if (item == FindItem(POWERUP_SUPERSPEED_NAME))
	{
		int timeout = 600;

		if (ent->client->superspeed_framenum > level.framenum)
			ent->client->superspeed_framenum += timeout;
		else
			ent->client->superspeed_framenum = level.framenum + timeout;
		ent->client->pers.inventory[ITEM_INDEX(item)]--;	// RSP 091200 - use it up
		ValidateSelectedItem(ent);
	}

// RSP 091100 - make lavaboots an "activate later" item

	else if (item == FindItem(POWERUP_LAVABOOTS_NAME))
	{
		int timeout = 600;

		if (ent->client->lavaboots_framenum > level.framenum)
			ent->client->lavaboots_framenum += timeout;
		else
			ent->client->lavaboots_framenum = level.framenum + timeout;
		ent->client->pers.inventory[ITEM_INDEX(item)]--;	// RSP 091200 - use it up
		ValidateSelectedItem(ent);
	}
}


void luc_ent_used_powerup(edict_t *ent, int type) 
{
	// make sure the player has this powerup

	if (ent->client->pers.inventory[luc_powerups[type].itemIndex])
	{
		ent->client->pers.inventory[luc_powerups[type].itemIndex]--;
		ValidateSelectedItem(ent);
	}
}


void luc_powerup_drop_dying (edict_t *ent, gitem_t *item)
{
	edict_t	*luc_powerup = Drop_Item(ent, item);

	// Drop_Item() sets think and nextthink, but we're changing think here
	luc_powerup->think = luc_powerup_make_touchable;

	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
	ValidateSelectedItem(ent);
}

void luc_powerups_drop_dying (edict_t *ent)
{
	int i;

	for (i = 0 ; i < POWERUP_COUNT ; i++)
		if (ent->client->pers.inventory[luc_powerups[i].itemIndex])
			luc_powerup_drop_dying(ent, &itemlist[luc_powerups[i].itemIndex]);
}
		
// drops a luc_powerup
void luc_powerup_drop (edict_t *ent, gitem_t *item) 
{
	edict_t	*luc_powerup = Drop_Item(ent, item);

	luc_powerup->nextthink = level.time + 2;
	luc_powerup->think = luc_powerup_make_touchable;

	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
	ValidateSelectedItem(ent);
}

// drop whatever powerups the player is currently carrying
void luc_drop_powerups(edict_t *ent) 
{
	int i;

	for (i = 0 ; i < POWERUP_COUNT ; i++)
		if (ent->client->pers.inventory[luc_powerups[i].itemIndex])
			luc_powerup_drop(ent, &itemlist[luc_powerups[i].itemIndex]);
}


