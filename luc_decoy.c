#include "g_local.h"
#include "m_player.h"
#include "luc_decoy.h"
#include "luc_powerups.h"

void Decoy_Think (edict_t *ent);

void decoy_remove (edict_t *ent)
{
	HelpMessage(ent->owner,"Holo Decoy Deactivated!\n");	// RSP 091600 - replaces gi.centerprintf
//	gi.centerprintf(ent->owner,"Holo Decoy Deactivated!\n");

	ent->owner->decoy = NULL;

	G_FreeEdict(ent); 

	// RSP 110299 - something to watch out for:
	// It looks like any monsters currently shooting at this decoy gracefully
	// forget about it when it leaves the game. A safer way of doing this would
	// be to explicitly find each monster targetting this decoy and remove the
	// decoy as the enemy, but that seems to be taken care of elsewhere. When an
	// enemy is no longer there, the monster recovers just fine.
} 


void SP_Decoy (edict_t *self)
{
	edict_t *monster;	// RSP 110299
	int		i;			// RSP 110399
	edict_t *decoy;		// RSP 081800

	if (!luc_ent_has_powerup(self, POWERUP_DECOY))
		return;

	if (self->decoy) 
	{  
		// RSP 081800 - We're turning the decoy off, so save any leftover time
		// WPO 141199 - Make decoy total duration fixed time
		self->client->decoy_timeleft = (self->decoy->timestamp - level.time)*10;	// RSP 091700
		decoy_remove (self->decoy);					// RSP 110299 - use common routine
		return; 
	}

	HelpMessage(self,"Holo Decoy Activated!\n");	// RSP 091600 - replaces gi.centerprintf
//	gi.centerprintf (self,"Holo Decoy Activated!\n");

	decoy = G_Spawn();
	VectorCopy(self->s.origin,decoy->s.origin);
	VectorCopy(self->s.angles, decoy->s.angles);	// RSP 110299 - decoy faces my way
	decoy->viewheight = self->viewheight;			// RSP 110299 - for visibility check
	decoy->classname = "decoy";
	decoy->takedamage = DAMAGE_NO;
	decoy->movetype = MOVETYPE_TOSS;
	decoy->mass = 200;
	decoy->solid = SOLID_NOT;						// RSP 081800
	decoy->health = 100;							// RSP 110299 - monsters won't recognize decoy w/o health
	decoy->deadflag = DEAD_NO;
//	decoy->clipmask = MASK_PLAYERSOLID;				// RSP 081800
	decoy->model = self->model;
	decoy->s.modelindex = self->s.modelindex;
	decoy->s.modelindex2 = self->s.modelindex2;
	decoy->s.frame = FRAME_stand01;					// RSP 081800 - start with standing animation
	decoy->decoy_animation = 0;						// RSP 081800
	decoy->s.skinnum = self->s.skinnum;				// WPO 281099 'magic' field to get proper model
	decoy->think = Decoy_Think;
	decoy->nextthink = level.time + FRAMETIME;

	// WPO 141199 - You get a fixed time that you can display a decoy. If you use and recall the
	// decoy several times, the sum of the individual display times can't exceed the total 
	// fixed time. (i.e. the powerup only has a certain amount of energy in it.)
	decoy->timestamp = level.time + self->client->decoy_timeleft/10;	// RSP 091700

	decoy->owner = self;
	VectorSet(decoy->mins, -16, -16, -24);
	VectorSet(decoy->maxs, 16, 16, 32);
	VectorClear(decoy->velocity);
	decoy->identity = ID_DECOY;
	decoy->flags |= FL_PASSENGER;					// possible BoxCar passenger

	self->decoy = decoy;							// RSP 081800 - tie to player

	gi.linkentity(decoy);

	// RSP 110299 - recognize decoy
	// find all monsters that currently have this player
	// as an enemy, if not in deathmatch, where there are no monsters

	if (deathmatch->value)
		return;

	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		monster = &g_edicts[i];

		// If this monster has this player as an enemy, shift the
		// monster's enemy to the decoy if the monster can see either the player
		// or the decoy.
		if ((monster->svflags & SVF_MONSTER) && (monster->enemy == self))
			{
				if (visible(monster,self) || visible(monster,decoy))
				{
					Forget_Player(monster);
					monster->enemy = monster->goalentity = decoy;	// Look for decoy
				}	
			}
	}
}


#define DECOY_ANIMATIONS  8
#define DECOY_FRAME_START 0
#define DECOY_FRAME_END   1

int decoy_animation_frames[DECOY_ANIMATIONS][2] = {
	FRAME_stand01,  FRAME_stand40,
	FRAME_salute01, FRAME_salute11,
	FRAME_stand01,  FRAME_stand40,
	FRAME_stand01,  FRAME_stand40,
	FRAME_wave01,   FRAME_wave11,
	FRAME_stand01,  FRAME_stand40,
	FRAME_stand01,  FRAME_stand40,
	FRAME_point01,  FRAME_point12
};

void Decoy_Think (edict_t *ent)
{     
	if (level.time > ent->timestamp)
	{
		// RSP 081800 - The decoy has used up all its fixed time, so goodbye.
		// WPO 141199 - Make decoy total duration fixed time
		luc_ent_used_powerup(ent->owner, POWERUP_DECOY); 
		ent->owner->client->decoy_timeleft = 0;
		decoy_remove(ent);
		return;
	}

	// RSP 081800 - rewritten to include standing animation and remove flip
	if (ent->s.frame < decoy_animation_frames[ent->decoy_animation][DECOY_FRAME_END])
		ent->s.frame++;
	else
	{
		if (++ent->decoy_animation == DECOY_ANIMATIONS)
			ent->decoy_animation = 0;
		ent->s.frame = decoy_animation_frames[ent->decoy_animation][DECOY_FRAME_START];
	}

	ent->nextthink = level.time + FRAMETIME;
}



        
