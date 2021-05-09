#include "g_local.h"
#include "luc_m_bots.h"

//static int sound_attack;	// RSP 102499

void worker_sight(edict_t *self, edict_t *other);
void worker_idle (edict_t *self);


/* MRB 030199 - TONS of changes for different 'bot laser.  
 * - laser is generated at bot spawn
 * - laser 'thinks' and aligns itself with the bot when visible
 * - just one scan beam now.
 * The plan is this - tell the laser what to do (scan, fire, etc ...) 
 * laser->pos1 = current angular offset delta
 */

static char botlasername[32] = "target_laser";

#define MAX_BEAM_DIST 2048

static vec3_t max_beamang = {45,45,0};
static vec3_t min_beamang = {-45,-45,0};	// -PITCH is tilting UP
static vec3_t dlt_beamang = {60,90,0};		// really the diff in the top two ...

#define MAX_BEAMANG_DELT	5		// biggest angle change per frame

void workerbeam_decide_laser_scan(edict_t *self, float nextcheck);
void bot_workerbeam_run(edict_t *self);


/* MRB 030199 - Worker beam's laser 'off' function
 *   call to turn off the laser */
void workerbeam_laser_off(edict_t *laser)
{
	if (!laser)
		return;

	laser->enemy = NULL;
	laser->nextthink = 0;
	laser->svflags |= SVF_NOCLIENT;
	laser->think = NULL;
}

/* MRB 030199 - trace a scan line for the laser */
edict_t *workerbeam_laser_tracelaser(edict_t *laser)
{
	trace_t tr;

	if (!laser)
		return NULL;

	_ExtendWithBounds(laser->s.origin, MAX_BEAM_DIST, laser->movedir, NULL, NULL, laser->s.old_origin);
	tr = gi.trace(laser->s.origin,NULL,NULL,laser->s.old_origin,laser->teammaster,MASK_SHOT);

	if (!tr.ent)
		return NULL;

	if (tr.fraction < 1.0)
	{
		VectorCopy(tr.endpos,laser->s.old_origin);

		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_LASER_SPARKS);
			gi.WriteByte (laser->s.frame << 2);
			gi.WritePosition (tr.endpos);
			gi.WriteDir (tr.plane.normal);
			gi.WriteByte (laser->s.skinnum);
			gi.multicast (tr.endpos, MULTICAST_PVS);
		}	// MRB 032999 - add check for 'on'
		else if (!(laser->svflags & SVF_NOCLIENT)	// laser is on
					&& tr.ent->takedamage			// target is damageable
					&& (laser->s.frame > 2))		// laser is 'thick'
		{
			T_Damage (tr.ent, laser->teammaster, laser->teammaster, laser->movedir, tr.endpos, tr.plane.normal,
							skill->value, 0, DAMAGE_ENERGY, MOD_TARGET_LASER);
		}
	}

	return(tr.ent);
}

/* MRB 030199 - This routine sets the laser's position relative to the parent
 * and also decides where to scan next.  Where to scan next is a combination of
 * our last firing direction (laser->movedir) and some incremental offset.
 * That offset is stored in laser->pos1 and is re-evaluated whenever the 
 * laser goes outside our acceptable 'range'.
 * Some variation is applied so we don't keep scanning back and forth ... */
void workerbeam_laser_stick(edict_t *laser)	// SELF is the laser
{
//	edict_t	*e;	// RSP 090900 - not used
	vec3_t	v, vTmp;
	int		i;								// RSP 082200
	int		pitch_down = max_beamang[0];	// RSP 082200

	if (!laser)
		return;

	// Don't bother if master is gone ...
	// if ((laser->svflags & SVF_NOCLIENT) || !laser->teammaster)
	if (!laser->teammaster)
		return;

//	e = laser->enemy;	// RSP 090900 - not used

	// set origin
	VectorCopy(laser->teammaster->s.origin, laser->s.origin);
	laser->s.origin[2] +=16;							// up 16 ...
	AngleVectors(laser->teammaster->s.angles, v,NULL,NULL);
	VectorMA(laser->s.origin,10,v,laser->s.origin);		// and fwd 10 from master
	// NOTE that that fwd offset is pretty constant ...

	// Try to keep him from scanning the floor all the time ...
	VectorMA(laser->s.origin, 80, v, vTmp);
	vTmp[2] -= 40;

	if (gi.pointcontents(vTmp) == CONTENTS_SOLID)
		pitch_down = 5;

	// set velocity
	VectorCopy(laser->teammaster->velocity, laser->velocity);
	VectorCopy(laser->teammaster->avelocity, laser->avelocity);

	// set angles based on 'scan' angles
	// find out current offset
	VectorSubtract(laser->s.angles,laser->teammaster->s.angles,v);

	// apply 'scan delta'
	VectorAdd(v,laser->pos1,v);

	// check bounds based on MY scan ranges
	for (i = 0 ; i < 2 ; i++)
	{
		if (v[i] > 180)
			v[i] -= 360;
		if (v[i] < -180)
			v[i] += 360;

		if (v[i] > pitch_down)
		{
			v[i] = pitch_down;
			laser->pos1[i] = -(random() * MAX_BEAMANG_DELT);
		}
		else if (v[i] < min_beamang[i])
		{
			v[i] = min_beamang[i];
			laser->pos1[i] = (random() * MAX_BEAMANG_DELT);
		}
	}
	VectorAdd(laser->teammaster->s.angles, v, laser->s.angles);

	// set direction
	AngleVectors(laser->s.angles,laser->movedir,NULL,NULL);
}

// MRB 030699 - re-aim to enemy
void workerbeam_aimatenemy(edict_t *laser, edict_t *e)
{
	vec3_t	d,v;
	int		i;

	// Try some re-aiming - pick a spot on the enemy ...
	v[0] = 0;
	v[1] = (random() * (e->maxs[1] - e->mins[1])) + e->mins[1];
	//v[1] = ((rand() + 1) % 2 ? e->maxs[1] : e->mins[1]);
	v[2] = (random() * (e->maxs[2] - e->mins[2])) + e->mins[2];
	//v[2] = ((rand() + 1) % 2 ? e->maxs[2] : e->mins[2]);

	// ... then position the plane perpendicular to my 'facing dir' ...
	d[0] = e->s.origin[0] - (laser->movedir[1] * v[1]);
	d[1] = e->s.origin[1] + (laser->movedir[0] * v[1]);
	d[2] = e->s.origin[2] + v[2];

	// ... then find that angle
	VectorSubtract(d,laser->s.origin, v);
	vectoangles(v,d);

	// now check the difference
	VectorSubtract(laser->s.angles,d,v);
			
	// Generate some 'difference angles' for next time
	for (i = 0 ; i < 2 ; i++)
	{
		if (v[i] > 180)
			v[i] -= 360;
		if (v[i] < -180)
			v[i] += 360;

		if (v[i] > 0)
			if (v[i] > MAX_BEAMANG_DELT)
				laser->pos1[i] = -MAX_BEAMANG_DELT;
			else
				laser->pos1[i] = -v[i];
		else
			if (v[i] < -MAX_BEAMANG_DELT)
				laser->pos1[i] = MAX_BEAMANG_DELT;
			else
				laser->pos1[i] = -v[i];
	}
}

// MRB 030199 - This routine sets the laser scanning for targets
void workerbeam_laser_think_scan(edict_t *laser)
{
	edict_t *target = NULL;

	// This should not be necessary EACH FRAME ...
	workerbeam_laser_stick(laser);

	// If we've been scanning long enough, turn off the scan laser
	if (level.time > laser->touch_debounce_time)
	{
		workerbeam_laser_off(laser);
		return;
	}

	// do graphic line - if we happen to hit something that might be an enemy
	// start running and allow the other code to decide whether to attack it or not
	target = workerbeam_laser_tracelaser(laser);

	if (laser->enemy && (target != laser->enemy))
	{	// we are looking for something ... at least go toward it 
		workerbeam_aimatenemy(laser,laser->enemy);
	}
	else 
		if (laser->teammaster && target->client	&&						// random scanning hit a player
			!(laser->teammaster->monsterinfo.aiflags & AI_PACIFIST) &&	// and we are not in pacifist mode
			!laser->teammaster->deadflag)
		{
			bot_workerbeam_run(laser->teammaster);
		}

	laser->nextthink = level.time + FRAMETIME;
}

mmove_t bot_workerbeam_attack_gun;

// MRB 032999 - a toggle to keep him from totally destroying the player
void workerbeam_laser_toggle_fire(edict_t *laser)
{
	if (level.time > laser->pain_debounce_time)
	{
		if (laser->svflags & SVF_NOCLIENT)
		{	// toggle on
			laser->svflags &= ~SVF_NOCLIENT;
			laser->pain_debounce_time = level.time + (FRAMETIME * 10);
			gi.sound (laser, CHAN_VOICE,gi.soundindex ("worker/atakbeam.wav"), 1, ATTN_NORM, 0);	// RSP 102499
		}
		else
		{	// toggle off for a shorter time
			laser->svflags |= SVF_NOCLIENT;
			laser->pain_debounce_time = level.time + (FRAMETIME * 5);
		}
	}
}

/* MRB 030199 - This routine sets the laser firing colors and
 * also controls the direction if we are scanning for a target - this
 * allows it to move and not stray from the target.  This also allows
 * it to sometimes hit you if you are hiding behind something.
 */
void workerbeam_laser_think_fire(edict_t *laser)
{
	edict_t *target = NULL;

	if (!laser->teammaster || !laser->enemy)
	{	// something happened- shut it down.
		workerbeam_laser_off(laser);
		return;
	}

	laser->s.skinnum = 0xf0f0f0f0;
	workerbeam_laser_stick(laser);

	// MRB 032999 - toggle laser on and off to prevent too much damage ...
	workerbeam_laser_toggle_fire(laser);

	/* Here we fired at the target - if we didn't hit, make a note of it
	 * and try to retarget.   If we miss too many times (or we get out of range 
	 * just give up
	 */
	if ((target = workerbeam_laser_tracelaser(laser)) != laser->enemy)
	{
		if (++(laser->count) > 20)
		{	// we missed too many times, go back down to scanning
			workerbeam_decide_laser_scan(laser->teammaster, 20);
			laser->count = -(6 - skill->value);
			laser->enemy = NULL;
			return;
		}

		workerbeam_aimatenemy(laser,laser->enemy);
	}
	else
	{
		VectorClear(laser->pos1);
		laser->count = 0;
	}

	laser->nextthink = level.time + FRAMETIME;
}

//---- END of laser specific code
//---- START of bot-specific code

// MRB 030199 - Workerbeam's firing setup function - very little movement here
void workerbeam_decide_laser_fire(edict_t *self)
{
	// turn on
	// MRB 033999 - set up toggling flag (if need to)
	workerbeam_laser_toggle_fire(self->teamchain);

	// set enemy
	self->teamchain->enemy = self->enemy;

	// set width
	self->teamchain->s.frame = 4;
	self->teamchain->s.skinnum = 0xf0f0f0f0;
	VectorClear(self->teamchain->pos1);

	// Set up thinking
	self->teamchain->think = workerbeam_laser_think_fire;
	self->teamchain->nextthink = level.time + FRAMETIME;
}

// MRB 030199 - Workerbeam's scanning function - decide whether to scan or not
void workerbeam_decide_laser_scan(edict_t *self, float nextcheck)
{
	int i = (rand() + 1) % 2;

	self->teamchain->think = workerbeam_laser_think_scan;
	self->teamchain->s.skinnum = 0xd0d1c0c1;
	self->teamchain->s.frame = 2;

	if (!self->enemy && (i > 0))
	{
		self->teamchain->nextthink = 0;
		return;
	}
		
	// Set up thinking
	self->teamchain->touch_debounce_time = level.time + (FRAMETIME * nextcheck);
	self->teamchain->nextthink = level.time + FRAMETIME;

	// ON and OFF is defined by SVF_NOCLIENT which makes the laser invisible to all
	// clients when it is set.  The laser just isn't drawn in that case.
	if (self->teamchain->svflags & SVF_NOCLIENT)
	{	// we are NOT on, so turn it on
		edict_t *z = self->teamchain;

		z->svflags &= ~SVF_NOCLIENT;
		
		// set up a good 'random' start point
		z->s.angles[PITCH] = self->s.angles[PITCH] + (random() * dlt_beamang[PITCH]) + min_beamang[PITCH]/2;
		z->s.angles[YAW]   = self->s.angles[YAW] + (random() * dlt_beamang[YAW]) + min_beamang[YAW]/2;
		z->s.angles[ROLL]  = self->s.angles[2];

		// set up a good 'random' delta
		z->pos1[PITCH] = (random() * (MAX_BEAMANG_DELT * 2)) - MAX_BEAMANG_DELT;
		z->pos1[YAW]   = (random() * (MAX_BEAMANG_DELT * 2)) - MAX_BEAMANG_DELT;
		z->pos1[ROLL]  = 0;
		
		workerbeam_laser_stick(z);
	}
}

// Local functions

void bot_beam_decide_gather (edict_t *self);

/* MRB 030199 - Bot tells the laser to atack, and then just sits back and waits.
 */
void bot_workerbeam_attackmore (edict_t *self)
{
	vec3_t v;

  	VectorMA(self->enemy->absmin,0.5,self->enemy->size,v);

	if (!visible(self, self->enemy)		||		// if no longer visible ...
		(self->teamchain->count < 0)	||	// or we've missed a lot 
		!botEvalTarget(self, v,MAX_BEAM_DIST,	// or out of range
				max_beamang[PITCH],min_beamang[PITCH],max_beamang[YAW]/3))
	{	// go back to scanning ...
		workerbeam_decide_laser_scan(self,15);
		return;
	}

	workerbeam_decide_laser_fire(self);
	self->monsterinfo.nextframe = FRAME_STAND_START;
}

// For attacking
mframe_t bot_workerbeam_frames_attack_gun [] =
{
	NULL, 0, NULL,	// 1
	NULL, 0, NULL,
	NULL, 0, NULL,
	NULL, 0, NULL,
	NULL, 0, NULL,
	NULL, 0, bot_workerbeam_attackmore	// 6
};

mmove_t bot_workerbeam_attack_gun = {FRAME_STAND_START, FRAME_STAND_START+5, 
					bot_workerbeam_frames_attack_gun, bot_run };

/* MRB 030199 - laser and bot think separately
 * This allows the bot to roam and the laser to do the scanning around.  Some
 *  syncronizing is necessary when attacking, but for the most part they 
 *  behave indepentantly.
 * During attack, the laser keeps a value in laser->count.  If this is 0
 *  it means that it just hit its target (which is stored in laser->enemy
 *  after the bot enters the attack frames).  If the value is positive it means
 *  that we just missed the enemy - each successive FARMETIME w/ a miss increments
 *  that value - if the laser is not attacking, that value is less than 0.
 */
void bot_workerbeam_attack(edict_t *self)
{
	vec3_t v;

	// Check for attack
	VectorMA(self->enemy->absmin,0.5,self->enemy->size,v);

	if (self->teamchain->enemy)			// If we're presently angry at someone ...
	{
		if (self->teamchain->count > 10 && 				// whom we are NOT hitting, and
			!botEvalTarget(self, v,MAX_BEAM_DIST,		// if the target isn't in range
				max_beamang[PITCH],min_beamang[PITCH],max_beamang[YAW]/2))  // (xtra narrow check)
		{
			workerbeam_decide_laser_scan(self,15);
			bot_run(self);// just run 
			return;
		}

	}
	else
	{
		if (!botEvalTarget(self, v,MAX_BEAM_DIST,		// if the target isn't in range
				max_beamang[PITCH],min_beamang[PITCH],max_beamang[YAW]/2))  // (xtra narrow check)
		{
			workerbeam_decide_laser_scan(self,15);
			bot_run(self);// just run 
			return;
		}
	}

	// MRB 030199 - give my laser MY enemy to track
	self->teamchain->enemy = self->enemy;
	workerbeam_decide_laser_fire(self);

	self->monsterinfo.currentmove = &bot_workerbeam_attack_gun;
}


// AJA 19980906 - "Gather resources" frames & behaviour.
mframe_t bot_beam_frames_gather [] =
{
	ai_stand, 0, NULL, // 1
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL, // 7
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL, // 13
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL // 18
};
mmove_t bot_beam_gather = {FRAME_STAND_START, FRAME_STAND_END, bot_beam_frames_gather, bot_beam_decide_gather};

void bot_beam_start_gather (edict_t *self)
{
	self->monsterinfo.currentmove = &bot_beam_gather;
}

void bot_beam_decide_gather (edict_t *self)
{
	// Stay in the gathering state unless hurt (which will be automatic).
	self->teamchain->enemy = NULL;
	bot_beam_start_gather(self);

	workerbeam_decide_laser_scan(self,20);
	// NOTE: should turn this off in pain somehow...
}

//MRB 030199 - Ultra-thin wrappers for 'real' bot code to better control laser
void bot_workerbeam_stand (edict_t *self)
{	// when standing, you can scan ...
	
	workerbeam_decide_laser_scan(self,20);

	// We should never be standing with an enemy.
	// RSP 030599: Commented out for now. Was causing crashes further along.
//	if (self->enemy)
//		self->enemy = NULL;

	if (self->monsterinfo.aiflags & AI_PACIFIST)
		bot_beam_start_gather(self);
	else
		bot_stand(self);
}

void bot_workerbeam_run (edict_t *self)
{	
	if (!self->enemy)
	{	// when running, you can scan if you have no enemy
		workerbeam_decide_laser_scan(self,15);
		bot_run(self);
	}

	// if you have an enemy, and the laser has recently NOT hit it lately, RUN
	else if (self->teamchain && (self->teamchain->count > 10))
		bot_run(self);

	// laser is gettin' busy - stand and attack
	else
		bot_workerbeam_attack(self);
}

void bot_workerbeam_walk (edict_t *self)
{	// when walking, you can scan ...
	if (!self->enemy)
		workerbeam_decide_laser_scan(self,15);

	bot_walk(self);
}

void bot_workerbeam_pain  (edict_t *self, edict_t *other, float kick, int damage)
{	// when in pain, STOP

	self->teamchain->enemy = NULL;
	workerbeam_laser_off(self->teamchain);
	bot_pain ( self, other,  kick,  damage);
}

void bot_workerbeam_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{	// when dying, free laser and STOP

	if (self->teamchain)
		G_FreeEdict(self->teamchain);

	bot_die(self, inflictor, attacker, damage, point);
}

/*QUAKED monster_bot_workerbeam (1 .5 0) (-24 -24 0) (24 24 32) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_bot_workerbeam (edict_t *self)
{
	// Using self->pos1 and self->pos2 as start,endpoints for beams and whatnot
	
	self->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Beam.md2");			// RSP 061899

	self->health = hp_table[HP_BEAMBOT];	// RSP 032399

	bot_spawnsetup(self);

	// MRB 030199 - ALL functions call locally first
	self->monsterinfo.stand  = bot_workerbeam_stand;
	self->monsterinfo.walk   = bot_workerbeam_walk;
	self->monsterinfo.run    = bot_workerbeam_run;
	self->monsterinfo.attack = bot_workerbeam_attack;

	self->identity = ID_BEAMBOT;			// RSP 072900

	// MRB 030199 - overwrite pain and die ...
	self->pain = bot_workerbeam_pain;
	self->die = bot_workerbeam_die;

	self->monsterinfo.scale = MODEL_SCALE;

	// MRB 030199 - do laser as an actual tag-along entity
	{	// Now give me a little laser friend
		edict_t *l = G_Spawn();

		l->flags |= (FL_MASTER_EXT|FL_PASSENGER);		// Cause it to think immediately after its master
		l->movetype = MOVETYPE_FLY;
		l->solid = SOLID_NOT;
		l->s.renderfx |= (RF_BEAM|RF_TRANSLUCENT);
		l->s.modelindex = 1;			// must be non-zero
		l->spawnflags = SPAWNFLAG_TLAS_START_ON;

		l->svflags = SVF_NOCLIENT;
		l->s.frame = 2;					// width of 2 for start (thin)
		l->s.skinnum = 0xd0d1c0c1;		//greenish w/flicker

		// set origin just up from the bot - do the 'id' thing and assume no pitch/yaw
		l->s.origin[0] = self->s.origin[0];
		l->s.origin[1] = self->s.origin[1];
		l->s.origin[2] = self->s.origin[2] + 16;

		VectorCopy(self->s.angles,l->s.angles);

		VectorSet(l->mins,-8,-8,-8);
		VectorSet(l->maxs,8,8,8);

		l->classname = botlasername;
		l->dmg = 1;
		self->teamchain = l;
		l->teammaster = self;

		gi.linkentity(l);
	}

	/* AJA 19980906 - Passive flag. This will mean that the bot will gather
	    resources instead of waiting to attack a player. The bot will not
	    react to the player unless the player harms it.
	   MRB 19981205 - walkmonster_start flags AI_PACIFIST, but we need to
	    feed it a currentmove to be consistant
	*/
	if (self->spawnflags & SPAWNFLAG_PACIFIST)
		self->monsterinfo.currentmove = &bot_beam_gather;
	else
		self->monsterinfo.currentmove = &bot_move_stand;

	gi.linkentity (self);
	walkmonster_start (self);
}

