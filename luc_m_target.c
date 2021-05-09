//  luc_m_target.c - A monster with no model, used in the endgame
//  Initial module revision - RSP 100800

#include "g_local.h"


void MonsterTarget_Think (edict_t *self)
{
	self->enemy = level.sight_client;	// Latch onto the player
	self->nextthink = level.time + 10*FRAMETIME;	// Think once per second
}

void MonsterTarget_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
}

void MonsterTarget_Pain (edict_t *self, edict_t *other, float kick, int damage)
{
	gi.sound (self, CHAN_BODY, gi.soundindex("world/spark6.wav"), 1, ATTN_NORM, 0);
}

void MonsterTarget_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_UseTargets (self, attacker);
	SpawnExplosion(self,TE_GRENADE_EXPLOSION,MULTICAST_PHS,EXPLODE_GRENADE);
}

/*QUAKED monster_target (1 .5 0) (-4 -4 -4) (4 4 4) Ambush Trigger_Spawn Sight Pacifist

This monster has no model. It's just an entity that will make the player its
enemy if it sees him, and is useful in the endgame to get the dreadlock to fire at
certain places. Place this monster at the location you want the dreadlock to fire.
*/
void SP_monster_target (edict_t *self)
{
	VectorSet (self->mins, -4, -4, -4);
	VectorSet (self->maxs, 4, 4, 4);
	self->s.modelindex = 1;	// must be non-zero

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->svflags = (SVF_NOCLIENT|SVF_MONSTER);		// invisible monster
	self->flags |= FL_SPARKDAMAGE;					// generate sparks, not blood
	self->health = 25;
	self->max_health = 25;
	self->takedamage = DAMAGE_YES;
	self->think = MonsterTarget_Think;
	self->touch = MonsterTarget_Touch;
	self->pain  = MonsterTarget_Pain;
	self->die   = MonsterTarget_Die;
	self->nextthink = level.time + 10*FRAMETIME;
	gi.linkentity (self);
}
