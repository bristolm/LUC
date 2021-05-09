#include "g_local.h"
#include "luc_m_bots.h"

void temp_bot_think (edict_t *self);

// RSP 102499 - added sounds
static int	sound_sight;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;

// Main routines for all bots

void worker_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void worker_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


qboolean botEvalTarget(edict_t *self, vec3_t dest, int maxdist, int maxup, int maxdown, int maxside)
{
	vec3_t	target_dist, target_angles;
	float   aim_direction;

	VectorSubtract (dest, self->s.origin, target_dist);
	if (VectorLength(target_dist) > maxdist)   // how far is it?
		return false;

	vectoangles (target_dist, target_angles);  
	if (target_angles[0] < -180)
		target_angles[0] += 360;
	if ((target_angles[0] > maxup) || (target_angles[0] < maxdown))  // check up/down
		return false;

	aim_direction = self->s.angles[1] - target_angles[1];
	if (fabs(aim_direction) > maxside)  // check left to right aim
		return false;

	return true;
}


void temp_bot_thinkback (edict_t *self)
{
	if (--self->s.frame < FRAME_STAND_START);
	{
		self->s.frame = FRAME_STAND_START;
		self->nextthink = level.time + (FRAMETIME * random() * 10);
		self->think = temp_bot_think;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void temp_bot_think (edict_t *self)
{
	if (++self->s.frame > FRAME_BRACE_END);
	{
		self->s.frame = FRAME_BRACE_END;
		self->nextthink = level.time + (FRAMETIME * random() * 10);
		self->think = temp_bot_thinkback;
	}
	self->nextthink = level.time + FRAMETIME;
}

// Standing around ...

mframe_t bot_frames_stand [] =
{
	ai_stand, 0, NULL,	// 1
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,	// 7
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,	// 13
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL	// 18
};
mmove_t bot_move_stand = {FRAME_STAND_START, FRAME_STAND_END, bot_frames_stand, NULL};

void bot_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &bot_move_stand;
}

// Run
mframe_t bot_frames_run [] =
{	// MRB 111898 - added some REAL running
	ai_run, 15, NULL,	// 1
	ai_run, 15, NULL,
	ai_run, 15, NULL,
	ai_run, 15, NULL,
	ai_run, 15, NULL,
	ai_run, 15, NULL	// 6
};
mmove_t bot_move_run = {FRAME_RUN_START, FRAME_RUN_END, bot_frames_run, NULL};

void bot_run (edict_t *self)
{
	self->monsterinfo.currentmove = &bot_move_run;
}

void botUnlockFrom(edict_t *self, edict_t *locked)
{
	if (locked && (locked->locked_to == self))			// RSP 070400
	{
		locked->locked_to = NULL;
		self->monsterinfo.currentmove = &bot_move_run;	// RSP 030999
	}
}

// Walk
mframe_t bot_frames_walk [] =
{
	ai_walk, 4, NULL, // 1
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,

	ai_walk, 4, NULL, // 7
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL  // 12
};
mmove_t bot_move_walk = {FRAME_WALK_START, FRAME_WALK_END, bot_frames_walk, NULL};

void bot_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &bot_move_walk;
}

// Pain
mframe_t bot_frames_paina [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 7
};
mmove_t bot_move_paina = {FRAME_PAINA_START, FRAME_PAINA_END, bot_frames_paina, bot_run};

mframe_t bot_frames_painb [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
//	ai_move, 0, NULL,				// RSP 072900 - painb only has 5 frames, not 6
	ai_move, 0, NULL	// 5
};
mmove_t bot_move_painb = {FRAME_PAINB_START, FRAME_PAINB_END, bot_frames_painb, bot_run};

void bot_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	waypoint_check(self);	// RSP 071800

	if ((rand()&7) < (skill->value * 2) )
		return;		// no pain anims in nightmare and higher skills

	if (self->enemy)
		botUnlockFrom(self, self->enemy);

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 1;

	if (random() < 0.5)
		self->monsterinfo.currentmove = &bot_move_paina;
	else
		self->monsterinfo.currentmove = &bot_move_painb;

	// RSP 102499 - randomize pain sound
	if (rand()&1)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
}

void botResetToWalk(edict_t *self)
{
	self->monsterinfo.currentmove = &bot_move_walk;
	if (self->s.frame < FRAME_TLEFT_START)
		self->monsterinfo.nextframe = self->s.frame - FRAME_TRIGHT_START + FRAME_WALK_START + 1;
	else
		self->monsterinfo.nextframe = self->s.frame - FRAME_TLEFT_START + FRAME_WALK_START + 1;
}

// Pivot
mframe_t bot_frames_pivot [] =
{
	ai_run, 0, NULL, // 1
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,

	ai_run, 0, NULL, // 7
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL  // 12
};

mmove_t bot_move_tright = {FRAME_TRIGHT_START, FRAME_TRIGHT_END, bot_frames_pivot, NULL};
mmove_t bot_move_tleft = {FRAME_TLEFT_START, FRAME_TLEFT_END, bot_frames_pivot, NULL};

void workerbot_straightenout(edict_t *self)
{
	if (self->enemy && (self->enemy->deadflag != DEAD_DEAD))
	{	// go back to running

		self->monsterinfo.run(self);

		if (self->s.frame < FRAME_TLEFT_START)
			self->monsterinfo.nextframe = (int)((self->s.frame - FRAME_TRIGHT_START)/2) + FRAME_RUN_START;
		else
			self->monsterinfo.nextframe = (int)((self->s.frame - FRAME_TLEFT_START)/2) + FRAME_RUN_START;
	}
	else
	{	// go back to walking

		self->monsterinfo.walk(self);

		if (self->s.frame < FRAME_TLEFT_START)
			self->monsterinfo.nextframe = self->s.frame - FRAME_TRIGHT_START + FRAME_WALK_START;
		else
			self->monsterinfo.nextframe = self->s.frame - FRAME_TLEFT_START + FRAME_WALK_START;
	}
}


void bot_pivot (edict_t *self, float yaw)
{
	/* OK, this is an attempt at some pivoting code.  Thank goodness
	 * I made the same number of turning and walking frames .. run is half
	 * order in .h file is (ok, so I probably should not be doing it this way ...)
		FRAME_TRIGHT_START
		FRAME_TRIGHT_END
		FRAME_TLEFT_START
		FRAME_TLEFT_END
	 */ 

	int index;

	if ((self->s.frame <= FRAME_WALK_END) && (self->s.frame >= FRAME_WALK_START))
		index = self->s.frame - FRAME_WALK_START;
	else if ((self->s.frame <= FRAME_RUN_END) && (self->s.frame >= FRAME_RUN_START))
		index = (self->s.frame - FRAME_RUN_START) * 2;
	else
		index = -1;

	if (index > 0)
	{	/* turning far enough to care ... */

		if (yaw < -self->yaw_speed)	  /* turning right */
		{
			self->monsterinfo.currentmove = &bot_move_tright;
			self->monsterinfo.nextframe = FRAME_TRIGHT_START + index;
		}
		else if (yaw > self->yaw_speed) /* turning left */
		{
			self->monsterinfo.currentmove = &bot_move_tleft;
			self->monsterinfo.nextframe = FRAME_TLEFT_START + index;
		}
		else
			return;
	}
	else	/* Check to see if I need to undo the pivoting or change it ... */
	{		/* See if I'm currently in a turning frame ... (note - must be contiguous in .md2) */

		if ((self->s.frame >= FRAME_TRIGHT_START) && (self->s.frame <= FRAME_TLEFT_END))
			if (yaw < -self->yaw_speed)
			{		/* turning right */

				if (self->s.frame >= FRAME_TLEFT_START)  /* I HAD been turning left */
				{										/* so I have to reset things */
					self->monsterinfo.currentmove = &bot_move_tright;
					self->monsterinfo.nextframe = self->s.frame - FRAME_TLEFT_START	+ FRAME_TRIGHT_START;
				}
			}
			else if (yaw > self->yaw_speed)
			{		/* turning left */

				if (self->s.frame <= FRAME_TRIGHT_END)  /* I HAD been turning right */
				{										/* so I have to reset things */
					self->monsterinfo.currentmove = &bot_move_tleft;
					self->monsterinfo.nextframe =  self->s.frame - FRAME_TRIGHT_START + FRAME_TLEFT_START;
				}
			}
			else	/* stop pivoting and return to running or walking */
				workerbot_straightenout(self);	// RSP 072900 - this section became a routine
	}
}

void bot_dead (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;

	gi.linkentity (self);
}

mframe_t bot_frames_deatha [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 6
};
mmove_t bot_move_deatha = {FRAME_DEATHA_START, FRAME_DEATHA_END, bot_frames_deatha, bot_dead};

mframe_t bot_frames_deathb [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 5
};
mmove_t bot_move_deathb = {FRAME_DEATHB_START, FRAME_DEATHB_END, bot_frames_deathb, bot_dead};

// Bots should prolly override this function with their own by setting self->die after
// calling bot_spawnsetup.
void bot_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	self->monsterinfo.pivot = NULL;

	if (self->enemy)
		botUnlockFrom(self, self->enemy);

	// gib if hurt bad enough
	if (self->health <= self->gib_health)
	{
		for (n = 0 ; n < 6 ; n++)
			ThrowGib(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);

		// AJA 19980908 - Remove the main entity (only giblets left over) and make the gib sound.
		// Getting rid of the body eliminates the "overflow" errors.
		// AJA 19980909 - Well, that was really a hack, here's the real way to do it. Set the
		// DEAD_DEAD flag, and check that the monster isn't DEAD_DEAD before doing the
		// "regular" death code. You must call ThrowHead on this monster, this turns the
		// monster's edict into the head, and sets it to be freed in a little over ten
		// second from now.
		ThrowHead (self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		gi.sound(self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (random() <= 0.50)
		self->monsterinfo.currentmove = &bot_move_deatha;
	else
		self->monsterinfo.currentmove = &bot_move_deathb;	
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);	// RSP 102499
}


// RSP 092500
void monster_bot_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	sound_sight = gi.soundindex ("worker/sight.wav");
	sound_idle  = gi.soundindex ("worker/idle.wav");
	sound_pain1 = gi.soundindex ("worker/pain1.wav");
	sound_pain2 = gi.soundindex ("worker/pain2.wav");
	sound_die   = gi.soundindex ("worker/death.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/bots/worker/tris.md2");
}


void bot_spawnsetup(edict_t *self)
{
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	VectorSet (self->mins, -32, -32, -5);	// RSP 072900 - match model
	VectorSet (self->maxs, 32, 32, 26);		// RSP 072900 - match model

	self->gib_health = -30; // if bot_die is called then this is irrelevant
	self->max_health = self->health;		// RSP 092600
	self->mass = 150;		// RSP 053099 - reduced from 400
	self->yaw_speed = 7.5;

	self->pain = bot_pain;
	self->die = bot_die;

	// RSP 092600 - moved common routine settings in here from plasma, beam, and claw bots
	self->monsterinfo.pivot  = bot_pivot;
	self->monsterinfo.sight  = worker_sight;	// RSP 102488 - added sound
	self->monsterinfo.idle   = worker_idle;	// RSP 102488 - added sound
	self->monsterinfo.dodge  = NULL;
	self->monsterinfo.melee  = NULL;
	self->monsterinfo.search = NULL;

	self->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	// RSP 102499 - added worker sounds
	monster_bot_indexing (self);
}

