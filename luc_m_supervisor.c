/*
luc_m_supervisor.c - This file contains the implementation of the Supervisor Bot
for the LUC project. This enemy is capable of directing the attacks of other
bots, repairing/recharging other bots, and attacking the player directly. It is
initially protected by a powerscreen.
*/

#include "g_local.h"
#include "luc_m_supervisor.h"

#define SUPERVISOR_RADIUS	512
#define SUPERVISOR_WALK		20
#define SUPERVISOR_RUN		30

// 19980810 AJA - Values returned by supervisor_recharge_ok
// Described the status of the power cord's path to target.
enum eRechargePath
{
	PATH_OK,		// Path is within distance/angular bounds
					// (says nothing about obstructions)
	PATH_TOO_FAR,	// Path is too long (target out of range)
	PATH_BAD_FOV	// Path is outside bot's FOV, but within range
					// (ie. bot is just facing the wrong way)
};

// SoundIndex values for common sounds.
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_launch;
static int	sound_impact;
static int	sound_suck;
static int	sound_reelin;
static int	sound_sight;
static int	sound_idle;


// Forward declaration of some AI functions.
void supervisor_stand (edict_t *self);
void supervisor_start_run (edict_t *self);
void supervisor_run (edict_t *self);
void supervisor_stop_run (edict_t *self);
void supervisor_start_walk (edict_t *self);
void supervisor_walk (edict_t *self);
void supervisor_attack (edict_t *self);
void supervisor_decide (edict_t *self);
void supervisor_order_to_attack (edict_t *self, edict_t *other, int sight_flag);
void supervisor_dead_drop (edict_t *self);
void supervisor_start_chase (edict_t *self);
void supervisor_chase (edict_t *self);
void supervisor_stop_chase (edict_t *self);
void supervisor_face (edict_t *self);
void supervisor_stop_face (edict_t *self);

// Decision-related functions.
edict_t *supervisor_recharge_target (edict_t *self);
enum eRechargePath supervisor_recharge_ok (edict_t *start, edict_t *end);
void supervisor_chase_decide (edict_t *self);

// Play the launch sound when the "power cable" is launched at another bot.
void supervisor_launch (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_launch, 1, ATTN_NORM, 0);
}

/* RSP 102799 - not used
// Play a tap sound when the bot gets impatient.
void supervisor_tap (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}
 */

// RSP 062200 - Allow supervisor to rotate randomly now and then
//              when standing
void supervisor_rotate(edict_t *self)
{
	if (self->spawnflags & SPAWNFLAG_AMBUSH)	// No rotation if AMBUSH set
		return;
	
	// if debounce is not -1, and level.time hasn't reached it yet, supervisor
	// is facing the way we want, and it's not time to change yet.

	if (self->pain_debounce_time > -1)
	{
		if (level.time < self->pain_debounce_time)
			return;

		// if level.time has reached debounce, then it's time to change the angle

		self->ideal_yaw = random()*360;
	}

	M_ChangeYaw (self);
	if (self->ideal_yaw == self->s.angles[YAW])
		self->pain_debounce_time = level.time + 2;	// stop turning for awhile
	else
		self->pain_debounce_time = -1;				// keep turning

	gi.linkentity (self);	// register the changes
}

//
// STAND CODE
//

// Stand frames
// MRB 080898 updated for new .h - took out 'tap' for the time being
mframe_t supervisor_frames_stand [] =
{
	ai_stand, 0, supervisor_rotate,	// RSP 062200 - added these rotation checks	// 1
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_decide,	// RSP 032599 - try to get supervisor's attention	// 10
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate,
	ai_stand, 0, supervisor_rotate	// 20
};
mmove_t	supervisor_move_stand =
{
	FRAME_stand1,
	FRAME_stand20,
	supervisor_frames_stand,
	supervisor_stand	// stay in "stand" state until disturbed.
};

// When we're idle, go into the "stand" state.
void supervisor_idle (edict_t *self)
{ 
	//self->monsterinfo.currentmove = &supervisor_move_start_fidget;
	self->monsterinfo.currentmove = &supervisor_move_stand;
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);	// RSP 063000
}

// Go into the "stand" state.
void supervisor_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_stand;
}


//
// RUN CODE
//

// Stop running frames
mframe_t supervisor_frames_stop_run [] =
{	
	ai_run, SUPERVISOR_RUN, NULL,		// 10
	ai_run, SUPERVISOR_RUN,	NULL,
	ai_run, (SUPERVISOR_RUN/2), NULL,
	ai_run, (SUPERVISOR_RUN/4),  NULL,
	ai_run, 2,  NULL,
	ai_run, 0,  NULL					// 15
};
mmove_t supervisor_move_stop_run =
{
	FRAME_run10,
	FRAME_run15,
	supervisor_frames_stop_run,
	supervisor_decide
};

// Running frames
mframe_t supervisor_frames_run [] =
{
	ai_run, SUPERVISOR_RUN, NULL,	// 3
	ai_run, SUPERVISOR_RUN, NULL,
	ai_run, SUPERVISOR_RUN, NULL,
	ai_run, SUPERVISOR_RUN, NULL,
	ai_run, SUPERVISOR_RUN, NULL,
	ai_run, SUPERVISOR_RUN, NULL,
	ai_run, SUPERVISOR_RUN, NULL	// 9
};
mmove_t supervisor_move_run =
{
	FRAME_run3,
	FRAME_run9,
	supervisor_frames_run,
	supervisor_stop_run
};

// Starting to run frames
mframe_t supervisor_frames_start_run [] =
{
	ai_run, 0,	NULL,					// 1
	ai_run, (SUPERVISOR_RUN/2), NULL	// 2
};
mmove_t supervisor_move_start_run =
{
	FRAME_run1,
	FRAME_run2,
	supervisor_frames_start_run,
	supervisor_run
};

void supervisor_start_run (edict_t *self)
{	
	// If we were told to run, but we are standing ground, stay put.
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &supervisor_move_stand;
	else
		self->monsterinfo.currentmove = &supervisor_move_start_run;
}

void supervisor_run (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_run;
}

void supervisor_stop_run (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_stop_run;
}


//
// WALK CODE
//

// Walk frames
mframe_t supervisor_frames_walk [] =
{
	ai_walk, SUPERVISOR_WALK, NULL,	// 3
	ai_walk, SUPERVISOR_WALK, NULL,
	ai_walk, SUPERVISOR_WALK, NULL,
	ai_walk, SUPERVISOR_WALK, NULL,
	ai_walk, SUPERVISOR_WALK, NULL,
	ai_walk, SUPERVISOR_WALK, NULL,
	ai_walk, SUPERVISOR_WALK, NULL	// 9
};
mmove_t supervisor_move_walk =
{
	FRAME_run3,
	FRAME_run9,
	supervisor_frames_walk,
	supervisor_walk
};

// Starting to walk frames
mframe_t supervisor_frames_start_walk [] =
{
	ai_walk, 0,	NULL,								// 1
	ai_walk, (SUPERVISOR_WALK/2), supervisor_walk	// 2
};
mmove_t supervisor_move_start_walk =
{
	FRAME_run1,
	FRAME_run2,
	supervisor_frames_start_walk,
	NULL
};

// Stop walking frames
mframe_t supervisor_frames_stop_walk [] =
{	
	ai_walk, (SUPERVISOR_WALK * 0.6) , NULL,	// 10
	ai_walk, (SUPERVISOR_WALK * 0.3) , NULL,
	ai_walk, 0, NULL,
	ai_walk, 0, NULL,
	ai_walk, 0, NULL,
	ai_walk, 0, NULL							// 15
};
mmove_t supervisor_move_stop_walk =
{
	FRAME_run10,
	FRAME_run15,
	supervisor_frames_stop_walk,
	NULL
};

void supervisor_start_walk (edict_t *self)
{	
	self->monsterinfo.currentmove = &supervisor_move_start_walk;
}

void supervisor_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_walk;
}


//
// CHASE CODE
//
// 19980810 AJA - All new. This is for when the supervisor is trying to
// maneuver to get into a position to recharge a bot (its current "enemy").

// Stop chasing frames
mframe_t supervisor_frames_stop_chase [] =
{	
	ai_charge, SUPERVISOR_WALK, NULL,		// 10
	ai_charge, SUPERVISOR_WALK,	NULL,
	ai_charge, (SUPERVISOR_WALK/2), NULL,
	ai_charge, (SUPERVISOR_WALK/4),  NULL,
	ai_charge, 2,  NULL,
	ai_charge, 0,  NULL						// 15
};
mmove_t supervisor_move_stop_chase =
{
	FRAME_walk10,
	FRAME_walk15,
	supervisor_frames_stop_chase,
	supervisor_decide
};

// Chasing frames
mframe_t supervisor_frames_chase [] =
{
	ai_charge, SUPERVISOR_WALK, NULL,	// 3
	ai_charge, SUPERVISOR_WALK, NULL,
	ai_charge, SUPERVISOR_WALK, NULL,
	ai_charge, SUPERVISOR_WALK, supervisor_chase_decide,
	ai_charge, SUPERVISOR_WALK, NULL,
	ai_charge, SUPERVISOR_WALK, NULL,
	ai_charge, SUPERVISOR_WALK, NULL	// 9
};
mmove_t supervisor_move_chase =
{
	FRAME_walk3,
	FRAME_walk9,
	supervisor_frames_chase,
	supervisor_chase_decide
};

// Starting to chase frames
mframe_t supervisor_frames_start_chase [] =
{
	ai_charge, 0,	NULL,					// 1
	ai_charge, (SUPERVISOR_WALK/2), NULL	// 2
};
mmove_t supervisor_move_start_chase =
{
	FRAME_walk1,
	FRAME_walk2,
	supervisor_frames_start_chase,
	supervisor_chase_decide
};

void supervisor_chase (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_chase;
}

void supervisor_stop_chase (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_stop_chase;
}

void supervisor_start_chase (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_start_chase;
}


//
// FACING FRAMES (sort of a hack, only the chase logic
// should use these frames, general AI should use the
// chase frames)
//

// Stop facing frames
mframe_t supervisor_frames_stop_face [] =
{	
	ai_charge, 0, NULL,	// 10
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL	// 15
};
mmove_t supervisor_move_stop_face =
{
	FRAME_walk10,
	FRAME_walk15,
	supervisor_frames_stop_face,
	supervisor_decide
};

// Facing frames
mframe_t supervisor_frames_face [] =
{
	ai_charge, 0, NULL,	// 3
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, supervisor_chase_decide,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL	// 9
};
mmove_t supervisor_move_face =
{
	FRAME_walk3,
	FRAME_walk9,
	supervisor_frames_face,
	supervisor_chase_decide
};

void supervisor_face (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_face;
}

void supervisor_stop_face (edict_t *self)
{
	self->monsterinfo.currentmove = &supervisor_move_stop_face;
}

// If the entity we're chasing is dead or dying, stop chasing it.
// If the entity we're chasing is in range, stop chasing it.
// Otherwise, continue as before (ie. do nothing).
void supervisor_chase_decide (edict_t *self)
{
	enum eRechargePath path_status = supervisor_recharge_ok(self,self->enemy);

	// RSP 022499: Added the check for 'inuse' because a dead gbot's entity has been freed, and
	// its deadflag zeroed, which is the value for 'still alive' (DEAD_NO)
	if ((self->enemy->deadflag != DEAD_NO) || (path_status == PATH_OK) || !(self->enemy->inuse))
	{
		// By not chasing anymore, supervisor_decide will be called.
		supervisor_stop_chase(self);
		return;
	}

	if (path_status == PATH_TOO_FAR)
	{
		// We need to get closer to the target
		// Use the "chase" frames
		self->monsterinfo.currentmove = &supervisor_move_chase;
	}
	else // (path_status == PATH_BAD_FOV)
	{
		// We're close enough, we're just facing the wrong way
		// Use the "face" frames
		self->monsterinfo.currentmove = &supervisor_move_face;
	}
}


//
// PAIN CODE
//

// Pain1 frames
// MRB 080898 - changed to match new header file - added second pain set.
mframe_t supervisor_frames_paina[] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL	// 5
};
mmove_t supervisor_move_paina =
{
	FRAME_paina1,
	FRAME_paina5,
	supervisor_frames_paina,
	supervisor_start_run
};

mframe_t supervisor_frames_painb[] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL	// 5
};
mmove_t supervisor_move_painb =
{
	FRAME_painb1,
	FRAME_painb5,
	supervisor_frames_painb,
	supervisor_start_run
};

// Take pain.
void supervisor_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// MRB 031999 - oops, need a pain skin ...
	//if (self->health < (self->max_health / 2))
	//	self->s.skinnum = 1;

	waypoint_check(self);	// RSP 071800 - retreat along waypoint path?

	// Order nearby bots to defend me!
	// RSP 022499: Only if attacker (other) was a client
	if (other->client)
		supervisor_order_to_attack(self, other, 0);

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (random() < 0.5)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &supervisor_move_paina;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &supervisor_move_painb;
	}
}


//
// RECHARGE CODE
//

// Check if there is a bot nearby that is in need of recharging. If there is,
// return a pointer to it, if not return NULL.
// 19980810 AJA - Renamed from supervisor_can_recharge (it was confusing me).
edict_t *supervisor_recharge_target (edict_t *self)
{
	edict_t	*bot = NULL;
	trace_t	tr;

	while (bot = findradius(bot, self->s.origin, self->radius))
	{
		// 19980810 AJA - Supervisor can only recharge enemies whose max_health != 0
		if ( (bot->svflags & SVF_MONSTER)		&&	// must be a monster
			 (bot->deadflag == DEAD_NO)			&&	// must be alive
			  bot->inuse						&&	// RSP 022499 must be in use
//			 !bot->client						&&	// must not be the player
			  bot != self						&&	// must not be yourself, the supervisor
			 (bot->health < bot->max_health)	&&	// health must be less than max
			 (bot->identity != ID_TURRET)		&&	// RSP 051899 must not be a turret
			!(bot->flags & FL_FROZEN)			&&	// RSP 090100 must not be tportgun or matrix frozen
			 (bot->freeze_framenum <= level.framenum) &&	// RSP 090300 must not be frozen by freezeball
			 (bot->identity != ID_JELLY))			// RSP 022799 must not be a jelly
		{
			// Check for obstructions between self and target bot.
			tr = gi.trace(self->s.origin, NULL, NULL, bot->s.origin, self, MASK_SHOT);
			if (tr.ent == bot)
			{
				self->spawnflags &= ~SPAWNFLAG_PACIFIST;	// RSP 061600 - someone's hurt
				self->monsterinfo.aiflags &= ~AI_PACIFIST;	// RSP 061600 - someone's hurt
				return bot;
			}
		}
	}

	return NULL;
}

// MRB 022899 - storage variables to facilitate the path drawing
vec3_t recharge_path_dir = {0,0,0};
float recharge_path_dist = 0.0;

// Check if the power cable can reach from "start" to "end", and that
// the endpoint is below and in front of the supervisor.
enum eRechargePath supervisor_recharge_ok (edict_t *start, edict_t *end)
{
	vec3_t	dir, angles;
	float	yaw;

	// check for max distance
	VectorSubtract(start->s.origin, end->s.origin, recharge_path_dir);

	// MRB 022899 - store distance and normal vector for later calulations
	if ((recharge_path_dist = VectorNormalize(recharge_path_dir)) > 256)
		return PATH_TOO_FAR;

	// check for min/max yaw
	vectoangles(recharge_path_dir, angles);	// MRB 022899
	yaw = start->s.angles[YAW] - angles[YAW] - 180;
	if (yaw < -180)
		yaw += 360;
	if (fabs(yaw) > 30)
		return PATH_BAD_FOV;

	// 19980810 AJA - Take into account min/max pitch angle.
	// Cannot repair anything above us.
	if ((angles[PITCH] > 0) || (angles[PITCH] < -180))
	{
		// RSP 022499:  Move supervisor vertically so he can try to get above target bot
		//				so he can feed him some health. If PATH_BAD_FOV was returned because
		//				the angle was bad but the height was ok, that's ok. Just let the
		//				supervisor rise and turn in tandem. The rise won't hurt.

		vec3_t	goal;
		trace_t	tr;

		VectorCopy(start->s.origin,goal);

		// Either move up to just above the target bot, or 10 units, whichever is minimum.
		// If 10 wasn't enough, then we'll apply that, and try again on the next frame.

		goal[2] = min(end->s.origin[2] + 1,goal[2] + 10);

		// See if there's room to move.

		tr = gi.trace(start->s.origin, start->mins, start->maxs, goal, start, MASK_SHOT);
		if (tr.fraction == 1)
		{
			VectorCopy(goal,start->s.origin);	// Go there
			gi.linkentity(start);				// Tell the world
			G_TouchTriggers(start);
			VectorSubtract(start->s.origin, end->s.origin, dir);

			// check for min/max yaw again
			vectoangles(dir, angles);
			if ((angles[PITCH] > 0) || (angles[PITCH] < -180))
				return PATH_BAD_FOV;	// target is still above us, even after we moved
		}	// End of vertical change code
	}

	return PATH_OK;
}

// Reel the power cable in, play a sound and make the bot that was being recharged
// stop glowing.
void supervisor_reel_in (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_reelin, 1, ATTN_NORM, 0);

	// Make the "enemy" stop glowing.
	// AJA 19980908 - Clear the AI_RECHARGING flag.
	if (self->enemy)	// RSP 090100
		self->enemy->monsterinfo.aiflags &= ~AI_RECHARGING;
}

// MRB 022899 - static array to define beam's start point
vec3_t beam_start_Trk[] = {
	3.999140,	0.000001,	2.667215, 
	4.096266,	-0.010997,	2.771953,
	4.221150,	-0.021133,	2.925648,
	4.294650,	0.002879,	3.020197,
	4.299379,	0.114185,	3.032828,
	4.281759,	0.248787,	3.031556,
	4.271103,	0.270391,	3.029671,
	4.282495,	0.055464,	3.033909,
	4.306887,	-0.274289,	3.036538,
	4.324049,	-0.469028,	3.034364,
	4.324313,	-0.429714,	3.032028, 
	4.318301,	-0.284781,	3.029784,
	4.307201,	-0.113465,	3.023351,
	4.290071,	0.005459,	3.008661,
	4.242912,	0.037133,	2.948490, 
	4.161035,	0.025881,	2.843900,
	4.076160,	0.012139,	2.746412,
 0.0f };

// Recharge our current enemy.
void supervisor_recharge (edict_t *self)
{
	vec3_t	start, f, r, end;
	trace_t	tr;
	int		i,heal;
	edict_t	*target;

	target = self->enemy;
	if (!target)	// RSP 090100
		return;

	// AJA 19980929 - Check if our recharge target got killed.
	// RSP 022499: Also have to make sure he's still 'inuse', since a freed entity looks like
	// he's still alive because his deadflag has been set to 0, which is DEAD_NO
	// RSP 090100: Also have to see if he's been targetted by a matrix ball or grabbed by a
	// teleport gun. In all of these cases, he is FLFROZEN, so that's what we check for.
	// Also, don't recharge a monster that's been hit by a freezeball.
	if ((target->deadflag & DEAD_DYING) ||
		(target->deadflag & DEAD_DEAD)  ||
		!target->inuse					||
		(target->flags & FL_FROZEN)		||				// matrix & teleportgun
		(target->freeze_framenum > level.framenum))		// freezegun
	{
		// Turn off the glow effect.
		target->monsterinfo.aiflags &= ~AI_RECHARGING;
		return;
	}

	// Calculate a vector to use as the power cable's start position.
	AngleVectors(self->s.angles, f, r, NULL);

	// MRB 022899 - offset based on value in static array
	G_ProjectSource(self->s.origin, beam_start_Trk[self->s.frame - FRAME_recharge1], f, r, start);

	// Get the end position, and check if the cable can reach.
	VectorCopy(target->s.origin, end);
	if (supervisor_recharge_ok(self, target) != PATH_OK)
	{
		target->monsterinfo.aiflags &= ~AI_RECHARGING;
		return;
	}

	// Check for obstructions in the cable's path.
	tr = gi.trace(start, NULL, NULL, target->s.origin, self, MASK_SHOT);
	if (tr.ent != target)
		return;

	/* MRB 022899 - fiddle with ends so beam is predictable (the model can't be cut)
	 *  NOTE that supervisor_recharge_ok() was called with the vectors essentially reversed,
	 *  thus the negative distance below */
	i = (int)(recharge_path_dist / 32);
	VectorMA(start,-8, recharge_path_dir, start);
	VectorMA(start, -(i * 32), recharge_path_dir, end);

	// Calculate health bump, and play sounds according to which frame we're on.
	if (self->s.frame == FRAME_recharge3)
	{
		// The end of the cable just made contact.
		heal = 0;
		gi.sound(target, CHAN_AUTO, sound_impact, 1, ATTN_NORM, 0);

		// Make the "enemy" glow. Hmm, doesn't work.
		// AJA 19980908 - If you set s.effects here it will be overridden
		// by M_SetEffects maybe this frame but definately the next. Instead
		// we use an AI flag to set the effects inside of M_SetEffects.
		target->monsterinfo.aiflags |= AI_RECHARGING;
	}
	else
	{
		// Pump energy through the cable since it's already attached.
		if (self->s.frame == FRAME_recharge4)
			gi.sound (self, CHAN_WEAPON, sound_suck, 1, ATTN_NORM, 0);
		heal = 3;	// will be 30 points total per recharge anim.
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_PARASITE_ATTACK);
	gi.WriteShort (self - g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	// Heal the target for "heal" hit points.
	if ((heal = (target->health + heal)) > target->max_health)
		target->health = target->max_health;
	else
		target->health = heal;
}

// Recharge frames.
mframe_t supervisor_frames_recharge [] =
{
	ai_charge, 0,	supervisor_launch,			// 1
	ai_charge, 0,	NULL,
	ai_charge, 5,	supervisor_recharge,		// Target hits
	ai_charge, 0,	supervisor_recharge,		// recharge
	ai_charge, 2,	supervisor_recharge,		// recharge
	ai_charge, 2,	supervisor_recharge,		// recharge
	ai_charge, 0,	supervisor_recharge,		// recharge
	ai_charge, -2,  supervisor_recharge,		// recharge
	ai_charge, -2,	supervisor_recharge,		// recharge
	ai_charge, 0,	supervisor_recharge,		// recharge

	ai_charge, 2,	supervisor_recharge,		// recharge
	ai_charge, 3,	supervisor_recharge,		// recharge
	ai_charge, 2,   supervisor_recharge,		// recharge
	ai_charge, 0,	supervisor_reel_in,			// let go
	ai_charge, -2,	NULL,
	ai_charge, -2,	NULL,
	ai_charge, -3,	NULL						// 17
};
mmove_t supervisor_move_recharge = {
	FRAME_recharge1,
	FRAME_recharge17,
	supervisor_frames_recharge,
	supervisor_start_run
};


//
// SIGHT / ATTACK CODE
//

void supervisor_order_to_attack (edict_t *self, edict_t *other, int sight_flag)
{
	edict_t *bot = NULL;

	// Tell all worker bots within a 'n' unit radius to attack this enemy
	while (bot = findradius(bot, self->s.origin, self->radius))
	{
		// 19980810 AJA - Supervisor can order any living monster to attack
		// 19980923 AJA - Except other supervisor bots
		if ((bot->svflags & SVF_MONSTER) && 
			(bot->deadflag == DEAD_NO)   &&
			!(bot->monsterinfo.aiflags & AI_GOOD_GUY) && // RSP 022499: Good guys can't attack
			(!bot->client) && (bot != self) && (bot->identity != ID_SUPERVISOR))	// RSP 062199
		{
			// Order the bot to attack the newcomer
			bot->enemy = other;

			// Make the bot play its "sight" sound if sight_flag is set
			if (sight_flag && bot->monsterinfo.sight)
				bot->monsterinfo.sight(bot, self);

			// Run towards player & attack
			if (bot->monsterinfo.run)
				bot->monsterinfo.run(bot);
		}
	}
}


// Supervisor fires a blaster shot at the player.
void supervisor_fire_blaster (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	vec3_t	zero = {8.0, 0.0, 8.0};
	int		flash_number = 0;

	if (!self->enemy)	// RSP 083100 - encountered this when testing teleportgun
		return;

	if (self->enemy->health <= 0)
	{
		if (self->enemy->client)	// RSP 082300
			Forget_Player(self);
		return;
	}

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, zero, forward, right, start);

	Pick_Client_Target(self,end);			// RSP 052699 - generalized routine
	VectorSubtract(end, start, dir);

	monster_fire_blaster(self, start, dir, 5, 600, flash_number, EF_BLASTER);
}

// Supervisor fires some more ...
void supervisor_fire_repeat ( edict_t *self)
{
	if ((--self->count) > 0)
		self->s.frame = FRAME_attack2;
}

// Attack frames
// MRB 080898 changed to reflect new .h file
mframe_t supervisor_frames_attack [] =
{
	ai_charge, 0, NULL,	// 1
	ai_charge, 0, NULL,
	ai_charge, 0, supervisor_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, supervisor_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, supervisor_fire_repeat,
	ai_charge, 0, NULL	// 8
};

mmove_t	supervisor_move_attack =
{
	FRAME_attack1,
	FRAME_attack8,
	supervisor_frames_attack,
	supervisor_start_run
};

// Supervisor sight function, triggered when the supervisor spots the player.
// Here, we want to alert all bots within range and go into "repair" mode.
void supervisor_sight (edict_t *self, edict_t *other)
{
	// 19980810 AJA - Save a pointer to the player (the REAL enemy)
	// in "oldenemy" so we don't forget about him.
	self->oldenemy = other;

	// Play the "sight" sound.
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);	// RSP 063000

	// Order nearby worker bots to attack.
	supervisor_order_to_attack(self, other, 1);

	// Figure out what to do next (recharge, chase, attack)
	// 19980810 - Don't hardcode a choice here, call the decide function.
	// RSP 031799: Figured out with MRB that the following line caused an endless loop
	// on the 2b map when the supervisor spotted you. supervisor_decide() gets called later
	// so a supervisor still has a chance to recharge bots or attack you.
//	supervisor_decide(self);
}

// Decide what to do:
//  - Recharge a nearby bot if possible, else
//  - Attack the player if he's around, else
//  - Veg out for a while until the player shows up.
void supervisor_decide (edict_t *self)
{
	edict_t *bot;

	self->enemy = NULL; // RSP 030599 it's getting reset anyway
	if (bot = supervisor_recharge_target(self))
	{
		// 19980810 - If there is a bot in need of assistance but it
		// isn't in our current FOV, chase it.
		self->enemy = bot;
		if (supervisor_recharge_ok(self, bot) == PATH_OK)
			self->monsterinfo.currentmove = &supervisor_move_recharge;
		else
			supervisor_start_chase(self);
	}
	else if (FindTarget(self))
	{
		self->oldenemy = self->enemy;	// RSP 030599 remember player later
		supervisor_attack(self);
	}
	else if (self->oldenemy)
	{	// Decided to attack my old enemy
		self->enemy = self->oldenemy;
		supervisor_attack(self);
	}
	else
	{	// Decided to stand around and vegetate
		self->monsterinfo.currentmove = &supervisor_move_stand;
		self->enemy = NULL;	// RSP 022499: insurance
	}
	self->goalentity = self->enemy; // RSP 022499
}


// We have been told to attack, but if the enemy is not in sight we must
// run around until he is, no point in shooting from here.
void supervisor_attack (edict_t *self)
{
	trace_t	tr;

	if (!self->enemy)
	{
		// We have no enemy, run around and decide what to do later,
		// maybe then we'll be able to recharge someone or we will have
		// spotted the enemy.
		self->monsterinfo.currentmove = &supervisor_move_start_run;
		return;
	}

	tr = gi.trace(self->s.origin, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT);
	if (tr.ent != self->enemy)
	{
		// We can't see the enemy from here, our view is obstructed, so
		// approach the enemy instead of shooting from here.
		self->monsterinfo.currentmove = &supervisor_move_start_run;
		return;
	}

	// We have an enemy and our view is clear, kick its ass.
	// MRB 080898 - Repetative shots to mimic previous look - should probably be a decision instead
	self->count = 3;
	self->monsterinfo.currentmove = &supervisor_move_attack;
}


//
// DEATH CODE
//

// MRB 080898 changed to reflect new .h
// MRB 031399 - changed again for new death frames

void supervisor_dead (edict_t *self)
{
	self->nextthink = 0;

	// MRB 031999 - mark him as really 'dead'
	self->deadflag = DEAD_DEAD;
	gi.linkentity(self);
}

mframe_t supervisor_frames_death [] =
{
	ai_move, 0,	 NULL,	// 8
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL	// 12
};

mmove_t supervisor_move_death =
{
	FRAME_death8,
	FRAME_death12,
	supervisor_frames_death,
	supervisor_dead
};

void supervisor_dropping (edict_t *self)
{
	if (self->groundentity)
		self->monsterinfo.currentmove = &supervisor_move_death;
}

mframe_t supervisor_frames_death_drop [] =
{
	ai_move, 0,	 supervisor_dropping,	// 1
	ai_move, 0,	 supervisor_dropping,
	ai_move, 0,	 supervisor_dropping,
	ai_move, 0,	 supervisor_dropping,
	ai_move, 0,	 supervisor_dropping,
	ai_move, 0,	 supervisor_dropping,
	ai_move, 0,	 supervisor_dropping	// 7
};
mmove_t supervisor_move_death_drop =
{
	FRAME_death1,
	FRAME_death7,
	supervisor_frames_death_drop,
	NULL
};

void supervisor_dead_drop (edict_t *self)
{
	VectorSet(self->mins, -24, -24, -16);	// MRB 031699
	VectorSet(self->maxs, 24, 24, 0);		// MRB 031699
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	// 19980810 AJA - Mark this monster as 'in its death throes'.
	self->deadflag = DEAD_DYING;

	self->monsterinfo.currentmove = &supervisor_move_death_drop;

	gi.linkentity(self);
}

void supervisor_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		// JBM 082398 - Changed the gibs to throw gears instead of body parts, and made it GIB_METALLIC
		// instead of GIB_ORGANIC... An oversight I'm sure :-)
		gi.sound(self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0 ; n < 2 ; n++)
			ThrowGib(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		for (n = 0 ; n < 4 ; n++)
			ThrowGib(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	// MRB 031999 - if simply 'dying', don't run these again ...
	if ((self->deadflag == DEAD_DEAD) || (self->deadflag == DEAD_DYING))
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

	// MRB 031399 - no 'death_start' frames - just start the dropping ones
	supervisor_dead_drop(self);
}


//
// SPAWN CODE
//

void monster_supervisor_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	sound_pain1  = gi.soundindex("superv/pain1.wav");	
	sound_pain2  = gi.soundindex("superv/pain2.wav");	
	sound_die    = gi.soundindex("superv/death1.wav");	
	sound_launch = gi.soundindex("superv/launch.wav");
	sound_impact = gi.soundindex("superv/impact.wav");
	sound_suck   = gi.soundindex("superv/suck.wav");
	sound_reelin = gi.soundindex("superv/reelin.wav");
	sound_sight  = gi.soundindex("superv/sight.wav");
	sound_idle   = gi.soundindex("superv/idle.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/bots/SuperV/tris.md2");	// RSP 061899
}


/*QUAKED monster_supervisor (1 .5 0) (-24 -24 -8) (24 24 8) Ambush Trigger_Spawn Sight Pacifist
"radius"	Radius for Bot detection
*/
void SP_monster_supervisor (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	monster_supervisor_indexing (self);	// RSP 092100 - set the sound and model indices

	VectorSet(self->mins, -24, -24, -8);
	VectorSet(self->maxs, 24, 24, 8);
	self->movetype = MOVETYPE_FLY;
	self->solid = SOLID_BBOX;

	self->health = hp_table[HP_SUPERVISOR];		// RSP 032399
	self->gib_health = -50;
	self->max_health = hp_table[HP_SUPERVISOR];	// RSP 032399
	self->mass = 150;							// RSP 053099

	self->pain = supervisor_pain;
	self->die = supervisor_die;
	self->pain_debounce_time = 0;	// RSP 062200 - for rotation when standing

	self->monsterinfo.stand  = supervisor_stand;
	self->monsterinfo.walk   = supervisor_start_walk;
	self->monsterinfo.run    = supervisor_start_run;
	self->monsterinfo.attack = supervisor_decide;
	self->monsterinfo.sight  = supervisor_sight;
	self->monsterinfo.idle   = supervisor_idle;

	self->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger
	self->identity = ID_SUPERVISOR;	// RSP 062199 - new identity flag

	if (!self->radius)
		self->radius = SUPERVISOR_RADIUS;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = 50;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &supervisor_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start(self);
}
