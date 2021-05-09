#include "g_local.h"

#define FRAME_JELLY_STAND_START 0
#define FRAME_JELLY_STAND_END   19

#define FRAME_JELLY_SWIM_START 20
#define FRAME_JELLY_SWIM_END   39

#define FRAME_JELLY_PAINA_START 40
#define FRAME_JELLY_PAINA_END   49

#define FRAME_JELLY_PAINB_START 50
#define FRAME_JELLY_PAINB_END   59

#define JELLY_FLEE_DISTANCE		150	// RSP 022099: Jelly will flee if player gets this close

static int	sound_water_idle;	// RSP 040199 - added code for idle sound
static int	sound_water_death1;
static int	sound_water_death2;
static int	sound_land_idle;	// RSP 040199 - added code for idle sound
static int	sound_land_death1;
static int	sound_land_death2;
static int	sound_land_pain1;
static int	sound_land_pain2;
static int	sound_water_pain1;
static int	sound_water_pain2;


// -- and off we go ... ---
void jelly_think (edict_t *self);

/* RSP 072500 - Not used

// Pick a spot and generate a fake goalentity path_corner for this guy to chase after

void jelly_choose_destiny(edict_t *self)
{
	jelly_think(self);
}
 */


//-- if startled --

void jelly_panic (edict_t *self)
{
	jelly_think(self);
}


// -- jelly pain stuff --

void jelly_glow (edict_t *self)
{
// When struck, the jelly will glow and then start healing again - 
//	the idea is that you will kill them if you are not careful.

	self->s.effects |= EF_ROCKET;

//gi.linkentity(self);
}

void jelly_stop_glow(edict_t *self)
{
	self->s.effects &= ~(EF_ROCKET);
//	gi.linkentity(self);
}

// RSP 022099: Adjusted so jelly glows longer and always shuts off at some point

void jelly_check_not_glow(edict_t *self)
{
	int max_health = self->flags & FL_SWIM ? hp_table[HP_WATERJELLY] : hp_table[HP_LANDJELLY];

	if (self->health < max_health)
		if (random() < 0.1)				// bump up only 1 in 10 times on average
		{
			self->health += 10;
			if (self->health >= max_health)
			{
				jelly_stop_glow(self);
				self->health = max_health; // adjust down if over max_health
			}
		}
}


// RSP 040199 - add code for jelly idling

void jelly_water_idle (edict_t *self)
{
	if (random() < JELLY_IDLE)	// RSP 070400
		gi.sound (self, CHAN_VOICE, sound_water_idle, 1, ATTN_IDLE, 0);
}


void jelly_land_idle (edict_t *self)
{
	if (random() < JELLY_IDLE)	// RSP 070400
		gi.sound (self, CHAN_VOICE, sound_land_idle, 1, ATTN_IDLE, 0);
}


// RSP 022099: Check for player and flee if too close

void jelly_check_flee(edict_t *self)
{
	edict_t *client;
	float	len;
	vec3_t	v;
	
	client = level.sight_client;
	if (!client)
		return;	// no clients to run from
	VectorSubtract(self->s.origin, client->s.origin, v);	// find vector
	len = VectorLength(v);									// find distance
	if (len < JELLY_FLEE_DISTANCE)							// too close?
	{
		self->ideal_yaw = vectoyaw(v);						// set yaw
		// If you want a small 'spurt' away from the player, uncomment the
		// following 2 lines
//		VectorScale (v, 0.25, v);							// set up move away
//		SV_movestep (self,v,true,false);					// move away
	}
}


// RSP 041799 - Corrected death sounds

void jelly_death_sounds(edict_t *self)
{
	int sound;

	if (rand()&1)
	{
		if (self->flags & FL_SWIM)	// water jelly
			sound = sound_water_death1;
		else
			sound = sound_land_death1;
	}
	else
	{
		if (self->flags & FL_SWIM)	// water jelly
			sound = sound_water_death2;
		else
			sound = sound_land_death2;
	}

	gi.sound(self,CHAN_VOICE,sound,1,ATTN_NORM,0);
}


// RSP 022099: Player consumes jelly for health if bumped
// RSP 032699: Shark consumes jelly for health if bumped
// RSP 032399: +15 for water jelly, +20 for land jelly
void jelly_touch(edict_t *j, edict_t *eater, cplane_t *plane, csurface_t *surf)
{
	if ((eater->client && (eater->health < 90)) ||		// If player health is less than 90,
		((eater->identity == ID_GREAT_WHITE) && (eater->health < (eater->max_health/2))))	// shark's hurting
	{
		if (j->flags & FL_SWIM)
			eater->health += 15;	// water jelly
		else
			eater->health += 20;	// land jelly
		if (eater->health > eater->max_health)
			eater->health = eater->max_health;

		jelly_death_sounds(j);	// RSP 041799 - corrected death sounds

		G_FreeEdict (j);	// Remove jelly from game
	}
}

// -- standing around --
mframe_t jelly_frames_stand [] =
{
	ai_stand, 0, NULL,	// 0
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_stand, 0, NULL,	// 5
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_stand, 0, NULL,	// 10
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_stand, 0, NULL,	// 15
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jelly_check_not_glow	// 19
};
mmove_t jelly_move_stand = {FRAME_JELLY_STAND_START, FRAME_JELLY_STAND_END, jelly_frames_stand, jelly_think};

// RSP 022099: Slow down jelly movement to half what it was before
//-- swimming around --
mframe_t jelly_frames_swim [] =
{
	ai_walk, 3, jelly_check_flee,	// 20
	ai_walk, 5, NULL,
	ai_walk, 6, NULL,
	ai_walk, 6, NULL,
	ai_walk, 5, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_walk, 5, jelly_check_flee,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 4, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_walk, 3, jelly_check_flee,	// 30
	ai_walk, 2, NULL,
	ai_walk, 2, NULL,
	ai_walk, 2, NULL,
	ai_walk, 2, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_walk, 2, jelly_check_flee,
	ai_walk, 2, NULL,
	ai_walk, 2, NULL,
	ai_walk, 3, NULL,
	ai_walk, 4, jelly_check_not_glow	// 39
};
mmove_t jelly_move_swim = {FRAME_JELLY_SWIM_START, FRAME_JELLY_SWIM_END, jelly_frames_swim, jelly_think};

mframe_t jelly_frames_pain [] =
{
	ai_stand, 0, jelly_glow,	// 40 (A) 50 (B)
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// RSP 022099: removed jelly_check_not_glow to slow down regeneration

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jelly_check_not_glow	// 49 (A) 59(B)
};
mmove_t jelly_move_paina = {FRAME_JELLY_PAINA_START, FRAME_JELLY_PAINA_END, jelly_frames_pain, jelly_panic};
mmove_t jelly_move_painb = {FRAME_JELLY_PAINB_START, FRAME_JELLY_PAINB_END, jelly_frames_pain, jelly_panic};

void jelly_water_pain(edict_t *self ,edict_t *other, float kick, int damage)
{
	if ((rand() + 1) % 2 == 1)
	{
		self->monsterinfo.currentmove = &jelly_move_paina;
		gi.sound(self,CHAN_ITEM,sound_water_pain1,1,ATTN_NORM,0);
	}
	else
	{
		self->monsterinfo.currentmove = &jelly_move_painb;
		gi.sound(self,CHAN_ITEM,sound_water_pain2,1,ATTN_NORM,0);
	}
}

void jelly_land_pain(edict_t *self ,edict_t *other, float kick, int damage)
{
	if ((rand() + 1) % 2 == 1)
	{
		self->monsterinfo.currentmove = &jelly_move_paina;
		gi.sound(self,CHAN_ITEM,sound_land_pain1,1,ATTN_NORM,0);
	}
	else
	{
		self->monsterinfo.currentmove = &jelly_move_painb;
		gi.sound(self,CHAN_ITEM,sound_land_pain2,1,ATTN_NORM,0);
	}
}

//-- dying code --
void jelly_dead (edict_t *self)
{
	self->nextthink = 0;
	jelly_stop_glow (self);
	self->svflags |= SVF_DEADMONSTER;
	self->movetype = MOVETYPE_TOSS;
	gi.linkentity (self);
}


/* RSP 073000 - not used
mframe_t jelly_frames_die [] =
{
	ai_move, 0, NULL,	// 30
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 39
};
mmove_t jelly_move_die = {(FRAME_JELLY_SWIM_END - 9), FRAME_JELLY_SWIM_END, jelly_frames_die, NULL};
*/


void jelly_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	if (self->health <= self->gib_health)	// RSP 030599
	{
		for (n = 0 ; n < 4 ; n++)
			ThrowGib(self,"models/objects/gibs/tissue/tris.md2",damage,GIB_ORGANIC);	// RSP 030599

		jelly_death_sounds(self);	// RSP 041799 - corrected death sounds

		self->deadflag = DEAD_DEAD;
		self->think = G_FreeEdict;
		return;
	}

/*	RSP 072800 - Since jellies are gibbed when they die, you should never reach this part
// of the code 
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	ThrowGib (self, "models/objects/gibs/tissue/tris.md2", damage, GIB_ORGANIC);	// RSP 030599
	self->monsterinfo.currentmove = &jelly_move_die;
 */
}

// -- hmmm ... what to do...
void jelly_think (edict_t *self)
{
	self->monsterinfo.currentmove = &jelly_move_swim;
}

void monster_jelly_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	sound_water_idle   = gi.soundindex ("jfish/idle.wav");		// RSP 040199
	sound_water_death1 = gi.soundindex ("jfish/death1.wav");
	sound_water_death2 = gi.soundindex ("jfish/death2.wav");
	sound_water_pain1  = gi.soundindex ("jfish/pain1.wav");
	sound_water_pain2  = gi.soundindex ("jfish/pain2.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/jelly/tris.md2");	// RSP 061899
}

void monster_landjelly_indexing (edict_t *self)
{
	sound_land_idle   = gi.soundindex ("ljfish/idle.wav");		// RSP 040199
	sound_land_death1 = gi.soundindex ("ljfish/death1.wav");
	sound_land_death2 = gi.soundindex ("ljfish/death2.wav");
	sound_land_pain1  = gi.soundindex ("ljfish/pain1.wav");
	sound_land_pain2  = gi.soundindex ("ljfish/pain2.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/landjell/tris.md2");	// RSP 061899
}


//-- spawning code --

/*QUAKED monster_jelly (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_jelly (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_jelly_indexing (self);

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	VectorSet (self->mins, -16, -16, -16);
	VectorSet (self->maxs, 16, 16, 16);

//	self->model = "models/monsters/jelly/tris.md2";		// RSP 061899

	self->ideal_yaw = self->s.angles[YAW]; // RSP 022099 This was missing; jelly always pointed east
	self->monsterinfo.aiflags |= AI_PACIFIST | AI_GOOD_GUY;	// RSP 022099: Jelly won't chase player
	self->health = hp_table[HP_WATERJELLY];	// RSP 032399
	self->gib_health = 0;			// RSP 030599 Gib jelly when he dies
	self->mass = 30;

	self->damage_debounce_time = 0;
	self->flags |= (FL_SWIM|FL_PASSENGER);
	self->identity = ID_JELLY;				// RSP 022099: Jelly flag for faster checking

	gi.linkentity (self);

	self->pain = jelly_water_pain;
	self->die = jelly_die;
	self->touch = jelly_touch;				// RSP 022099: Player eats jelly for health when bumped

	self->monsterinfo.stand = jelly_think;
	self->monsterinfo.walk = jelly_think;
	self->monsterinfo.run = jelly_think;
	self->monsterinfo.melee = jelly_panic;

	// .search is used in ai_walk
	// .idle is used in ai_stand
	// Since jellies don't stand around, you need to use .search for the idle sound.

	self->monsterinfo.search = jelly_water_idle;	// RSP 040199
	
	self->monsterinfo.currentmove = &jelly_move_stand;
	self->monsterinfo.scale = 1;

	swimmonster_start (self);
}

// 'Landjelly' code

/*QUAKED monster_landjelly (1 .5 0) (-24 -24 0) (24 24 24) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_landjelly (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_landjelly_indexing (self);

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	VectorSet (self->mins, -24, -24, -0);
	VectorSet (self->maxs, 24, 24, 24);

//	self->model = "models/monsters/landjell/tris.md2";	// RSP 061899

	self->ideal_yaw = self->s.angles[YAW]; // RSP 022799
	self->monsterinfo.aiflags |= AI_PACIFIST | AI_GOOD_GUY;	// RSP 022799: Jelly won't chase player
	self->health = hp_table[HP_LANDJELLY];	// RSP 032399
	self->gib_health = 0;			// RSP 030599 Gib jelly when he dies
	self->mass = 30;
	self->flags |= FL_PASSENGER;
	self->identity = ID_JELLY;		// RSP 022799: Jelly flag for faster checking

	self->damage_debounce_time = 0;

	gi.linkentity (self);

	self->pain = jelly_land_pain;
	self->die = jelly_die;
	self->touch = jelly_touch;			// RSP 022799: Player eats jelly for health when bumped

	self->monsterinfo.stand = jelly_think;
	self->monsterinfo.walk = jelly_think;
	self->monsterinfo.run = jelly_think;
	self->monsterinfo.melee = jelly_panic;

	// .search is used in ai_walk
	// .idle is used in ai_stand
	// Since jellies don't stand around, you need to use .search for the idle sound.

	self->monsterinfo.search = jelly_land_idle;	// RSP 040199
	
	self->monsterinfo.currentmove = &jelly_move_stand;
	self->monsterinfo.scale = 1.25;

	walkmonster_start (self);
}
