#include "g_local.h"

// WPO 011199
//======================================================
// returns true is entity is on the ground
//======================================================

qboolean G_CheckGround(edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->velocity[2] > 100)
		return false;

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
		return false;

	return true;
}

//======================================================
// Returns distance between 2 entities
//======================================================

float G_EntDist(edict_t *ent1, edict_t *ent2)
{
	vec3_t	vec;

	VectorSubtract(ent1->s.origin, ent2->s.origin, vec);
	return VectorLength(vec);
}

//======================================================
// True if Ent is valid, has client, and edict_t inuse.
//======================================================

qboolean G_EntExists(edict_t *ent) 
{
	return ((ent) && (ent->client) && (ent->inuse));
}

//======================================================
// True if ent is not DEAD or DEAD or DEAD (and BURIED!)
//======================================================

qboolean G_ClientNotDead(edict_t *ent) 
{
	qboolean buried = true;
	qboolean b1=ent->client->ps.pmove.pm_type != PM_DEAD;
	qboolean b2=ent->deadflag != DEAD_DEAD;
	qboolean b3=ent->health > 0;

	return (b3||b2||b1) && (buried);
}

//======================================================
// True if ent is not DEAD
//======================================================

qboolean G_ClientInGame(edict_t *ent) 
{
	if (!G_EntExists(ent)) 
		return false;

	if (!G_ClientNotDead(ent)) 
		return false;

	return true;
}

//======================================================
//========== Spawn Temp Entity Functions ===============
//======================================================

/*
Spawns (type) Splash with {count} particles of {color} at {start} moving
in {direction} and Broadcasts to all in Potentially Visible Set from
vector (origin)

TE_LASER_SPARKS - Splash particles obey gravity
TE_WELDING_SPARKS - Splash particles with flash of light at {origin}
TE_SPLASH - Randomly shaded shower of particles
TE_WIDOWSPLASH - New to v3.20

colors:

1 - red/gold - blaster type sparks
2 - blue/white - blue
3 - brown - brown
4 - green/white - slime green
5 - red/orange - lava red
6 - red - blood red

All others are grey
*/

void G_Spawn_Splash(int type, int count, int color, vec3_t start, vec3_t movdir, vec3_t origin) 
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WriteByte(count);
	gi.WritePosition(start);
	gi.WriteDir(movdir);
	gi.WriteByte(color);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
Spawns a string of successive (type) models of from record (rec_no)
from (start) to (endpos) which are offset by vector (offset) and
Broadcasts to all in Potentially Visible Set from vector (origin)

Type:

TE_GRAPPLE_CABLE - The grappling hook cable
TE_MEDIC_CABLE_ATTACK - NOT IMPLEMENTED IN ENGINE
TE_PARASITE_ATTACK - NOT IMPLEMENTED IN ENGINE
*/

void G_Spawn_Models(int type, short rec_no, vec3_t start, vec3_t endpos, vec3_t offset, vec3_t origin) 
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WriteShort(rec_no);
	gi.WritePosition(start);
	gi.WritePosition(endpos);
	gi.WritePosition(offset);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
Spawns a trail of (type) from {start} to {end} and Broadcasts to all
in Potentially Visible Set from vector (origin)

TE_BFG_LASER - Spawns a green laser
TE_BUBBLETRAIL - Spawns a trail of bubbles
TE_PLASMATRAIL - NOT IMPLEMENTED IN ENGINE
TE_RAILTRAIL - Spawns a blue spiral trail filled with white smoke
*/

void G_Spawn_Trails(int type, vec3_t start, vec3_t endpos, vec3_t origin) 
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(start);
	gi.WritePosition(endpos);
	gi.multicast(origin, MULTICAST_PVS);
}


/*
Spawns sparks of (type) from {start} in direction of {movdir} and
Broadcasts to all in Potentially Visible Set from vector (origin)

TE_BLASTER - Spawns a blaster sparks
TE_BLOOD - Spawns a spurt of red blood
TE_BULLET_SPARKS - Same as TE_SPARKS, with a bullet puff and richochet sound
TE_GREENBLOOD - NOT IMPLEMENTED - Spawns a spurt of green blood
TE_GUNSHOT - Spawns a grey splash of particles, with a bullet puff
TE_SCREEN_SPARKS - Spawns a large green/white splash of sparks
TE_SHIELD_SPARKS - Spawns a large blue/violet splash of sparks
TE_SHOTGUN - Spawns a small grey splash of spark particles, with a bullet puff
TE_SPARKS - Spawns a red/gold splash of spark particles
TE_BLASTER2 - New to v3.20
TE_MOREBLOOD - New to v3.20
TE_CHAINFIST_SMOKE - New to v3.20
TE_TUNNEL_SPARKS - New to v3.20
TE_ELECTRIC_SPARKS - New to v3.20
TE_HEATBEAM_SPARKS - New to v3.20
TE_BLUEHYPERBLASTER - New to v3.20
*/

void G_Spawn_Sparks(int type, vec3_t start, vec3_t movdir, vec3_t origin) 
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(start);
	gi.WriteDir(movdir);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
Spawns a (type) explosion at (start} and Broadcasts to all Potentially
Visible Sets from {origin}

TE_BFG_BIGEXPLOSION - Spawns a BFG particle explosion
TE_BFG_EXPLOSION - Spawns a BFG explosion sprite
TE_BOSSTPORT - Spawns a mushroom-cloud particle effect
TE_EXPLOSION1 - Spawns a mid-air-style explosion
TE_EXPLOSION2 - Spawns a nuclear-style explosion
TE_GRENADE_EXPLOSION - Spawns a grenade explosion
TE_GRENADE_EXPLOSION_WATER - Spawns an underwater grenade explosion
TE_ROCKET_EXPLOSION - Spawns a rocket explosion
TE_ROCKET_EXPLOSION_WATER - Spawns an underwater rocket explosion

Note: The last four EXPLOSION entries overlap to some degree.
TE_GRENADE_EXPLOSION is the same as TE_EXPLOSION2,
TE_ROCKET_EXPLOSION is the same as TE_EXPLOSION1,
and both of the EXPLOSION_WATER entries are the same, visually.

TE_NUKEBLAST - New to v3.20
TE_EXPLOSION1_BIG - New to v3.20
TE_EXPLOSION1_NP - New to v3.20
TE_PLAIN_EXPLOSION - New to v3.20
TE_PLASMA_EXPLOSION - New to v3.20
TE_TRACKER_EXPLOSION - New to v3.20
*/

// RSP 072100 - More accurate positioning of shot

void R_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result)
{
    result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] + up[0] * distance[2];
    result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] + up[1] * distance[2];
    result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + up[2] * distance[2];
}

void G_Spawn_Explosion(int type, vec3_t start, vec3_t origin) 
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(start);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
   generic function to spew a little debris
   the model parameters are the debris models to use
*/
void G_Make_Debris(edict_t *ent, char *model1, char *model2, char *model3)
{
	ThrowDebris(ent, model3, 3.75, ent->s.origin);
	ThrowDebris(ent, model3, 2.50, ent->s.origin);
	ThrowDebris(ent, model2, 4.60, ent->s.origin);
	ThrowDebris(ent, model2, 1.50, ent->s.origin);
	ThrowDebris(ent, model1, 2.30, ent->s.origin);
	ThrowDebris(ent, model1, 4.50, ent->s.origin);
}

// Returns true if any part of the entity is over 'contents' (i.e. CONTENTS_LAVA)

qboolean G_ContentsCheck (edict_t *ent, int contents)
{
	vec3_t	mins, maxs, start;
	int	x, y;
	vec3_t newOrigin;
	
	VectorCopy(ent->s.origin, newOrigin);

	newOrigin[2] -= 8;

	VectorAdd (newOrigin, ent->mins, mins);
	VectorAdd (newOrigin, ent->maxs, maxs);

	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for (x = 0; x <= 1 ; x++)
		for (y = 0; y <= 1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (gi.pointcontents(start) == contents)
				return true;
		}

	return false;	
}


// WPO 100699 Doesn't appear to be needed, we'll see.
// If it ends up this is needed, dump this code in favor
// of a modified G_TouchTriggers, but just expand the
// bounding box of the dreadlock for the call to gi.BoxEdicts()
#if 0
void Flyer_TouchTriggers(edict_t *flyer, float radius)
{
	edict_t	*hit = NULL;

	while ((hit = findradius(hit, flyer->s.origin, radius)) != NULL)
	{
		if (hit->solid != SOLID_TRIGGER)
			continue;
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;

		if (!Q_stricmp(hit->classname, "waypoint"))
			continue;

		hit->touch(hit, flyer, NULL, NULL);
	}
}
#endif

#if 1

// RSP 080300 - general dump of entity field values

void Dump_Entity(edict_t *self)
{
	gclient_t	*cl;
	int			i;

	// Dump the entity state

	printf("********************* %5.2f %s **************************\n",level.time,self->classname);
	printf("s.number %d\n",self->s.number);
	printf("s.origin %s\n",vtos(self->s.origin));
	printf("s.angles %s\n",vtos(self->s.angles));
	printf("s.old_origin %s\n",vtos(self->s.old_origin));
	printf("s.modelindex %d\n",self->s.modelindex);
	printf("s.modelindex2 %d\n",self->s.modelindex2);
	printf("s.modelindex3 %d\n",self->s.modelindex3);
	printf("s.modelindex4 %d\n",self->s.modelindex4);
	printf("s.frame %d\n",self->s.frame);
	printf("s.skinnum %x\n",self->s.skinnum);
	printf("s.effects %x\n",self->s.effects);
	printf("s.renderfx %x\n",self->s.renderfx);
	printf("s.solid %x\n",self->s.solid);
	printf("s.sound %d\n",self->s.sound);
	printf("s.event %d\n",self->s.event);
	printf("inuse %d\n",self->inuse);
	printf("linkcount %d\n",self->linkcount);
	printf("svflags %x\n",self->svflags);
	printf("mins %s\n",vtos(self->mins));
	printf("maxs %s\n",vtos(self->maxs));
	printf("absmin %s\n",vtos(self->absmin));
	printf("absmax %s\n",vtos(self->absmax));
	printf("size %s\n",vtos(self->size));
	printf("solid %d\n",self->solid);
	printf("clipmask %x\n",self->clipmask);
	printf("owner %s\n",self->owner ? self->owner->classname : "NULL");
	printf("movetype %d\n",self->movetype);
	printf("flags %x\n",self->flags);
	printf("model %s\n",self->model ? self->model : "NULL");
	printf("freetime %5.2f\n",self->freetime);
	printf("message %s\n",self->message ? self->message : "NULL");
	printf("classname %s\n",self->classname);
	printf("spawnflags %x\n",self->spawnflags);
	printf("timestamp %5.2f\n",self->timestamp);
	printf("angle %5.2f\n",self->angle);
	printf("target %s\n",self->target ? self->target : "NULL");
	printf("targetname %s\n",self->targetname ? self->targetname : "NULL");
	printf("killtarget %s\n",self->killtarget ? self->killtarget : "NULL");
	printf("team %s\n",self->team ? self->team : "NULL");
	printf("pathtarget %s\n",self->pathtarget ? self->pathtarget : "NULL");
	printf("deathtarget %s\n",self->deathtarget ? self->deathtarget : "NULL");
	printf("combattarget %s\n",self->combattarget ? self->combattarget : "NULL");
	printf("target_ent %s\n",self->target_ent ? self->target_ent->classname : "NULL");
	printf("cell %s\n",self->cell ? self->cell->classname : "NULL");
	printf("speed %5.2f\n",self->speed);
	printf("accel %5.2f\n",self->accel);
	printf("decel %5.2f\n",self->decel);
	printf("movedir %s\n",vtos(self->movedir));
	printf("pos1 %s\n",vtos(self->pos1));
	printf("pos2 %s\n",vtos(self->pos2));
	printf("pos3 %s\n",vtos(self->pos3));
	printf("velocity %s\n",vtos(self->velocity));
	printf("avelocity %s\n",vtos(self->avelocity));
	printf("mass %d\n",self->mass);
	printf("air_finished %5.2f\n",self->air_finished);
	printf("gravity %5.2f\n",self->gravity);
	printf("goalentity %s\n",self->goalentity ? self->goalentity->classname : "NULL");
	printf("movetarget %s\n",self->movetarget ? self->movetarget->classname : "NULL");
	printf("yaw_speed %5.2f\n",self->yaw_speed);
	printf("ideal_yaw %5.2f\n",self->ideal_yaw);
	printf("nextthink %5.2f\n",self->nextthink);
	printf("prethink is %s\n",self->prethink ? "set" : "not set");
	printf("think is %s\n",self->think ? "set" : "not set");
	printf("blocked is %s\n",self->blocked ? "set" : "not set");
	printf("touch is %s\n",self->touch ? "set" : "not set");
	printf("use is %s\n",self->use ? "set" : "not set");
	printf("pain is %s\n",self->pain ? "set" : "not set");
	printf("die is %s\n",self->die ? "set" : "not set");
	printf("touch_debounce_time %5.2f\n",self->touch_debounce_time);
	printf("pain_debounce_time %5.2f\n",self->pain_debounce_time);
	printf("damage_debounce_time %5.2f\n",self->damage_debounce_time);
	printf("fly_sound_debounce_time %5.2f\n",self->fly_sound_debounce_time);
	printf("last_move_time %5.2f\n",self->last_move_time);
	printf("health %d\n",self->health);
	printf("max_health %d\n",self->max_health);
	printf("gib_health %d\n",self->gib_health);
	printf("deadflag %d\n",self->deadflag);
	printf("show_hostile %d\n",self->show_hostile);
	printf("powerarmor_time %5.2f\n",self->powerarmor_time);
	printf("map %s\n",self->map ? self->map : "NULL");
	printf("viewheight %d\n",self->viewheight);
	printf("takedamage %d\n",self->takedamage);
	printf("dmg %d\n",self->dmg);
	printf("radius_dmg %d\n",self->radius_dmg);
	printf("dmg_radius %5.2f\n",self->dmg_radius);
	printf("sounds %d\n",self->sounds);
	printf("count %d\n",self->count);
	printf("enemy %s\n",self->enemy ? self->enemy->classname : "NULL");
	printf("oldenemy %s\n",self->oldenemy ? self->oldenemy->classname : "NULL");
	printf("activator %s\n",self->activator ? self->activator->classname : "NULL");
	printf("groundentity %s\n",self->groundentity ? self->groundentity->classname : "NULL");
	printf("groundentity_linkcount %d\n",self->groundentity_linkcount);
	printf("noise_index %d\n",self->noise_index);
	printf("noise_index2 %d\n",self->noise_index2);
	printf("volume %5.2f\n",self->volume);
	printf("attenuation %5.2f\n",self->attenuation);
	printf("wait %5.2f\n",self->wait);
	printf("delay %5.2f\n",self->delay);
	printf("random %5.2f\n",self->random);
	printf("teleport_time %5.2f\n",self->teleport_time);
	printf("watertype %d\n",self->watertype);
	printf("waterlevel %d\n",self->waterlevel);
	printf("move_origin %s\n",vtos(self->move_origin));
//	printf("move_angles %s\n",vtos(self->move_angles));	// RSP 100200 - not used
	printf("light_level %d\n",self->light_level);
	printf("style %d\n",self->style);
	printf("item %s\n",self->item ? self->item->classname : "NULL");
	printf("monsterinfo.aiflags %x\n",self->monsterinfo.aiflags);
	printf("radius %5.2f\n",self->radius);
	printf("name %s\n",self->name ? self->name : "NULL");
	printf("z_offset %d\n",self->z_offset);
	printf("locked_to %s\n",self->locked_to ? self->locked_to->classname : "NULL");
	printf("turn_direction %d\n",self->turn_direction);
	printf("count2 %d\n",self->count2);
	printf("attack %s\n",self->attack ? self->attack : "NULL");
	printf("retreat %s\n",self->retreat ? self->retreat : "NULL");
	printf("stopthink %5.2f\n",self->stopthink);
	printf("model2 %s\n",self->model2 ? self->model2 : "NULL");
	printf("model3 %s\n",self->model3 ? self->model3 : "NULL");
	printf("identity %d\n",self->identity);
	printf("curpath %s\n",self->curpath? self->curpath->classname : "NULL");
	printf("dlockOwner %s\n",self->dlockOwner? self->dlockOwner->classname : "NULL");
	printf("state %d\n",self->state);
	printf("decoy %s\n",self->decoy ? self->decoy->classname : "NULL");
	printf("decoy_animation %d\n",self->decoy_animation);
	printf("freeze_framenum %5.2f\n",self->freeze_framenum);
	printf("teleportgun_victim %s\n",self->teleportgun_victim ? self->teleportgun_victim->classname : "NULL");
	printf("teleportgun_framenum %5.2f\n",self->teleportgun_framenum);             
	printf("tportding_framenum %5.2f\n",self->tportding_framenum);		// RSP 090100 next frame to sound teleportgun storage sound

	// If this is a client, dump the client fields
	if (!self->client)
		return;

	printf("--- client ---\n");
	cl = self->client;
	for (i = 0 ; i < 3 ; i++)
		printf("ps.move.delta_angles (%5.2f %5.2f %5.2f)\n",cl->ps.pmove.delta_angles[0],cl->ps.pmove.delta_angles[1],cl->ps.pmove.delta_angles[2]);
	printf("resp.cmd_angles %s\n",vtos(cl->resp.cmd_angles));
	printf("ps.viewangles %s\n",vtos(cl->ps.viewangles));
	printf("oldviewangles %s\n",vtos(cl->oldviewangles));
	printf("v_angle %s\n",vtos(cl->v_angle));

	printf("showscores is %s\n",cl->showscores ? "set" : "not set");
	printf("showinventory is %s\n",cl->showinventory ? "set" : "not set");
	printf("showhelp is %s\n",cl->showhelp ? "set" : "not set");
	printf("showhelpicon is %s\n",cl->showhelpicon ? "set" : "not set");
	printf("ammo_index %d\n",cl->ammo_index);
	printf("buttons %d\n",cl->buttons);
	printf("oldbuttons %d\n",cl->oldbuttons);
	printf("latched_buttons %d\n",cl->latched_buttons);
	printf("weapon_thunk is %s\n",cl->weapon_thunk ? "set" : "not set");
	printf("newweapon %s\n",cl->newweapon? cl->newweapon->classname : "NULL");
	printf("damage_armor %d\n",cl->damage_armor);
	printf("damage_parmor %d\n",cl->damage_parmor);
	printf("damage_blood %d\n",cl->damage_blood);
	printf("damage_knockback %d\n",cl->damage_knockback);
	printf("damage_from %s\n",vtos(cl->damage_from));
	printf("killer_yaw %5.2f\n",cl->killer_yaw);
	printf("kick_angles %s\n",vtos(cl->kick_angles));
	printf("kick_origin %s\n",vtos(cl->kick_origin));
	printf("v_dmg_roll %5.2f\n",cl->v_dmg_roll);
	printf("v_dmg_pitch %5.2f\n",cl->v_dmg_pitch);
	printf("v_dmg_time %5.2f\n",cl->v_dmg_time);
	printf("fall_time %5.2f\n",cl->fall_time);
	printf("fall_value %5.2f\n",cl->fall_value);
	printf("damage_alpha %5.2f\n",cl->damage_alpha);
	printf("bonus_alpha %5.2f\n",cl->bonus_alpha);
	printf("damage_blend %s\n",vtos(cl->damage_blend));
	printf("bobtime %5.2f\n",cl->bobtime);
	printf("oldvelocity %s\n",vtos(cl->oldvelocity));
	printf("next_drown_time %5.2f\n",cl->next_drown_time);
	printf("old_waterlevel %d\n",cl->old_waterlevel);
	printf("machinegun_shots %d\n",cl->machinegun_shots);
	printf("anim_end %d\n",cl->anim_end);
	printf("anim_priority %d\n",cl->anim_priority);
	printf("anim_duck is %s\n",cl->anim_duck ? "set" : "not set");
	printf("anim_run is %s\n",cl->anim_run ? "set" : "not set");
	printf("invincible_framenum %5.2f\n",cl->invincible_framenum);
	printf("quad_framenum %5.2f\n",cl->quad_framenum);
	printf("superspeed_framenum %5.2f\n",cl->superspeed_framenum);
	printf("PlayerSpeed %5.2f\n",cl->PlayerSpeed);
	printf("lavaboots_framenum %5.2f\n",cl->lavaboots_framenum);
	printf("cloak_framenum %5.2f\n",cl->cloak_framenum);
	printf("vampire_framenum %5.2f\n",cl->vampire_framenum);
	printf("tri_framenum %5.2f\n",cl->tri_framenum);
	printf("decoy_timeleft %5.2f\n",cl->decoy_timeleft);
	printf("scenario_framenum %d\n",cl->scenario_framenum);
	printf("capture_framenum %d\n",cl->capture_framenum);
	printf("implant_framenum %d\n",cl->implant_framenum);
	printf("grenade_blew_up is %s\n",cl->grenade_blew_up ? "set" : "not set");
	printf("grenade_time %5.2f\n",cl->grenade_time);
	printf("weapon_sound %d\n",cl->weapon_sound);
	printf("pickup_msg_time %5.2f\n",cl->pickup_msg_time);
	printf("flood_locktill %5.2f\n",cl->flood_locktill);
	printf("respawn_time %5.2f\n",cl->respawn_time);
	printf("scuba_next_breathe %d\n",cl->scuba_next_breathe);
	printf("scuba_breathe_sound %d\n",cl->scuba_breathe_sound);
	printf("teleport_stored is %s\n",cl->teleport_stored ? "set" : "not set");
	printf("teleport_angles %s\n",vtos(cl->teleport_angles));
	printf("teleport_origin %s\n",vtos(cl->teleport_origin));

	printf("--- pmove_t ---\n");
	printf("ps.pmove.delta_angles is (%5.2f %5.2f %5.2f)\n",cl->ps.pmove.delta_angles[0],cl->ps.pmove.delta_angles[1],cl->ps.pmove.delta_angles[2]);
}
#endif


#if 0

// RSP 090200 - general dump of trace_t field values

void Dump_Trace(trace_t *tr)
{
	printf("********************* %5.2f **************************\n",level.time);
	printf("allsolid is %s\n",tr->allsolid ? "true" : "false");
	printf("startsolid is %s\n",tr->startsolid ? "true" : "false");
	printf("fraction %5.2f\n",tr->fraction);
	printf("endpos %s\n",vtos(tr->endpos));
	printf("surface.name %s\n",tr->surface->name);
	printf("flags %x\n",tr->surface->flags);
	printf("value %x\n",tr->surface->value);
	printf("contents %x\n",tr->contents);
	printf("ent %s\n",tr->ent ? tr->ent->classname : "NULL");
	printf("ent origin %s\n",vtos(tr->ent->s.origin));
}

#endif
