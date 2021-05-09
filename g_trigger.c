#include "g_local.h"
#include "luc_hbot.h"		// RSP 070700


void InitTrigger (edict_t *self)
{
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel (self, self->model);
	self->svflags = SVF_NOCLIENT;
}


// the wait time has passed, so set back up for another activation
void multi_wait (edict_t *ent)
{
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger (edict_t *ent)
{
	if (ent->nextthink)
		return;		// already been triggered

	G_UseTargets (ent, ent->activator);

	if (ent->wait > 0)	
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + ent->wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEdict;
	}
}

/*
==============================================================================
WPO 081999: Dreadlock trigger
trigger_hbot

==============================================================================
*/

// RSP 070200 - Added a one-time think routine to allow synchronization of
//              sound effect with movement of func_train locks on the dreadlock
//              station.
void trigger_dreadlock_think (edict_t *self)
{
	G_UseTargets(self,self->activator);	// RSP 070200 - allow it to have targets
	G_FreeEdict(self);
}

void trigger_dreadlock_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client && (other->health > 0))	// RSP 062900 - non-players cause no action
	{
		// if the player doesn't already own a dreadlock, give him one

		if (other->client->pers.dreadlock == DLOCK_UNAVAILABLE)
		{
			other->client->pers.dreadlock = DLOCK_DOCKED;	// set the dreadlock to inactive

			// RSP 101400 - when first dreadlock is picked up, kick off the implant sequence
			if (!(other->flags & FL_FIRSTDLOCK))
			{
				other->implant_state = 1;
				other->client->implant_framenum = level.framenum + IMPLANT_FADEOUT_LENGTH;	// frames to fade out
				other->flags |= FL_FIRSTDLOCK;					// RSP 101400
				gi.configstring (CS_STATUSBAR, "");				// Turn off the status bar
			}
			else if (!(self->spawnflags & SPAWNFLAG_DL_NOMESSAGE))	// RSP 101600
				HelpMessage(other,"You now have a Dreadlock!");		// RSP 091600 - replaces gi.centerprintf

			// Send sound now, but wait 0.2s before doing anything else

			gi.sound(self, CHAN_VOICE, gi.soundindex("hbot/hbotfree.wav"), 1, ATTN_NORM, 0);
			self->think = trigger_dreadlock_think;
			self->nextthink = level.time + 2*FRAMETIME;
		}
		else
		{	// Already have a dreadlock; send a sound, but not too often

			if (self->think)	// Already touched this trigger
				return;
			if (level.time < self->touch_debounce_time)
				return;
			self->touch_debounce_time = level.time + 5.0;

			if (!(self->spawnflags & SPAWNFLAG_DL_NOMESSAGE))	// RSP 101600
				HelpMessage(other,"You already have a Dreadlock!");	// RSP 091600 - replaces gi.centerprintf

			gi.sound(other,CHAN_AUTO,gi.soundindex("hbot/locked.wav"),1,ATTN_NORM,0);
		}
	}
}


/*QUAKED trigger_hbot (.5 .5 .5) ? nomessage
Gives the player a dreadlock (fka helperbot).
If nomessage is not set, the player will see a message indicating
they now have a dreadlock.
*/

void SP_trigger_hbot (edict_t *self)
{
	InitTrigger (self);
	self->touch = trigger_dreadlock_touch;
	gi.linkentity (self);
}


/*
==============================================================================

trigger_scenario_end

==============================================================================
*/

// RSP 092500
/*QUAKED trigger_scenario_end (.5 .5 .5) ? START_OFF TOGGLE
This will fade the screen to black, then bring it back to normal.
This is a one-time trigger. It will either take the player back
to a returnpoint, or it will move the player forward. In order
to know where to move, set

target:     path_corner to move forward to
pathtarget: path_corner to move back to
*/

// This new trigger will be used during the multiple door sequences in the
// final map. It is placed at the end of each endgame scenario, and is used
// to bring the player back to the returnpoint location so they can
// try another door, or move them forward when all scenarios are complete.

void scenario_end_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity (self);

	if (!(self->spawnflags & SPAWNFLAG_TS_TOGGLE))
		self->use = NULL;
}

void scenario_end_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other->client)
		return;	// non-players cause no action
	
	if (other->health <= 0)
		return;	// dead players cause no action
	
	other->flags |= FL_NODROWN;			// can't drown at this point
	other->client->scenario_framenum = level.framenum + SCENARIO_FADEOUT_LENGTH;	// frames to fade out
	other->scenario_state = 1;	// 1 = fading out
								// 2 = moving to new location
								// 3 = fading in

	gi.configstring (CS_STATUSBAR, "");	// Turn off the status bar

	// turn off the current scenario indicator
	other->endgame_flags &= ~CURRENT_SCENARIO_MASK;

	// If all scenarios are finished, move forward.
	// Otherwise, move the player back to the return point.

	if ((other->endgame_flags & SCENARIO_ALL) == SCENARIO_ALL)
	{
		other->cell = G_Find (NULL, FOFS(targetname), self->target);
		if (!other->cell)
			gi.dprintf ("No forward dest for trigger_scenario_end\n");
	}
	else
	{
		other->cell = G_Find (NULL, FOFS(targetname), self->pathtarget);
		if (!other->cell)
			gi.dprintf ("No return dest for trigger_scenario_end\n");
	}

	// Recall dreadlock if he's out there
	if (other->client->pers.dreadlock == DLOCK_LAUNCHED) 
        Dreadlock_recall(other->client->pers.Dreadlock,false); // recall the Dreadlock

	// Turn off any active slowdeath
	other->flags &= ~FL_SLOWDEATH;

	G_FreeEdict(self);
}


void SP_trigger_scenario_end (edict_t *self)
{
	InitTrigger (self);
	self->touch = scenario_end_touch;

	// Added triggerable spawn flag
	if (self->spawnflags & SPAWNFLAG_TS_START_OFF)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & SPAWNFLAG_TS_TOGGLE)
		self->use = scenario_end_use;

	gi.linkentity (self);
}


/*
==============================================================================

trigger_end_scenario2

==============================================================================
*/

// RSP 101900
/*QUAKED trigger_end_scenario2 (.5 .5 .5) ?
This is a one-time trigger. If the player activates it, it ends the game and 
plays a *.cin movie. If something else activates it, the player wins, and
the trigger activates its target, which should be a door that opens to let the
player out. This trigger is placed inside the energy cell that the player is
invited to walk into.

map: name of *.cin
*/

void end_scenario2_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client)
	{		// player loses
		if (other->health <= 0)
			return;	// dead players cause no action
		BeginIntermission (self);
	}
	else if (!(other->svflags & SVF_MONSTER) && (other->identity != ID_DLOCK))
		return;
	else	// player wins
	{
		G_UseTargets(self,other);	// trigger targets
		other->flags |= FL_FROZEN;	// freeze whatever triggered us
		other->movetype = MOVETYPE_NOCLIP;	// make it non-solid so it doesn't block the door
		other->solid = SOLID_NOT;	// make it non-solid so it doesn't block the door
	}

	G_FreeEdict(self);	// only one use
}

void SP_trigger_end_scenario2 (edict_t *self)
{
	InitTrigger (self);
	self->touch = end_scenario2_touch;
	gi.linkentity (self);
}


/*
==============================================================================

trigger_multiple

==============================================================================
*/

void Use_Multi (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->activator = activator;
	multi_trigger (ent);
}

void Touch_Multi (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client)
	{
		if (self->spawnflags & SPAWNFLAG_TMUL_NOT_PLAYER)
			return;

		// If this is a message from the Core AI, the player has to have picked
		// up their first dreadlock in order to display it.
		if (self->spawnflags & SPAWNFLAG_COREAI)
			if (!(other->flags & FL_FIRSTDLOCK))
				return;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!(self->spawnflags & SPAWNFLAG_TMUL_MONSTER))
			return;
	}
	else 
		// WPO 100699 - allow dreadlocks to trigger
		if (other->identity != ID_DLOCK)
			return;

	if (!VectorCompare(self->movedir, vec3_origin))
	{
		vec3_t forward;

		AngleVectors(other->s.angles, forward, NULL, NULL);
		if (_DotProduct(forward, self->movedir) < 0)
			return;
	}

	// WPO 100699 - if dreadlock triggered this, set activator to dreadlock's owner
	if (other->identity != ID_DLOCK)
		self->activator = other;
	else
		self->activator = other->dlockOwner;

	multi_trigger (self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED COREAI
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.

If COREAI is set, this message will only be seen if the player
has had at least one dreadlock.

wait: Seconds between triggerings. (.2 default)
sounds:
 -1)	quiet
  1)	secret
  2)	beep beep
  3)	large switch
  4)

set "message" to text string

If TRIGGERED, this trigger must be triggered before it is live.
*/
void trigger_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	gi.linkentity (self);
}

void SP_trigger_multiple (edict_t *ent)
{
	// WPO 052400 - Modified meaning of 'sounds' (-1 means no sound)
	if (ent->sounds == -1)
		ent->noise_index = -1;
	else if (ent->sounds == 1)
		ent->noise_index = gi.soundindex ("misc/secret.wav");
	else if (ent->sounds == 2)
		ent->noise_index = gi.soundindex ("misc/talk.wav");
	else if (ent->sounds == 3)
		ent->noise_index = gi.soundindex ("misc/butn4.wav");
	
	if (!ent->wait)
		ent->wait = 0.2;
	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->svflags |= SVF_NOCLIENT;

	if (ent->spawnflags & SPAWNFLAG_TMUL_TRIGGERED)
	{
		ent->solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	if (!VectorCompare(ent->s.angles, vec3_origin))
		G_SetMovedir (ent->s.angles, ent->movedir);

	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}


/*
==============================================================================

trigger_once

==============================================================================
*/

/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED COREAI
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.

If COREAI is set, this message will only be seen if the player
has had at least one dreadlock.

sounds:
-1)	quiet
 1)	secret
 2)	beep beep
 3)	large switch

message: string to be displayed when triggered
*/

void SP_trigger_once(edict_t *ent)
{
	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (ent->spawnflags & SPAWNFLAG_T1_OLDTRIGGER)
	{
		vec3_t	v;

		VectorMA (ent->mins, 0.5, ent->size, v);
		ent->spawnflags &= ~SPAWNFLAG_T1_OLDTRIGGER;
		ent->spawnflags |= SPAWNFLAG_T1_NEWTRIGGER;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", ent->classname, vtos(v));
	}

	ent->wait = -1;
	SP_trigger_multiple (ent);
}


/*
==============================================================================

trigger_relay

==============================================================================
*/

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
void trigger_relay_use (edict_t *self, edict_t *other, edict_t *activator)
{
	G_UseTargets (self, activator);
}

void SP_trigger_relay (edict_t *self)
{
	self->use = trigger_relay_use;
}


/*
==============================================================================

trigger_key

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires its targets if player has the proper key.

item: specifies the required key, for example "key_glyph03"
*/
void trigger_key_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int		index;
	char	msg[512];	// RSP 091600

	if (!self->item)
		return;
	if (!activator->client)
		return;

	index = ITEM_INDEX(self->item);
	if (!activator->client->pers.inventory[index])
	{
		if (level.time < self->touch_debounce_time)
			return;
		self->touch_debounce_time = level.time + 5.0;
		sprintf(msg,"You need %s",self->item->pickup_name);	// RSP 091600
		HelpMessage(activator,msg);	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf (activator, "You need %s", self->item->pickup_name);	// RSP 070799 - removed "the" from the string
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keytry.wav"), 1, ATTN_NORM, 0);
		return;
	}

	gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keyuse.wav"), 1, ATTN_NORM, 0);
	if (coop->value)
	{
		int		player;
		edict_t	*ent;

		// WPO 230100 - Changed name from key_power_cube
		if (strcmp(self->item->classname, "key_explosive") == 0)
		{
			int	cube;

			for (cube = 0 ; cube < 8 ; cube++)
				if (activator->client->pers.power_cubes & (1 << cube))
					break;
			for (player = 1 ; player <= game.maxclients ; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				if (ent->client->pers.power_cubes & (1 << cube))
				{
					ent->client->pers.inventory[index]--;
					ent->client->pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else
		{
			for (player = 1 ; player <= game.maxclients ; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				ent->client->pers.inventory[index] = 0;
			}
		}
	}
	else
	{
		activator->client->pers.inventory[index]--;
	}

	G_UseTargets (self, activator);

	self->use = NULL;
}

void SP_trigger_key (edict_t *self)
{
	if (!st.item)
	{
		gi.dprintf("no key item for trigger_key at %s\n", vtos(self->s.origin));
		return;
	}
	self->item = FindItemByClassname (st.item);

	if (!self->item)
	{
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item, vtos(self->s.origin));
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s at %s has no target\n", self->classname, vtos(self->s.origin));
		return;
	}

	gi.soundindex ("misc/keytry.wav");
	gi.soundindex ("misc/keyuse.wav");

	self->use = trigger_key_use;
}


/*
==============================================================================

trigger_counter

==============================================================================
*/

// RSP 051100 - Added RESET spawnflag

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage reset
Acts as an intermediary for an action that takes multiple inputs.

If NOMESSAGE is not set, it will print "1 more..." etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of its targets and remove
itself, unless RESET is set. If RESET is set, trigger_counter will stay in the game, and its counter will
be reset to the original "count".
*/

void trigger_counter_use(edict_t *self, edict_t *other, edict_t *activator)
{
	char msg[512];	// RSP 091600

	if (self->count == 0)
		return;

	self->count--;

	if (self->count)
	{
		if (!(self->spawnflags & SPAWNFLAG_TC_NOMESSAGE))
		{
			sprintf(msg,"%i more to go...",self->count);	// RSP 091600
			HelpMessage(activator,msg);	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(activator, "%i more to go...", self->count);
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	
	if (!(self->spawnflags & SPAWNFLAG_TC_NOMESSAGE))
	{
		HelpMessage(activator, "Sequence completed!");	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(activator, "Sequence completed!");
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
	self->activator = activator;
	multi_trigger (self);

	// RSP 051100 - allow the trigger_counter to stick around
	if (self->spawnflags & SPAWNFLAG_TC_RESET)
	{
		self->nextthink = 0;		// undo what multi_trigger did
		self->think = NULL;			// undo what multi_trigger did
		self->count = self->health;	// reset counter
	}
}

void SP_trigger_counter (edict_t *self)
{
	self->wait = -1;
	if (!self->count)
		self->count = 2;

	self->health = self->count;			// RSP 051100

	self->use = trigger_counter_use;
}


/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (edict_t *ent)
{
	// we must have some delay to make sure our use targets are present
	if (ent->delay < 0.2)
		ent->delay = 0.2;
	G_UseTargets(ent, ent);
}


/*
==============================================================================

trigger_push

==============================================================================
*/

static int windsound;

// WPO 051800 - Added function for triggerable spawn flag
void push_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity (self);

	if (!(self->spawnflags & SPAWNFLAG_TP_TOGGLE))
		self->use = NULL;
}

void trigger_push_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (strcmp(other->classname, "grenade") == 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);
	}
	else if (other->health > 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);

		if (other->client)
		{
			// don't take falling damage immediately from this
			VectorCopy (other->velocity, other->client->oldvelocity);
			if (other->fly_sound_debounce_time < level.time)
			{
				other->fly_sound_debounce_time = level.time + 1.5;
				gi.sound (other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}
	if (self->spawnflags & SPAWNFLAG_TP_PUSH_ONCE)
		G_FreeEdict (self);
}

// WPO 051800 - Added triggerable spawn flag
/*QUAKED trigger_push (.5 .5 .5) ?  PUSH_ONCE START_OFF TOGGLE
Pushes the player

speed:	defaults to 1000
*/
void SP_trigger_push (edict_t *self)
{
	InitTrigger (self);
	windsound = gi.soundindex ("misc/windfly.wav");
	self->touch = trigger_push_touch;
	if (!self->speed)
		self->speed = 1000;

	// WPO 051800 - Added triggerable spawn flag
	if (self->spawnflags & SPAWNFLAG_TP_START_OFF)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & SPAWNFLAG_TP_TOGGLE)
		self->use = push_use;

	gi.linkentity (self);
}


/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

dmg: points of damage each server frame, default 5 (whole numbers only)
*/
void hurt_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity (self);

	if (!(self->spawnflags & SPAWNFLAG_TH_TOGGLE))
		self->use = NULL;
}


void hurt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int dflags;

	if (!other->takedamage)
		return;

	if (self->timestamp > level.time)
		return;

	if (self->spawnflags & SPAWNFLAG_TH_SLOW)
		self->timestamp = level.time + 1;
	else
		self->timestamp = level.time + FRAMETIME;

	if (!(self->spawnflags & SPAWNFLAG_TH_SILENT))
	{
		if ((level.framenum % 10) == 0)
			gi.sound (other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
	}

	if (self->spawnflags & SPAWNFLAG_TH_NO_PROTECTION)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, self->dmg, dflags, MOD_TRIGGER_HURT);
}

void SP_trigger_hurt (edict_t *self)
{
	InitTrigger (self);

	self->noise_index = gi.soundindex ("world/electro.wav");
	self->touch = hurt_touch;

	if (!self->dmg)
		self->dmg = 5;

	if (self->spawnflags & SPAWNFLAG_TH_START_OFF)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & SPAWNFLAG_TH_TOGGLE)
		self->use = hurt_use;

	gi.linkentity (self);
}


/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entity's gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/

void trigger_gravity_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	other->gravity = self->gravity;
}

void SP_trigger_gravity (edict_t *self)
{
	if (st.gravity == 0)
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n", vtos(self->s.origin));
		G_FreeEdict  (self);
		return;
	}

	InitTrigger (self);
	self->gravity = atoi(st.gravity);
	self->touch = trigger_gravity_touch;
}


/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle

speed:  default to 200, the speed thrown forward
height: default to 200, the speed thrown upwards
*/

void trigger_monsterjump_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->flags & (FL_FLY | FL_SWIM))
		return;
	if (other->svflags & SVF_DEADMONSTER)
		return;
	if (!(other->svflags & SVF_MONSTER))
		return;

// set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;
	
	if (!other->groundentity)
		return;
	
	other->groundentity = NULL;
	other->velocity[2] = self->movedir[2];
}

void SP_trigger_monsterjump (edict_t *self)
{
	if (!self->speed)
		self->speed = 200;
	if (!st.height)
		st.height = 200;
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;
	InitTrigger (self);
	self->touch = trigger_monsterjump_touch;
	self->movedir[2] = st.height;
}

/*
==============================================================================

trigger_capture

RSP 092900 - new trigger for the endgame

==============================================================================
*/

/*QUAKED trigger_capture (1 0 0) ?
Sets up a different endgame scenario at each use.
*/
void capture_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int			i;
	qboolean	choosing;

	if (!other->client)
		return;

	if (level.time < self->touch_debounce_time)
		return;

	self->touch_debounce_time = level.time + 10;

	if (other->health <= 0)
		return;	// dead players cause no action

printf("%5.2f: capture_touch with endgame_flags of %x\n",level.time,other->endgame_flags);	// RSP DEBUG

	// Set the scenario flags. SCENARIO_XX tells you which scenarios
	// you've played. CURRENT_SCENARIO_XX tells you which scenario you're
	// currently playing. The first bit stays with you once you're finished;
	// the second bit gets cleared.
	choosing = true;
	while (choosing)
	{
		i = (rand() % 3) + 1;
		switch(i)
		{
		case 1:
printf("Trying case 1\n");	// RSP DEBUG
			if (other->endgame_flags & SCENARIO_01)	// try again if you've already played this one
				break;
			other->cell = G_Find (NULL, FOFS(targetname), "endgame_dest_01");
printf("Playing case 1\n");	// RSP DEBUG
			if (other->cell)
			{
				other->endgame_flags |= (SCENARIO_01|CURRENT_SCENARIO_01);
				choosing = false;
			}
			else
				gi.dprintf ("Couldn't find destination %d\n",i);
			break;
		case 2:
printf("Trying case 2\n");	// RSP DEBUG
			if (other->endgame_flags & SCENARIO_02)	// try again if you've already played this one
				break;
			other->cell = G_Find (NULL, FOFS(targetname), "endgame_dest_02");
printf("Playing case 2\n");	// RSP DEBUG
			if (other->cell)
			{
				other->endgame_flags |= (SCENARIO_02|CURRENT_SCENARIO_02);
				choosing = false;
			}
			else
				gi.dprintf ("Couldn't find destination %d\n",i);
			break;
		case 3:
printf("Trying case 3\n");	// RSP DEBUG
			if (other->endgame_flags & SCENARIO_03)	// try again if you've already played this one
				break;
printf("Playing case 3\n");	// RSP DEBUG
			other->cell = G_Find (NULL, FOFS(targetname), "endgame_dest_03");
			if (other->cell)
			{
				other->endgame_flags |= (SCENARIO_03|CURRENT_SCENARIO_03);
				choosing = false;
			}
			else
				gi.dprintf ("Couldn't find destination %d\n",i);
			break;
		}
	}
printf("Traveling to %s\n\n\n",other->cell->targetname);	// RSP DEBUG

	other->cell->target_ent = other;	// RSP 101900 - link destination to player
	other->client->capture_framenum = level.framenum + 10;
	other->flags |= FL_CAPTURED;

	// If the player is entering the final scenario, you no longer need this trigger
	if ((other->endgame_flags & SCENARIO_ALL) == SCENARIO_ALL)
{
		G_FreeEdict(self);
printf("deleting trigger_capture\n\n\n");	// RSP DEBUG
}
}


void SP_trigger_capture (edict_t *ent)
{
	InitTrigger(ent);
	ent->touch = capture_touch;
	ent->touch_debounce_time = 0;
	gi.linkentity (ent);
}

/*
==============================================================================

trigger_slowdeath

RSP 110400 - new trigger for the endgame

==============================================================================
*/

/*QUAKED trigger_slowdeath (1 0 0) ?
When triggered, waits 60 seconds, then subtracts one unit
of health each second. For the endgame.
*/
void slowdeath_think (edict_t *self)
{
	// If the enemy has already exited the scenario, then don't
	// trigger the slow death.
	if (self->enemy->endgame_flags & CURRENT_SCENARIO_MASK)
		self->enemy->flags |= FL_SLOWDEATH;
	G_FreeEdict(self);
}

void slowdeath_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->think)	// already triggered?
		return;

	if (!other->client)
		return;

	self->enemy = other;
	self->think = slowdeath_think;
	self->nextthink = level.time + 60;
}

void SP_trigger_slowdeath (edict_t *self)
{
	InitTrigger(self);
	self->touch = slowdeath_touch;
	gi.linkentity (self);
}


