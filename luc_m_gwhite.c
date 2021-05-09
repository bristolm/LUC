/*
==============================================================================

GREAT WHITE SHARK

==============================================================================
*/

//MRB 19980222 - I restructured most movements to go with the new frames
//    but did not comment those changes - I did comment all logic
//    changes I made
#include "g_local.h"
#include "luc_m_gwhite.h"
//#include "luc_m_gwhite_ext.h"	// RSP 041899 - removed extensions

static int	sound_chomp;
static int	sound_attack;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_death;
static int	sound_sight;
static int  sound_idle;

extern qboolean enemy_vis;	// RSP 032699
extern int	enemy_range;	// RSP 041099 - to stop far shark lunges

//
// RUN / CHARGE
//

#define GWHITE_WALK_SPEED	10
#define GWHITE_RUN_SPEED	24

#define GWHITE_BITE_RAD		70
#define GWHITE_TEAR_RAD		90
#define GWHITE_TEAR_OFFSET	30

mframe_t gwhite_frames_run_loop [] =
{
	ai_run, GWHITE_RUN_SPEED, NULL,	// 2
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,	// 6

	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL,
	ai_run, GWHITE_RUN_SPEED, NULL // 12

};

void gwhite_run (edict_t *self);
void gwhite_walk (edict_t *self);
void gwhite_move_run_loop (edict_t *self);

void gwhite_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


//mmove_t gwhite_run_loop = {FRAME_gwchrg2, FRAME_gwchrg12, gwhite_frames_run_loop, gwhite_run};
mmove_t gwhite_run_loop = {FRAME_gwchrg2, FRAME_gwchrg12, gwhite_frames_run_loop, gwhite_move_run_loop};

void gwhite_move_run_loop (edict_t *self)
{
	self->monsterinfo.currentmove = &gwhite_run_loop;
}

// --- run start ---

mframe_t gwhite_frames_run_start [] =
{
	ai_stand, GWHITE_WALK_SPEED, NULL
};

mmove_t gwhite_move_run_start = {FRAME_gwchrg1, FRAME_gwchrg1, gwhite_frames_run_start, gwhite_move_run_loop};

// RSP 032699 - run or walk, depending on whether there's an enemy
void gwhite_decide (edict_t *self)
{
	if (self->enemy)
		gwhite_run(self);
	else
		gwhite_walk(self);
}

void gwhite_run (edict_t *self)
{
	self->monsterinfo.currentmove = &gwhite_move_run_start;
}

//
// WALK - Standard Swimming
//


void gwhite_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

mframe_t gwhite_frames_move_walk [] =
{
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 1
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 10
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 20
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL	// 24
};

mmove_t gwhite_move_walk = {FRAME_gwhor1, FRAME_gwhor24, gwhite_frames_move_walk, NULL};

void gwhite_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &gwhite_move_walk;
}

//
// STAND
//

void gwhite_stand (edict_t *self)
{
	//MRB 19980222 -  Never stop moving
	self->monsterinfo.currentmove = &gwhite_move_walk;
}

//
//  ATTACK
//


//--hurt target

void gwhite_ungrab (edict_t *self)
{
	// check for a locked_to target to release
	if (self->enemy && self->enemy->locked_to == self)
	{
		self->enemy->locked_to = NULL;
		if (self->s.frame != FRAME_gwtear15)
			self->monsterinfo.nextframe = FRAME_gwtear15;
	}
}

void gwhite_bite (edict_t *self)
{
	vec3_t aim;

	VectorSet (aim, (self->enemy->locked_to == self ? GWHITE_TEAR_RAD : GWHITE_BITE_RAD) , 0, 0);
	if (!(fire_hit (self, aim, 3, 0)))	// let go if too far away
		gwhite_ungrab (self);
}

// GRAB for tear attack - the trick here is that the model shifts, not the shark.
void gwhite_grab (edict_t *self)
{
	trace_t t;
	vec3_t enemy_end, end_pos, from_target;
	static float shark_rad = (1.414 * 70);  // RSP 012099: new max value for radius
	float flt = 0;
	float enemy_shift = 0;

	// Largest angle of swing is ~40 degrees.
	// The separation needs to be done carefully to avoid bumping into each other.
	// Shark model extends origin + 50 units during the tearing.
	// SAFEST horizonal distance is:  shark->maxs[0] / COS(45) + enemy->maxs[0] / COS(45)
	VectorSubtract(self->s.origin,self->enemy->s.origin,from_target);
	from_target[2] = 0;
	VectorNormalize(from_target);

	flt = (shark_rad + (1.414 * self->enemy->maxs[0])) *  1.2;
	VectorMA(self->enemy->s.origin,-flt,from_target,end_pos);

	// If we're not too low, pick up target a little
	// This gives us some vertical breathing room for moving the target
	enemy_shift = self->s.origin[2] - 15 - self->enemy->s.origin[2] - self->enemy->mins[2];
	if (enemy_shift < 0)
		enemy_shift = 0;
	else if (enemy_shift > 25)
		enemy_shift = 25;
	
	VectorCopy(self->enemy->s.origin,enemy_end);
	enemy_end[2] += enemy_shift;
	t = gi.trace (self->enemy->s.origin, self->mins, self->maxs, enemy_end, self->enemy, MASK_MONSTERSOLID|MASK_PLAYERSOLID); // RSP 012099 added playersolid mask

	if (t.fraction == 1)
		VectorCopy(enemy_end,self->enemy->s.origin);
	// If that didn't work, oh well ...

	// Figure where I should be to tear adequately
	// Make sure point is kind of relative to the enemy's viewheight
	//   (+15 because the shark's origin is waaay above its center)
	flt = self->enemy->s.origin[2] + (self->enemy->viewheight * 1.25) + 15;
	end_pos[2] = ((self->s.origin[2] > flt) ? flt : self->s.origin[2]);

	// lunge at this kludged point - if we don't make it ... well, OK.
	t = gi.trace (self->s.origin, self->mins, self->maxs, end_pos, self, MASK_MONSTERSOLID);
	if (t.ent)
	{
		// Move to new spot.
		VectorCopy(t.endpos,self->s.origin);
	}

	// lock target
	self->enemy->locked_to = self;

	// Shark's head is now ahead of where it was - use spot further out 
	// as the center for all rotations.  Store new 'lock' difference
	VectorInverse(from_target);
	VectorMA(self->s.origin,GWHITE_TEAR_OFFSET,from_target,end_pos);
	VectorSubtract(self->enemy->s.origin,end_pos,self->pos2);

	// do some bite damage
	gwhite_bite(self);
}

void gwhite_nibble (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, GWHITE_BITE_RAD, 0, 0);
	fire_hit (self, aim, 5, 0);
}

void gwhite_preattack (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_chomp, 1, ATTN_NORM, 0);	// RSP 063000
}

vec3_t drag_enemy [] =
{	// Map to frames in qwhite_frames_tear()
	// YAW, PITCH, ROLL
	0,0,0,
	0,0,0,		// --- START -- frame #114 in LightWave scene 
	-25,0,0,	// left
	-35,0,0,	// left
	-23,0,0,	// right
	4,0,-2,		// right
	29,6,-5,	// right
	26,8,30,	// up
	26,20,30,	// up
	16,0,10,	// down
	12,-20,-18,	// down
	10,-34,-37,	// down
	5,15,-34,	// up
	0,10,-20,	// up
	0,0,0,		// --- END ---- frame #127 in Lightwave scene
};

void gwhite_drag(edict_t *self)
{
	vec3_t	knock, after_drag, enemy_end, aim, forward, right;
	int	index;
	trace_t t;
	VectorSet (aim, GWHITE_TEAR_RAD, 0, 0);

	// do a little damage ...
	if (!(fire_hit (self, aim, 3, 0)))
	{
		vec3_t tmp;

		VectorSubtract (self->enemy->s.origin, self->s.origin, tmp);
		gwhite_ungrab(self);
	}

	// If he's dead, back off.
	if (self->enemy->deadflag == DEAD_DEAD)
		gwhite_ungrab(self);

	// Determine new spot for enemy's origin.   Assumes only 'self' is moving it.
	// --Add offset to rotation center--
	aim[0] = GWHITE_TEAR_OFFSET;
	AngleVectors(self->s.angles,forward,right,NULL);
	G_ProjectSource(self->s.origin,aim,forward,right,enemy_end);

	// --Add new rotation portion--
	index = self->s.frame - FRAME_gwtear1;
	VectorAdd(self->s.angles,drag_enemy[index],after_drag);
	AngleVectors(after_drag,forward,right,NULL);
	G_ProjectSource(enemy_end,self->pos2,forward,right,enemy_end);

	// Move target - if move is BLOCKED, stop the attack.
	if (self->enemy->svflags & SVF_MONSTER)
		t = gi.trace(self->enemy->s.origin,self->enemy->mins, self->enemy->maxs,enemy_end,self->enemy,MASK_MONSTERSOLID);
	else
		t = gi.trace(self->enemy->s.origin,self->enemy->mins, self->enemy->maxs,enemy_end,self->enemy,MASK_PLAYERSOLID);

	if (t.fraction < 1)
	{	// If we hit something, bump it, and leave the player at that point
		if (t.ent == self)
		{	// Hit myself with enemy.  Just push the player away.
			VectorSubtract(t.endpos,self->s.origin,aim);
			VectorScale (forward, (1500.0/(self->enemy->mass < 50 ? 50 : self->enemy->mass)), knock);
			VectorAdd (self->enemy->velocity, knock, self->enemy->velocity);
		}
		else if ((t.ent->svflags & SVF_MONSTER) || (t.ent->client))
		{	// Thrown into another creature
			VectorSubtract(t.endpos,self->enemy->s.origin,aim);
			T_Damage (t.ent, self->enemy, self, aim, t.endpos,
						aim, (int)(self->enemy->mass * 0.01) + 1, 1, 0, MOD_UNKNOWN);
		}
		else
		{	// Thrown into a wall
			VectorSubtract(t.endpos,self->enemy->s.origin,aim);
			T_Damage (self->enemy, self->enemy, self, aim, t.endpos,
						t.plane.normal, (int)(self->enemy->mass * 0.01), 1, 0, MOD_UNKNOWN);
		}

//		VectorCopy(t.endpos,self->enemy->s.origin); // RSP 012099: Don't embed in wall
		gwhite_ungrab(self);
		return;
	}

	// Finish move
	VectorCopy(enemy_end,self->enemy->s.origin);
	
	// Rotate target's angles
	if (self->enemy->client)
	{
		self->enemy->client->kick_angles[0] += drag_enemy[index][0];
		self->enemy->client->kick_angles[1] -= drag_enemy[index][1];
		self->enemy->client->kick_angles[2] -= drag_enemy[index][2]*0.5;
	}

	self->enemy->s.angles[0] += drag_enemy[index][0];
	self->enemy->s.angles[1] -= drag_enemy[index][1];
	self->enemy->s.angles[2] -= drag_enemy[index][2];
}
//-- tear attack

mframe_t gwhite_frames_tear [] =
{
	ai_turn, (GWHITE_RUN_SPEED * 1.5),	gwhite_preattack,	// lunge	// 1
	ai_turn, GWHITE_RUN_SPEED,	gwhite_grab,				// lunge
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,	// 10
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	NULL, 0,	gwhite_drag,
	ai_turn, 0,	gwhite_ungrab	// 15
};

mmove_t gwhite_move_tear = {FRAME_gwtear1, FRAME_gwtear15, gwhite_frames_tear, gwhite_run};

//-- bite attack
mframe_t gwhite_frames_attack [] =
{
	ai_turn, 18,	gwhite_preattack,	// 1
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL,
	ai_turn, 18,	gwhite_bite,
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL,
	ai_turn, 18,	gwhite_bite,
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL,				// 10
	ai_turn, 18,	NULL,
	ai_turn, 18,	gwhite_bite,
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL,
	ai_turn, 18,	NULL				// 15
};

mmove_t gwhite_move_attack = {FRAME_gwbite1, FRAME_gwbite15, gwhite_frames_attack, gwhite_run};

void gwhite_melee(edict_t *self)
{
	float f;
	vec3_t a,v;		// RSP 032699
	float adiff;	// RSP 032699

	// RSP 032699 - Check for jelly
	if (self->enemy->identity == ID_JELLY)
	{
		// Check for z heights so shark doesn't eat a jelly that's way above or below him
		// Check to be sure jelly is in front of shark.
		if ((self->absmax >= self->enemy->absmin) || (self->absmin <= self->enemy->absmax))
		{
			VectorSubtract(self->enemy->s.origin,self->s.origin,v);
			vectoangles(v,a);
			adiff = anglemod(anglemod(self->s.angles[YAW]) - anglemod(a[YAW]));
			if ((adiff < 45) || (adiff > 315))
				jelly_touch(self->enemy,self,NULL,NULL);
		}
		return;
	}

	f = (self->s.origin[2] - 15) - self->enemy->s.origin[2];
								
	if ( (!(self->enemy->locked_to)) && (random() < 0.7) &&
		(f > (self->enemy->mins[2] * 0.8)) && (f < (self->enemy->maxs[2] * 1.25)) )
		self->monsterinfo.currentmove = &gwhite_move_tear;
	else
		self->monsterinfo.currentmove = &gwhite_move_attack;
}

/*
//
//  DODGE
//

void gwhite_dodge_low (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	gi.linkentity (self);
}

void gwhite_dodge_high (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t gwhite_frames_dodge [] =
{
	ai_run, 4,  NULL,	// 1
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  gwhite_dodge_low,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,	// 10

	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,	// 20

	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  gwhite_dodge_high,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,
	ai_run, 4,  NULL,	// 30

	ai_run, 4,  NULL,	// 31

};
mmove_t	gwhite_move_dodge = {FRAME_gwver1, FRAME_gwver31, gwhite_frames_dodge, gwhite_run};

void gwhite_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (random() < 0.20)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &gwhite_move_dodge;
}
 */

//
// PIVOT
//

mframe_t gwhite_frames_move_pivstart []=
{
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 1
	ai_walk, GWHITE_WALK_SPEED, NULL	// 2
};

mframe_t gwhite_frames_move_pivot [] =
{
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 3
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,

	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL,
	ai_walk, GWHITE_WALK_SPEED, NULL	// 8
};

mframe_t gwhite_frames_move_pivend []=
{
	ai_walk, GWHITE_WALK_SPEED, NULL,	// 9
	ai_walk, GWHITE_WALK_SPEED, NULL	// 10
};


mmove_t gwhite_move_tleftstart = {FRAME_gwturnl1, FRAME_gwturnl2 , gwhite_frames_move_pivstart, NULL};
mmove_t gwhite_move_tleft =      {FRAME_gwturnl3, FRAME_gwturnl8 , gwhite_frames_move_pivot, NULL};
mmove_t gwhite_move_tleftend =   {FRAME_gwturnl9, FRAME_gwturnl10, gwhite_frames_move_pivend, gwhite_decide};	// RSP 032699
//mmove_t gwhite_move_tleftend =   {FRAME_gwturnl9, FRAME_gwturnl10, gwhite_frames_move_pivend, gwhite_run};

mmove_t gwhite_move_trightstart = {FRAME_gwturnr1, FRAME_gwturnr2 , gwhite_frames_move_pivstart, NULL};
mmove_t gwhite_move_tright =      {FRAME_gwturnr3, FRAME_gwturnr8 , gwhite_frames_move_pivot, NULL};
mmove_t gwhite_move_trightend =   {FRAME_gwturnr9, FRAME_gwturnr10, gwhite_frames_move_pivend, gwhite_decide};	// RSP 032699
//mmove_t gwhite_move_trightend =   {FRAME_gwturnr9, FRAME_gwturnr10, gwhite_frames_move_pivend, gwhite_run};

// RSP 012099: Straighten out frames if ending a turn

void gwhite_straightenout(edict_t *self)
{
	if ((self->monsterinfo.currentmove == &gwhite_move_tleftstart) ||
		(self->monsterinfo.currentmove == &gwhite_move_tleft))
	{
		self->monsterinfo.currentmove = &gwhite_move_tleftend;
	}
	else if ((self->monsterinfo.currentmove == &gwhite_move_trightstart) ||
			 (self->monsterinfo.currentmove == &gwhite_move_tright))
	{
		self->monsterinfo.currentmove = &gwhite_move_trightend;
	}
	self->turn_direction = TD_STRAIGHT;	// RSP 032699
}

void gwhite_pivot (edict_t *self, float yaw)
{
	qboolean	Turningleft;
	int			index;

	if (yaw < -self->yaw_speed)	    // turning right
	{
		Turningleft = false;
		index = self->s.frame - FRAME_gwturnr1;
		self->turn_direction = TD_RIGHT;	// RSP 032699
	}
	else if (yaw > self->yaw_speed) // turning left
	{
		Turningleft = true;
		index = self->s.frame - FRAME_gwturnl1;
		self->turn_direction = TD_LEFT;	// RSP 032699
	}
	else							// no longer turning enough to care
	{
		gwhite_straightenout(self);	// RSP 012099
		return;
	}

	// RSP 041199 - New decision code to make sure correct pivot frames are used
	if (Turningleft)
	{
		if ((self->monsterinfo.currentmove != &gwhite_move_tleftstart) &&
		    (self->monsterinfo.currentmove != &gwhite_move_tleft))
		{
			self->monsterinfo.currentmove = &gwhite_move_tleftstart;
			self->turn_direction = TD_LEFT;
			return;
		}
	}
	else
		if ((self->monsterinfo.currentmove != &gwhite_move_trightstart) &&
		    (self->monsterinfo.currentmove != &gwhite_move_tright))
		{
			self->monsterinfo.currentmove = &gwhite_move_trightstart;
			self->turn_direction = TD_RIGHT;
			return;
		}

	if (index == 1 || index == 7)	// set frames to loop through main 'pivoting' loop
		if (Turningleft)
			self->monsterinfo.currentmove = &gwhite_move_tleft;
		else
			self->monsterinfo.currentmove = &gwhite_move_tright;
}

//
//  PAIN
//

//--pain2--
mframe_t gwhite_frames_pain2 [] =
{
	ai_move, GWHITE_WALK_SPEED/2, NULL,	// 1
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL	// 5
};
mmove_t gwhite_move_pain2 = {FRAME_gwpain1, FRAME_gwpain5, gwhite_frames_pain2, gwhite_run};

//-- pain1 --
mframe_t gwhite_frames_pain1 [] =
{
	ai_move, GWHITE_WALK_SPEED/2, NULL,	// 6
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL,
	ai_move, GWHITE_WALK_SPEED/2, NULL	// 10
};
mmove_t gwhite_move_pain1 = {FRAME_gwpain6, FRAME_gwpain10, gwhite_frames_pain1, gwhite_run};

// RSP 032699 - Check shark's health. If it's low, send him after some jellies if he isn't
// already on that mission
void gwhite_checkhealth(edict_t *self)
{
	edict_t *target = NULL;	// for finding jellies
	float	dist;			// not used, but must be present
	trace_t	tr;

	if (self->health < (self->max_health / 2))

		// See if there's a jelly around. If you find one,
		// forget the player and go after the jelly to boost your health.
		// If you're already looking for a jelly, skip this part

		if (!(self->enemy && (self->enemy->identity == ID_JELLY)))
			while (target = _findlifeinradius(self,target,self->s.origin,2000,false,true,false,&dist))
				if ((target->identity == ID_JELLY) && (target->flags & FL_SWIM))
				{
					// Check for line of sight
					tr = gi.trace (self->s.origin,NULL,NULL,target->s.origin,self,MASK_MONSTERSOLID);
					if (tr.ent == target)
					{
						if (self->enemy && self->enemy->client)	// player?
						{
							Forget_Player(self);
							self->monsterinfo.walk(self);
						}	
						self->enemy = self->goalentity = target;	// Look for jelly
						break;
					}
				}
}

//--pain logic
void gwhite_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	// Free target if necessary
	if (self->enemy && self->enemy->locked_to == self)	// MRB 030599 Fixes 2b crash
		self->enemy->locked_to = NULL;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	
	if (rand()&1)	// RSP 041799 - simplify
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &gwhite_move_pain1;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &gwhite_move_pain2;
	}
}

//
// DEATH
//

void gwhite_drop (edict_t *self)
{
	// start dropping
	self->movetype = MOVETYPE_TOSS;
	self->velocity[0] = self->velocity[1] = 0;	// RSP 071800
}

void gwhite_dead (edict_t *self)
{
//	VectorSet (self->mins, -16, -16, -24);
//	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
//	self->nextthink = 0;
	gi.linkentity (self);
}

// MRB 19980222 - 2 death frames now and a couple of spastic 
//     close range death nibbles

//--death1
mframe_t gwhite_frames_death1 [] =
{
	ai_move, GWHITE_WALK_SPEED,	  NULL,	// 1
	ai_move, GWHITE_WALK_SPEED,	  NULL,
	ai_move, GWHITE_WALK_SPEED-2, NULL,
	ai_move, GWHITE_WALK_SPEED-4, NULL,
	ai_move, GWHITE_WALK_SPEED-6, gwhite_nibble,
	ai_move, GWHITE_WALK_SPEED-8, NULL,
	NULL, 0,	 gwhite_nibble,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,	// 10

	NULL, 0,	 gwhite_drop,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,	// 20

	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL	// 30
};

mmove_t gwhite_move_death1 = {FRAME_gwdeth1, FRAME_gwdeth30, gwhite_frames_death1, gwhite_dead};

//--death2
mframe_t gwhite_frames_death2 [] =
{
	ai_move, GWHITE_WALK_SPEED,	  NULL,	// 31
	ai_move, GWHITE_WALK_SPEED,	  NULL,
	ai_move, GWHITE_WALK_SPEED-2, NULL,
	ai_move, GWHITE_WALK_SPEED-4, NULL,
	ai_move, GWHITE_WALK_SPEED-6, gwhite_nibble,
	ai_move, GWHITE_WALK_SPEED-8, NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,	// 40

	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 gwhite_nibble,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 gwhite_drop,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,	// 50

	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL,
	NULL, 0,	 NULL	// 55
};

mmove_t gwhite_move_death2 = {FRAME_gwdeth31, FRAME_gwdeth55, gwhite_frames_death2, gwhite_dead};

//--death logic
void gwhite_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;
	int r;

	self->monsterinfo.pivot = NULL;

// Free target if necessary
	if (self->enemy && (self->enemy->locked_to == self))	// RSP 070400
		self->enemy->locked_to = NULL;

/* RSP 070400 - teamchain not used
// Prevent movement of these guys from affecting the master after death
	if (self->teamchain)
	{
		edict_t *chainent;
		for (chainent = self->teamchain; chainent; chainent=chainent->teamchain)
		{
			chainent->deadflag = DEAD_DEAD;
			// Do we want this?  Shark might slide through a wall after being shot.
			chainent->flags &= ~FL_EFFECT_MASTER;
		}
	}

 */

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0 ; n < 2 ; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0 ; n < 2 ; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

/* RSP 070400 - teamchain not used
		// Free extensions - ideally this should be put in a main 'extensions' file
		//  and coded somehow - have to free from the tail up because the memory is zeroed
		if (self->teamchain)
		{
			if (self->teamchain->teamchain)
			{
				G_FreeEdict(self->teamchain->teamchain);
				self->teamchain->teamchain = NULL;
			}
			G_FreeEdict(self->teamchain);
			self->teamchain = NULL;
		}
 */
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	r = (rand() + 1) % 2;
	if (r == 0)
		self->monsterinfo.currentmove = &gwhite_move_death1;
	else
		self->monsterinfo.currentmove = &gwhite_move_death2;	
}

void monster_gwhite_indexing (edict_t *self)
{
	// RSP 102799 - uniform sound filenames
	sound_pain1		= gi.soundindex ("gwhite/pain1.wav");	
	sound_pain2		= gi.soundindex ("gwhite/pain2.wav");	
	sound_death		= gi.soundindex ("gwhite/death1.wav");	
	sound_chomp		= gi.soundindex ("gwhite/attack1.wav");
	sound_attack	= gi.soundindex ("gwhite/attack2.wav");
	sound_sight		= gi.soundindex ("gwhite/sight.wav");
	sound_idle		= gi.soundindex ("gwhite/idle.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/gwhite/tris.md2");	// RSP 061899
}


// RSP 012099: Changed bounding box values
/*QUAKED monster_gwhite (1 .5 0) (-50 -50 -30) (50 50 20) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_gwhite (edict_t *self)
{
	// AJA 19980223 - I had disabled this, my bad. I didn't notice that there are no
	// monsters in deathmatch. Of course there aren't I just didn't notice until I
	// thought about it and saw this same code elsewhere.
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_gwhite_indexing (self);

	// RSP 071400 - trying a fixed, square bounding box for a while instead
	// of changing all the time.

	VectorSet (self->mins, -50, -50, -30);	// RSP 071400: Changed bounding box values
	VectorSet (self->maxs, 50, 50, 19);		// RSP 071400: Changed bounding box values
//	M_ChangeBBox(self);	// RSP 012099: Change bounding box

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

//	self->model = "models/monsters/gwhite/tris.md2";	// RSP 061899

	self->health = hp_table[HP_SHARK];		// RSP 032399
	self->gib_health = -50;
	self->mass = 300;

	self->pain = gwhite_pain;
	self->die = gwhite_die;

	self->yaw_speed = 10;
	self->ideal_yaw = self->s.angles[YAW]; // RSP 012099 This was missing; shark always pointed east

// AJA 19980223 - Make this monster "pitch aware" so that it angles
// up and down on vertical movement and enemy tracking.

// AJA 19980308 - Commented out for 3.14 build. Pitching hasn't been
// put back in (yet?).
	//self->pitch_speed = 4;
	//self->flags |= FL_PITCH_AWARE;

	self->monsterinfo.stand = gwhite_stand;
	self->monsterinfo.walk = gwhite_walk;
	self->monsterinfo.run = gwhite_run;
	self->monsterinfo.melee = gwhite_melee;
	self->monsterinfo.sight = gwhite_sight;
	self->monsterinfo.dodge = NULL;				// RSP 032699 - remove shark dodge code
	self->monsterinfo.search = gwhite_idle;		// RSP 102899
	self->monsterinfo.pivot = gwhite_pivot;
	self->flags |= (FL_SWIM|FL_PASSENGER);
	self->identity = ID_GREAT_WHITE;			// RSP 012099: Great White flag for faster checking
	self->gravity = 0.05;						// RSP 012099: Less gravity underwater
	self->turn_direction = TD_STRAIGHT;			// RSP 032699: straight ahead
	self->count2 = 0;							// RSP 041099: init counter for timing getting stuck on walls
	self->spawnflags |= SPAWNFLAG_AMBUSH;		// RSP 071400: so shark has to see you to attack

	gi.linkentity (self);

// MRB 19980222 - Start with walk frame - always moving
	self->monsterinfo.currentmove = &gwhite_move_walk;	
	self->monsterinfo.scale = MODEL_SCALE;

// MRB 10090917 - tack on fin and tail extensions
//	self->teamchain = New_Fin_Ext(self);				// RSP 012099: Not used now
//	self->teamchain->teamchain = New_Tail_Ext(self);	// RSP 012099
//	self->teamchain->teamchain->teamchain = NULL;		// RSP 012099

	swimmonster_start (self);
}
