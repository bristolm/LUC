#include "g_local.h"


//
// BOT_DRILL ornament
//

// FIRING
// frames 0-4 are in, 5 is firing, 6 is out

#define DBOT_NOTHING	0
#define DBOT_STARTING	1
#define DBOT_STOPPING	2
#define DBOT_FIRING		3  // and greater

#define DBOT_LASER_COLOR_START		225		// Orange-ish laser
#define DBOT_LASER_COLOR_CHANGE		-2

#define DBOT_LASER_WIDTH_START		16		// FAT
#define DBOT_LASER_WIDTH_CHANGE		0

#define DBOT_LASER_NUMBER_CHANGES	5
#define DBOT_LASER_THRESH			(DBOT_FIRING + DBOT_LASER_NUMBER_CHANGES)
#define DBOT_LASER_REPEAT			(DBOT_FIRING + DBOT_LASER_NUMBER_CHANGES * 2)


#define SET_DBOT_LASER(c)	((((((c << 8) | c) << 8) | c) << 8) | c)

void bot_drill_fire (edict_t *self)
{	// use ->last_move_time for determining how long to fire.
	int		i;
	edict_t	*l;

	self->nextthink = level.time + FRAMETIME;
	switch (self->count)
	{
		case DBOT_NOTHING:	// just starting
			self->count = DBOT_STARTING;
			self->s.frame++;
			break;

		case DBOT_STARTING:	// moving to fire
			if (self->solid != SOLID_TRIGGER)
			{	// Someone wants to 'land'
				self->count = DBOT_STOPPING;
				self->s.frame = 11 - self->s.frame;
			}
			else if (++self->s.frame == 5)
			{
				self->count = DBOT_FIRING;
				self->last_move_time = level.time + (FRAMETIME * 1);
			}
			break;

		case DBOT_STOPPING:
			if (++self->s.frame > 10)
			{
				self->s.frame = 0;
				self->count = DBOT_NOTHING;
				self->last_move_time = level.time;

				if (self->solid == SOLID_TRIGGER)
					self->nextthink = level.time + (FRAMETIME * random() * 3);
				else
					self->nextthink = 0;
			}

			break;

			// Lock on the firing frame and play with the lasers until someone shows up
		default:
			if (self->solid != SOLID_TRIGGER)
			{	// Someone wants to 'land' - make the lasers invisible
				for (l = self->teamchain ; l ; l = l->teamchain)
				{
					l->svflags |= SVF_NOCLIENT;
					l->s.frame = DBOT_LASER_WIDTH_START;
					l->s.skinnum = SET_DBOT_LASER(DBOT_LASER_COLOR_START);
				}

				self->count = DBOT_STOPPING;
				self->s.frame = 6;
			}
			else
			{	// Do the lasering - ramp count from 3 to 3 + (DBOT_LASER_NUMBER_CHANGES * 2)
				if (++self->count > DBOT_LASER_REPEAT)
				{	// We reached a choice point.  Decide which ones stay on now.
					for (l = self->teamchain ; l ; l = l->teamchain)
					{	// Reset values and check each laser to see if it wants to be on or off
						l->s.frame = DBOT_LASER_WIDTH_START;
						l->s.skinnum = SET_DBOT_LASER(DBOT_LASER_COLOR_START);
						if (rand() & 3)
						{	// Switch the laser's state
							if (l->svflags & SVF_NOCLIENT)
								l->svflags &= ~SVF_NOCLIENT;	// OFF
							else
								l->svflags |= SVF_NOCLIENT;		// ON
						}
					}
					self->count = DBOT_FIRING;
				}
				else
				{	// Change the laser's properties
					i = (self->count <= DBOT_LASER_THRESH ? 1 : (-1));

					for (l = self->teamchain;l;l=l->teamchain)
					{
						int j = (l->s.skinnum & 0xFF) + (i * DBOT_LASER_COLOR_CHANGE);

						l->s.frame += (i * DBOT_LASER_WIDTH_CHANGE);
						l->s.skinnum = SET_DBOT_LASER(j);
					}
				}
			}
	}

	for (l = self->teamchain ; l ; l = l->teamchain)
	{	// Target the lasers
		if (!(l->svflags & SVF_NOCLIENT))	// NOT SVF_NOCLIENT means visible
		{
			vec3_t end;
			trace_t tr;

			_ExtendWithBounds(l->s.origin, 8000, l->movedir, NULL, NULL, end);
			tr = gi.trace (l->s.origin, NULL, NULL, end, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_LASER_SPARKS);
			gi.WriteByte (8);
			gi.WritePosition (tr.endpos);
			gi.WriteDir (tr.plane.normal);
			gi.WriteByte (l->s.skinnum);
			gi.multicast (tr.endpos, MULTICAST_PVS);

			VectorCopy(tr.endpos,l->s.old_origin);
		}
	gi.linkentity(l);
	}
}


// TOUCHED
void bot_drill_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other)
		return;

	if (other->movetarget != self)
		return;

	// If this isn't a probe using us, we'll have to try something else
//	if (Q_stricmp (other->classname, "misc_bot_probe"))
	if (other->identity != ID_PROBEBOT)	// RSP 080600 - use identity flag
		return;

	self->solid = SOLID_NOT;
	other->use(other,self,self);

	gi.linkentity(self);
}

static char botlasername[32] = "target_laser";

/*QUAKED misc_bot_drill (1 .5 0) (-116 -84 -200) (116 84 0)
*/
void SP_misc_bot_drill (edict_t *self)
{
	edict_t	*parent = self;
	int		i;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_TRIGGER;
	self->target_ent = NULL;

	self->count = DBOT_NOTHING;
	self->last_move_time = level.time;

	self->s.modelindex = gi.modelindex("models/objects/drillbot/tris.md2");

	VectorSet(self->mins,-2,-2,-2);
	VectorSet(self->maxs,2,2,2);

	self->s.frame = 0;
	self->touch = bot_drill_touch;
	self->identity = ID_DRILLBOT;	// RSP 080600

	self->think = bot_drill_fire;
	self->nextthink = level.time + 1;

	gi.linkentity(self);

	// Now give drillbot 2 little laser friends
	for (i = 0; i < 2; i++)
	{
		edict_t *l = G_Spawn();

		l->movetype = MOVETYPE_NONE;
		l->solid = SOLID_NOT;
		l->s.renderfx |= (RF_BEAM|RF_TRANSLUCENT);
		l->s.modelindex = 1;			// must be non-zero
		l->spawnflags = SPAWNFLAG_TLAS_START_ON;

		l->svflags = SVF_NOCLIENT;
		l->s.frame = DBOT_LASER_WIDTH_START;
		l->s.skinnum = SET_DBOT_LASER(DBOT_LASER_COLOR_START);

		l->s.origin[0] = self->s.origin[0];
		l->s.origin[1] = self->s.origin[1] + (64.8 * (i ? (-1) : 1));
		l->s.origin[2] = self->s.origin[2] - 126.7;

		VectorSubtract(self->s.origin,l->s.origin,l->movedir);
		l->movedir[2] -= 206;
		VectorNormalize(l->movedir);
		vectoangles(l->movedir,l->s.angles);

		VectorSet(l->mins,-8,-8,-8);
		VectorSet(l->maxs,8,8,8);

		l->classname = botlasername;
		l->teammaster = self;
		l->dmg = 1;

		// We're linking drillbot --- lasers[0] --- lasers[1] through teamchain
		parent->teamchain = l;
		l->teamchain = NULL;
		parent = l;

		gi.linkentity(l);
	}
}


//
// BOT_PROBE ornament
//


// STAND
mframe_t frames_bot_probe_stand_empty [] =
{
	ai_stand,10, NULL,
};
mmove_t move_bot_probe_stand_empty = {0, 0, frames_bot_probe_stand_empty, NULL};


void bot_probe_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &move_bot_probe_stand_empty;
}


// WALK (fly)
mframe_t frames_bot_probe_fly_empty [] =
{
	ai_walk, 10, NULL,
};
mmove_t move_bot_probe_fly_empty = {0, 0, frames_bot_probe_fly_empty, NULL};


void bot_probe_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &move_bot_probe_fly_empty;
}

/* RSP 080700 - remove ability for Probebot to be hurt or die. No point to it.

// PAIN
void bot_probe_pain(edict_t *self ,edict_t *other, float kick, int damage)
{
	bot_probe_walk(self);
}


// RSP 080600 - Added death code
// DEATH
void bot_probe_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	if (self->health <= self->gib_health)
	{
		for (n = 0 ; n < 4 ; n++)
			ThrowGib(self,"models/objects/gibs/gear/tris.md2",damage,GIB_METALLIC);

		self->deadflag = DEAD_DEAD;
		self->think = G_FreeEdict;
	}
}
 */

// USE

extern qboolean M_ChangeYaw (edict_t *ent);	// RSP 012099

void bot_align (edict_t *self, float dist)
{
	int		i;
	float	f;


	if (self->s.angles[YAW] != self->movetarget->s.angles[YAW])
	{	// Turn so we're looking in the proper direction
		self->ideal_yaw = self->movetarget->s.angles[YAW];
		M_ChangeYaw (self);
	}

	for (i = 0 ; i < 3 ; i++)
	{	// Move a little closer if we really need to
		if (fabs(f = (self->movetarget->s.origin[i] - self->s.origin[i])) > 10)
		{
			if (f < 0)
				self->s.origin[i] -= 10;
			else
				self->s.origin[i] += 10;
		}
		else
			self->s.origin[i] += f;
	}
}


void bot_probet_checktograb(edict_t *self)
{
	if (self->movetarget->s.frame != 0)
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
}


void bot_probe_doneload(edict_t *self)
{
	edict_t *next;

	// Reset the drill bot's 'condition'
	self->movetarget->solid = SOLID_TRIGGER;					// Allow the drill bot to be touched
	self->movetarget->nextthink = level.time + 1 + random();	// Allow the drillbot to think

	gi.linkentity(self->movetarget);

	if (self->movetarget->target)
		next = G_PickTarget(self->movetarget->target);

	self->movetarget = self->goalentity = next;

	bot_probe_walk(self);
}


mframe_t frames_bot_probe_load [] =
{
		bot_align, 0, NULL,	// down
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,

		bot_align, 0, bot_probet_checktograb,	// grab
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,

		bot_align, 0, NULL,	// up
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
		bot_align, 0, NULL,
};
mmove_t move_bot_probe_load = {0, 29, frames_bot_probe_load, bot_probe_doneload};

void bot_probe_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!other)
		return;

	if (self->movetarget != other)
		return;

	// If this isn't a drill using us, we'll have to try something else
//	if (Q_stricmp (other->classname, "misc_bot_drill"))
	if (other->identity != ID_DRILLBOT)	// RSP 080600 - use identity flag
		return;

	self->monsterinfo.currentmove = &move_bot_probe_load;
}

/*QUAKED misc_bot_probe (1 .5 0) (-50 -50 -80) (50 50 50)
*/
void SP_misc_bot_probe (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->monsterinfo.aiflags = (AI_GOOD_GUY | AI_PACIFIST);

	self->s.modelindex = gi.modelindex ("models/objects/probebot/tris.md2");

	VectorSet (self->mins, -50, -50, -80);
	VectorSet (self->maxs, 50, 50, 50);

	self->health = hp_table[HP_PROBEBOT];	// RSP 032399
	self->gib_health = -50;
	self->mass = 100;

	self->monsterinfo.stand = bot_probe_stand;
	self->monsterinfo.idle = bot_probe_stand;

	self->monsterinfo.walk = bot_probe_walk;
	self->monsterinfo.run = bot_probe_walk;

//	self->pain = bot_probe_pain;	// RSP 080700
//	self->die = bot_probe_die;		// RSP 080700
	self->yaw_speed = 25;
	self->monsterinfo.scale = 1;
	self->identity = ID_PROBEBOT;	// RSP 080600

	bot_probe_walk(self);

	gi.linkentity(self);

	flymonster_start(self);

	// use is redefined in flymonster_start - so 'fix' it here
	self->use = bot_probe_use;

	// RSP 080700 - no damage, no pain, no death
	self->takedamage = DAMAGE_NO;
}