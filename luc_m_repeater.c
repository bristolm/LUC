/*
==============================================================================

repeater

==============================================================================
*/

#include "g_local.h"
#include "luc_m_repeater.h"
extern qboolean enemy_vis;	// RSP 042999
extern int	enemy_range;	// RSP 042999

#define DEFAULT_REPEAT_HSPREAD	1000	// MRB 022799
#define DEFAULT_REPEAT_VSPREAD	500		// MRB 022799

void repeaterFireBlast (edict_t *self);	// MRB 022799
extern qboolean infront (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;
static int	sound_shell_open;			// MRB 022799
static int	sound_sight;
static int	sound_idle;

void repeater_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


mframe_t repeater_frames_fidgit [] =
{
	ai_stand, 0, NULL,	// 1
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 10
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 20
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 30
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL	// 40
};
mmove_t repeater_move_fidgit = {FRAME_stand_a1, FRAME_stand_a40, repeater_frames_fidgit, NULL};

mframe_t repeater_frames_stand [] =
{
	ai_stand, 0, NULL,	// 1
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 10
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL	// 20
};
mmove_t repeater_move_stand = {FRAME_stand_b1, FRAME_stand_b20, repeater_frames_stand, NULL};

void repeater_stand (edict_t *self)
{
	self->yaw_speed = 20;
	if (random() < 0.2)
		self->monsterinfo.currentmove = &repeater_move_fidgit;
	else
		self->monsterinfo.currentmove = &repeater_move_stand;
}

// WALK
mframe_t repeater_frames_stop_walk [] =
{
	ai_walk, 0, NULL,	// 1
	ai_walk, 0, NULL	// 2
};
mmove_t repeater_move_stop_walk = {FRAME_run_end1, FRAME_run_end2, repeater_frames_stop_walk, NULL};

mframe_t repeater_frames_walk [] =
{
	ai_walk, 10, NULL,	// 1	// RSP 082100 - these were 20, now 10 to slow him down
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL	// 8
};
mmove_t repeater_move_walk = {FRAME_run_cycle1, FRAME_run_cycle8, repeater_frames_walk, NULL};

void repeater_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &repeater_move_walk;
}

mframe_t repeater_frames_start_walk [] =
{
	ai_walk, 0, NULL,	// 1
	ai_walk, 0, NULL	// 2
};
mmove_t repeater_move_start_walk = {FRAME_run_start1, FRAME_run_start2, repeater_frames_start_walk, repeater_walk};

void repeater_start_walk (edict_t *self)
{
	self->yaw_speed = 20;
	self->monsterinfo.currentmove = &repeater_move_start_walk;
}


// RUN

mframe_t repeater_frames_run [] =
{
	ai_run, 20, NULL,	// 1	// RSP 082100 - these were 10, now 20 to speed him up
	ai_run, 20, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL	// 8
};
mmove_t repeater_move_run = {FRAME_run_cycle1, FRAME_run_cycle8, repeater_frames_run, NULL};

void repeater_run (edict_t *self)
{
	self->yaw_speed = 20;
	self->monsterinfo.currentmove = &repeater_move_run;
}

mframe_t repeater_frames_start_run [] =
{
	ai_run, 0, NULL,	// 1
	ai_run, 0, NULL		// 2
};
mmove_t repeater_move_start_run = {FRAME_run_start1, FRAME_run_start2, repeater_frames_start_run, repeater_run};

mframe_t repeater_frames_stop_run [] =
{
	ai_run, 0, NULL,	// 1
	ai_run, 0, NULL		// 2
};
mmove_t repeater_move_stop_run = {FRAME_run_end1, FRAME_run_end2, repeater_frames_stop_run, NULL};

void repeater_start_run (edict_t *self)
{
	self->yaw_speed = 20;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &repeater_move_stand;
	else
		self->monsterinfo.currentmove = &repeater_move_start_run;
}

// PIVOT (Only while attacking though)
#define		FRAME_REP_TRIGHT_START		FRAME_attack_tright1
#define		FRAME_REP_TRIGHT_END		FRAME_attack_tright10
#define		FRAME_REP_TLEFT_START		FRAME_attack_tleft1
#define		FRAME_REP_TLEFT_END			FRAME_attack_tleft10

// MRB 022799 Changed from machinegun to shotgun
mframe_t frames_repeater_pivot [] =
{
	ai_charge, 0, 	NULL,	// 1
	ai_run,	   5,	NULL,	// WPO 092499  fix(?) for repeater not moving	// RSP 082100 - 10->5
	ai_charge, 0, 	NULL,
	ai_run,	   5,	NULL,	// WPO 092499  fix(?) for repeater not moving
	ai_charge, 0, 	repeaterFireBlast,

	ai_charge, 0, 	NULL,	// 6
	ai_run,	   5,	NULL,	// WPO 092499  fix(?) for repeater not moving
	ai_charge, 0, 	NULL,
	ai_run,	   5,	NULL,	// WPO 092499  fix(?) for repeater not moving
	ai_charge, 0, 	repeaterFireBlast	// 10
};
mmove_t move_repeater_tleft = {FRAME_REP_TLEFT_START, FRAME_REP_TLEFT_END, frames_repeater_pivot, NULL};
mmove_t move_repeater_tright = {FRAME_REP_TRIGHT_START, FRAME_REP_TRIGHT_END, frames_repeater_pivot, NULL};

//fwd declaration
mmove_t repeater_move_attack;

void repeater_pivot (edict_t *self, float yaw)
{
	/* OK, this is a copy of the 'bot' stuff.  ONLY occurs during attacking though.
	 * order in .h file is (ok, so I probably should not be doing it this way ...)
		FRAME_REP_TLEFT_START
		FRAME_REP_TLEFT_END
		FRAME_REP_TRIGHT_START
		FRAME_REP_TRIGHT_END
	 * NOTE:  This stuff should really be standardized and written as a function ...
	 */ 

	int index;

	if (self->s.frame <= FRAME_attack_cycle10 && self->s.frame >= FRAME_attack_cycle1 )
		index = self->s.frame - FRAME_attack_cycle1;
	else if (self->s.frame <= FRAME_REP_TRIGHT_END  && self->s.frame >= FRAME_REP_TLEFT_START)
		index = (-1);	// We are already in the pivot frames ...
	else	// Not doing anything we care about - return
		return;

	
	if (index > 0)
	{	/* turning far enough to care ... */
		if (yaw < -self->yaw_speed)	  /* turning right */
		{
			self->monsterinfo.currentmove = &move_repeater_tright;
			self->monsterinfo.nextframe = FRAME_REP_TRIGHT_START + index;
		}
		else if (yaw > self->yaw_speed) /* turning left */
		{
			self->monsterinfo.currentmove = &move_repeater_tleft;
			self->monsterinfo.nextframe = FRAME_REP_TLEFT_START + index;
		}
		else
			return;
	}
	else	/* ALWAYS in a turning frame in this case... (note - must be contiguous in .md2) */
	{		
		if (yaw < -self->yaw_speed)
		{		/* turning right */
			if (self->s.frame >=FRAME_REP_TLEFT_START)  /* I HAD been turning left */
			{										/* so I have to reset things */
				self->monsterinfo.currentmove = &move_repeater_tright;
				self->monsterinfo.nextframe = self->s.frame - FRAME_REP_TLEFT_START
									+ FRAME_REP_TRIGHT_START;
			}
		}
		else if (yaw > self->yaw_speed)
		{		/* turning left */
			if (self->s.frame <=FRAME_REP_TRIGHT_END)  /* I HAD been turning right */
			{										/* so I have to reset things */
				self->monsterinfo.currentmove = &move_repeater_tleft;
				self->monsterinfo.nextframe =  self->s.frame - FRAME_REP_TRIGHT_START 
									+ FRAME_REP_TLEFT_START;
			}
		}
		else	/* stop pivoting and return to attacking */
		{
			self->monsterinfo.currentmove = &repeater_move_attack;
			if (self->s.frame < FRAME_REP_TLEFT_START)
				self->monsterinfo.nextframe = (int)((self->s.frame - FRAME_REP_TRIGHT_START)/2) + FRAME_attack_cycle1;
			else
				self->monsterinfo.nextframe = (int)((self->s.frame - FRAME_REP_TLEFT_START)/2) + FRAME_attack_cycle1;
		}
	}
}

// ATTACK
vec3_t	death_fire_angles[] =
{
	0.0,  10.0, 0.0,
	0.0,  35.0, 0.0,
	0.0,  50.0, 0.0,
	0.0,  25.0, 0.0,
	0.0,   0.0, 0.0,
	0.0, -30.0, 0.0,
	0.0, -15.0, 0.0,
	0.0,   0.0, 0.0
};

vec3_t  death_fire_sparkspots[] =
{
	0,0,0
};

vec3_t  regular_fire_sparkspots[] =
{
	0,0,0
};

void ThrowSpark(vec3_t start, vec3_t dir)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPARKS);
	gi.WritePosition (start);
	gi.WriteDir (dir);
	gi.multicast (start, MULTICAST_PVS);
}

static vec3_t live_flash_offset = {30,0,25};
static vec3_t dead_flash_offset = {-4,0,25};

// MRB 022799 Changed from machine gun to shotgun
void repeaterFireBlast (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right;
	vec3_t	vec;

	if (self->s.frame < FRAME_death_a1)
	{	// Regular attack  (30,0,25 = center)
		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, live_flash_offset, forward, right, start);

		if (self->enemy)
		{
			VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;
			VectorSubtract (target, start, forward);
			VectorNormalize (forward);
		}
		else
		{
			AngleVectors (self->s.angles, forward, right, NULL);
		}

		if (self->s.frame == FRAME_attack_cycle10 && 
			self->enemy && infront(self,self->enemy) && random() < 0.3)
		
			self->monsterinfo.nextframe = FRAME_attack_cycle1;

		// WPO 072599  Only fire if enemy can be seen. and is alive
		if (infront(self,self->enemy) && visible(self,self->enemy) && self->enemy->health > 0)
		{
			if (self->s.frame == (self->monsterinfo.currentmove->firstframe + 4))
				monster_fire_shotgun (self, start, forward, 
								skill->value, 0, 
								DEFAULT_REPEAT_HSPREAD/2, DEFAULT_REPEAT_VSPREAD/2, 
								skill->value * 2 + 5, MZ2_INFANTRY_MACHINEGUN_2);
			else
				monster_fire_shotgun (self, start, forward, 
								skill->value, 0, 
								DEFAULT_REPEAT_HSPREAD, DEFAULT_REPEAT_VSPREAD, 
								skill->value * 2 + 5, MZ2_INFANTRY_MACHINEGUN_2);
		}
		
	}
	else
	{	// Death throw thingee .. (-5,0,25 = center)
		int count = ((rand() + 1) % (4 + (int)skill->value)) - 4;  //range (5 -> 8) - 4 

		if (count < 0)
			return;

		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, dead_flash_offset, forward, right, start);

		VectorSubtract (self->s.angles, death_fire_angles[self->s.frame - FRAME_death_b_cycle1], vec);
		AngleVectors (vec, forward, NULL, NULL);

		if (self->s.frame == FRAME_death_b_cycle8)
			self->monsterinfo.nextframe = FRAME_death_b_cycle1;
		monster_fire_shotgun (self, start, forward, 
						skill->value, 0, 
						DEFAULT_REPEAT_HSPREAD, DEFAULT_REPEAT_VSPREAD, 
						count * 2 + 5, MZ2_INFANTRY_MACHINEGUN_2);
	}
}


void repeater_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_BODY, sound_sight, 1, ATTN_NORM, 0);
}

// MRB 022799 Changed from machine gun to shotgun
mframe_t repeater_frames_attack [] =
{
	ai_charge,	0, NULL,  // cycle	// 1
	ai_charge,	0, NULL,
	ai_charge,	0, NULL,
	ai_charge,	0, NULL,
	ai_charge,	0, repeaterFireBlast,

	ai_charge,	0, NULL,
	ai_charge,	0, NULL,
	ai_charge,	0, NULL,
	ai_charge,	0, NULL,
	ai_charge,	0, repeaterFireBlast,	// 10

	NULL,		0, NULL,  // end	// 1
	NULL,		0, NULL,
	NULL,		0, NULL,
	NULL,		0, NULL				// 4
};
mmove_t repeater_move_attack = {FRAME_attack_cycle1, FRAME_attack_end4, repeater_frames_attack, repeater_run};

void repeater_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &repeater_move_attack;
}

void repeater_open_shell(edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_shell_open, 1, ATTN_NORM, 0);
}

mframe_t repeater_frames_start_attack [] =
{	// Use a NULL AI function so he doesn't pivot while he's beginning to attack.
	NULL, 0,  repeater_open_shell,  // start	// 1
	NULL, 0,  NULL,
	NULL, 0,  NULL,
	NULL, 0,  NULL		// 4
};
mmove_t repeater_move_start_attack = {FRAME_attack_start1, FRAME_attack_start4, repeater_frames_start_attack, repeater_attack};

void repeater_start_attack(edict_t *self)
{
	self->yaw_speed = 9;	// make 10 pivot frames of 9 degrees each.
	self->monsterinfo.currentmove = &repeater_move_start_attack;
}


// PAIN
mframe_t repeater_frames_pain1 [] =
{
	ai_move, -3, NULL,	// 1
	ai_move, -2, NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL	// 4
};
mmove_t repeater_move_pain1 = {FRAME_pain_a1, FRAME_pain_a4, repeater_frames_pain1, repeater_run};

mframe_t repeater_frames_pain2 [] =
{
	ai_move, -3, NULL,	// 1
	ai_move, -3, NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL	// 5
};
mmove_t repeater_move_pain2 = {FRAME_pain_b1, FRAME_pain_b5, repeater_frames_pain2, repeater_run};

mframe_t repeater_frames_pain3_start [] =
{	// goes into attack frames after
	ai_move, -3, NULL,	// 1
	ai_move, -3, NULL,
//	ai_move, 0,  NULL,	// RSP 081800 - only 4 pain frames, not 5
	ai_move, -1, NULL,
	ai_move, -2, NULL	// 4
};
mmove_t repeater_move_pain3_start = {FRAME_pain_c1, FRAME_pain_c4,repeater_frames_pain3_start, repeater_attack};


void repeater_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	waypoint_check(self);	// RSP 062199 - retreat along waypoint path?

	self->yaw_speed = 20;

	//if (self->health < (self->max_health / 2))
	//	self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if ((rand() % 2) == 0)
	{
		self->monsterinfo.currentmove = &repeater_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &repeater_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

// DEAD

void repeater_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;	// WPO 102299 - stop thinking
	gi.linkentity (self);

//	M_FlyCheck (self);	// RSP 030599 - No more flies
}

mframe_t repeater_frames_death1 [] =
{
	ai_move, -4, NULL,	// 1
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL	// 4
};
mmove_t repeater_move_death1 = {FRAME_death_a1, FRAME_death_a4, repeater_frames_death1, repeater_dead};

void repeater_gib(edict_t *self, int damage)
{
	int n;

	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0 ; n < 2 ; n++)
		ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
	for (n = 0 ; n < 4 ; n++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
	self->deadflag = DEAD_DEAD;
}

void repeater_repeat_die(edict_t *self)
{
	repeaterFireBlast(self);

	if (level.time > self->touch_debounce_time)
	{
		repeater_gib(self, ((rand() + 1) % 10) + 20);
		repeater_dead(self);
		return;
	}

	self->monsterinfo.nextframe = FRAME_death_b_cycle1;
}

// Fire and fire and fire ...
mframe_t repeater_frames_death2 [] =
{
	ai_move, 0,   NULL,	// start	// 1
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,				// 3

	ai_move, 0,  repeaterFireBlast,  // cycle	// 1
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeaterFireBlast,
	ai_move, 0,  repeater_repeat_die			// 8
};
mmove_t repeater_move_death2 = {FRAME_death_b1, FRAME_death_b_cycle8, repeater_frames_death2, repeater_dead};

void repeater_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

// check for gib
// MRB 022799 - moved gibbing to its own function
	if (self->health <= self->gib_health)
	{
		repeater_gib(self,damage);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	n = (rand() + 1) % 3;

	if (n == 0)
	{
		// MRB 022799 - only repeat this way for a certain amount of time
		self->touch_debounce_time = level.time + 3 + skill->value * 2 ;  // 5 -> 13 seconds ...

		self->monsterinfo.currentmove = &repeater_move_death2;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &repeater_move_death1;
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	}
}


void monster_repeater_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	// MRB 022799 New sound dirs
	sound_pain1      = gi.soundindex ("repeater/pain1.wav");
	sound_pain2      = gi.soundindex ("repeater/pain2.wav");
	sound_die1       = gi.soundindex ("repeater/death1.wav");
	sound_die2       = gi.soundindex ("repeater/death2.wav");
	sound_shell_open = gi.soundindex ("repeater/open.wav");
	sound_sight      = gi.soundindex ("repeater/sight.wav");
	sound_idle       = gi.soundindex ("repeater/idle.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/bots/repeater/tris.md2");	// RSP 061899
}


/*QUAKED monster_repeater (1 .5 0) (-24 -24 0) (24 24 40) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_repeater (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_repeater_indexing (self);
	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

//	self->model = "models/monsters/bots/repeater/tris.md2";	// RSP 061899

	VectorSet (self->mins, -24, -24, 0);
	VectorSet (self->maxs, 24, 24, 40);

	self->health = hp_table[HP_REPEATER];	// RSP 032399
	self->gib_health = -40;	// MRB 031999
	self->mass = 200;

	self->pain = repeater_pain;
	self->die = repeater_die;

	self->monsterinfo.stand = repeater_stand;
	self->monsterinfo.walk = repeater_start_walk;
	self->monsterinfo.run = repeater_start_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = repeater_start_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = repeater_sight;
	self->monsterinfo.idle = repeater_idle;	// RSP 102799
	self->monsterinfo.pivot = repeater_pivot;

	self->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger
	self->identity = ID_REPEATER;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &repeater_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
