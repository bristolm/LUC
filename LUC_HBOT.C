// LUC Dreadlock and Evil Guardbot code

#include	"g_local.h"
#include	"luc_hbot.h"

// RSP 070700 - extensive changes

// CheckSpawnPoint() returns TRUE if the entity will fit.
// It returns false if SOLID is encountered at the origin or
// at any of the bounding box corners.

qboolean CheckSpawnPoint(edict_t *ent)
{
	vec3_t c;

	// first check origin

	if (gi.pointcontents(ent->s.origin) & CONTENTS_SOLID)
		return false;	// no good

	// then check corners of bounding box

	VectorAdd(ent->s.origin,ent->mins,c);

	if (!(gi.pointcontents(c) & CONTENTS_SOLID))
	{
		c[1] = ent->s.origin[1] + ent->maxs[1];
		if (!(gi.pointcontents(c) & CONTENTS_SOLID))
		{
			c[0] = ent->s.origin[0] + ent->maxs[0];
			if (!(gi.pointcontents(c) & CONTENTS_SOLID))
			{
				c[1] = ent->s.origin[1] + ent->mins[1];
				if (!(gi.pointcontents(c) & CONTENTS_SOLID))
				{
					c[0] = ent->s.origin[0] + ent->mins[0];
					c[2] = ent->s.origin[2] + ent->maxs[2];
					if (!(gi.pointcontents(c) & CONTENTS_SOLID))
					{
						c[1] = ent->s.origin[1] + ent->maxs[1];
						if (!(gi.pointcontents(c) & CONTENTS_SOLID))
						{
							c[0] = ent->s.origin[0] + ent->maxs[0];
							if (!(gi.pointcontents(c) & CONTENTS_SOLID))
							{
								c[1] = ent->s.origin[1] + ent->mins[1];
								if (!(gi.pointcontents(c) & CONTENTS_SOLID))
									return true;
							}
						}
					}
				}
			}
		}
	}

	// got here, so we cannot spawn at this point
	return false;
}


// Tells the player the status of his Dreadlock

void DreadlockStatus(edict_t *player) 
{
    edict_t *dreadlock;
	char	msg[512];	// RSP 091600

	switch(player->client->pers.dreadlock)
	{
	default:
	case DLOCK_UNAVAILABLE:
		HelpMessage(player,"You have no Dreadlock available\n");	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(player,"You have no Dreadlock available\n");
		break;
	case DLOCK_DOCKED:
		sprintf(msg,"Dreadlock available,\nbut inactive\nHealth = %d\n",player->client->pers.dreadlock_health);	// RSP 091600
		HelpMessage(player,msg);	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(player,"Dreadlock available, but inactive\nHealth = %d\n",player->client->pers.dreadlock_health);
		break;
	case DLOCK_LAUNCHED:
    	dreadlock = player->client->pers.Dreadlock;

		// dreadlock is engaged in an activity

        if (dreadlock->state == DLOCK_ATTACK)
		{
			sprintf(msg,"Dreadlock attacking enemy\nHealth = %d\nEnemy health = %d\n",dreadlock->health,dreadlock->enemy->health);	// RSP 091600
			HelpMessage(player,msg);	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(player,"Dreadlock attacking enemy\nHealth = %d\nEnemy health = %d\n",dreadlock->health,dreadlock->enemy->health);
		}
		else
		{
			sprintf(msg,"Dreadlock active\nHealth = %d\n",dreadlock->health);	// RSP 091600
			HelpMessage(player,msg);	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(player,"Dreadlock active\nHealth = %d\n",dreadlock->health);
		}
		break;
	}
}


// Uses the regular Q2 movement routines to go after an enemy or a Waypoint

void Dreadlock_Move(edict_t *dreadlock,edict_t *target,float dist)
{
	vec3_t dir,start; 

    // Calculate some directional stuff

    VectorCopy(target->s.origin,start);
    start[2] += 20;
    VectorSubtract(start,dreadlock->s.origin,dir);
    VectorCopy(dir,dreadlock->movedir);
    vectoangles(dir,dreadlock->s.angles);
    dreadlock->ideal_yaw = dreadlock->s.angles[YAW];
    M_MoveToGoal(dreadlock,dist);
}


// Find a Waypoint if nearby

edict_t *AcquireWaypoint(edict_t *dreadlock)
{
	edict_t *newent = NULL;
	edict_t *waypoint = NULL;

	while ((newent = findradius(newent,dreadlock->s.origin,DLOCK_WAYPOINT_RADIUS)))
	{
		if ((newent->identity == ID_WAYPOINT) && (newent->spawnflags & SPAWNFLAG_WAYPOINT_DLOCK))
		{
			// set the target and break out of the loop

			waypoint = dreadlock->enemy = dreadlock->goalentity = newent;
			break;
		}
	}
	return waypoint;
}


// Find an enemy to go after

edict_t *AcquireTarget(edict_t *player)
{
	vec3_t	start,forward,end;
	trace_t	tr;
	edict_t *newent;

	// if the crosshair is aimed at an enemy, attack it
	// locate our start point by adding our viewheight to our origin

	VectorCopy(player->s.origin,start);
	start[2] += player->viewheight;

	// give us our forward viewing angle

	AngleVectors(player->client->v_angle,forward,NULL,NULL);

	// multiply this forward unit vector by 8192, 
	// add it to the start position, and store this
	// endpoint in end

	VectorMA(start,8192,forward,end);

	// trace a straight line, stop on solid wall or monster

	tr = gi.trace(start,NULL,NULL,end,player,MASK_MONSTERSOLID);

	// if a viable target was found, set the enemy

	newent = NULL;
	if (tr.ent && tr.ent->takedamage) 
        if (tr.fraction * 8192 <= TARGETRADIUS)	// is target within range?
            if (tr.ent->svflags & SVF_MONSTER)	// make sure it's worth shooting at
				newent = tr.ent;
			else
			{
				HelpMessage(player,"Dreadlock doesn't know what\nto do with selected target.");	// RSP 091600 - replaces gi.centerprintf
//				gi.centerprintf(player,"Dreadlock doesn't know what\nto do with selected target.");
			}
		else
		{
			HelpMessage(player,"Selected target is out\nof the Dreadlock's range!");	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(player,"Selected target is out of\nthe Dreadlock's range!");
		}
	else
	{
		HelpMessage(player,"Selected target can't be hurt!");	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(player,"Selected target can't be hurt!");
	}

	return newent;
}


// Find an enemy to go after

void DreadlockTarget(edict_t *player) 
{
	edict_t *newenemy = AcquireTarget(player);

    if (newenemy)
    {
        edict_t *dreadlock = player->client->pers.Dreadlock;

        dreadlock->goalentity = dreadlock->enemy = newenemy;
        dreadlock->state = DLOCK_ATTACK;
        dreadlock->flags |= FL_AGGRESSIVE;
    }
}


// Fire the Dreadlock's blaster

void Dreadlock_Fire(edict_t *dreadlock,float damage)
{
    vec3_t  start,target,point;
    vec3_t  forward,ofs;
    float   dist;
	edict_t	*enemy = dreadlock->enemy;

    dist = G_EntDist(dreadlock,enemy);

	VectorAdd(enemy->absmax,enemy->absmin,point);
	VectorScale(point,0.5,point);
    if ((enemy->health > 0) && (dist > 64))
        VectorMA(point,dist*(1/1000),enemy->velocity, target);
    else
        VectorCopy(point,target);

	// Align dreadlock toward target

    VectorSubtract(target,dreadlock->s.origin,forward);
    vectoangles(forward,dreadlock->s.angles);

    AngleVectors(dreadlock->s.angles,forward,NULL,NULL);
    VectorScale(forward,8,ofs);
    VectorAdd(dreadlock->s.origin,ofs,start);

    VectorSubtract(target,start,forward);
    VectorNormalize(forward);

    monster_fire_blaster(dreadlock,start,forward,damage,600,MZ_BLASTER,EF_BLASTER);

	// check [XY] distance from enemy. try to stay at least SEPARATIONRADIUS away.
	// don't consider Z difference, because doing so would allow the dreadlock to
	// get closer to the enemy in the [XY] plane. The battle looks better if the
	// dreadlock isn't allowed to get right over the enemy.

	VectorCopy(dreadlock->s.origin,start);
	VectorCopy(enemy->s.origin,target);
	start[2] = target[2] = 0;
	VectorSubtract(start,target,ofs);
	if (VectorLength(ofs) < SEPARATIONRADIUS)
	{
		VectorNormalize(ofs);
		VectorSet(dreadlock->velocity,ofs[0]*(DLOCK_WANDER_SPEED),ofs[1]*(DLOCK_WANDER_SPEED),0);
	}
	else
	{	// strafe a bit
	    dreadlock->velocity[0] = crandom() * DLOCK_WANDER_SPEED;
		dreadlock->velocity[1] = crandom() * DLOCK_WANDER_SPEED;
		if (dreadlock->groundentity)
			dreadlock->velocity[2] = 10;
		else
			dreadlock->velocity[2] = crandom() * DLOCK_WANDER_SPEED/2;
	}
}


// Dreadlock kicked the bucket

void Dreadlock_Die(edict_t *dreadlock) 
{
	// Immediately Flag as not available

	dreadlock->dlockOwner->client->pers.dreadlock = DLOCK_UNAVAILABLE; 

	HelpMessage(dreadlock->dlockOwner,"Dreadlock has been destroyed!\n");	// RSP 091600 - replaces gi.centerprintf
//	gi.centerprintf(dreadlock->dlockOwner,"Dreadlock has been destroyed!\n");

	// We're going to blow up the dreadlock

	// Do damage to all ents within blast radius

	if (!G_ClientInGame(dreadlock->dlockOwner))
		// Owner has since died so don't assign damage frags to Dreadlock's owner
		T_RadiusDamage(dreadlock, dreadlock, 2*dreadlock->dmg, NULL, dreadlock->dmg_radius, MOD_DREADLOCK);
	else
		// Owner alive, so assign all frags to the Dreadlock's owner
		T_RadiusDamage(dreadlock, dreadlock->dlockOwner, 2*dreadlock->dmg, NULL, dreadlock->dmg_radius, MOD_DREADLOCK);

	// Spew some Dreadlock guts

	G_Make_Debris(dreadlock, DEBRIS1_MODEL, DEBRIS2_MODEL, DEBRIS3_MODEL);

	// Blow up the Dreadlock Unit

	SpawnExplosion(dreadlock,TE_EXPLOSION1,MULTICAST_PVS,EXPLODE_DLOCK);

	G_FreeEdict(dreadlock); 
}


// Dreadlock got recalled

void Dreadlock_recall(edict_t *dreadlock, qboolean showmsg) 
{
	if (showmsg)
	{
		gi.sound(dreadlock->dlockOwner,CHAN_VOICE,ACTIVATION_SOUND,1,ATTN_NORM,0);    // Play recall sound
		HelpMessage(dreadlock->dlockOwner,"Dreadlock has been recalled!\n");
	}
	else
	{	// endgame scenario. maximize the dreadlock's health
		dreadlock->health = DLOCK_MAX_HEALTH;
	}

	// Immediately Flag as inactive

	dreadlock->dlockOwner->client->pers.dreadlock = DLOCK_DOCKED; 
	dreadlock->dlockOwner->client->pers.dreadlock_health = dreadlock->health;

    // Draw the teleport splash, and wait 1 frame to remove Dreadlock,
    // so the splash can be seen.

    dreadlock->s.event = EV_PLAYER_TELEPORT; // draw the teleport splash
    dreadlock->think = G_FreeEdict;
    dreadlock->nextthink = level.time + FRAMETIME;
}


// Returns true if Dreadlock has been deactivated

qboolean DeActivated(edict_t *dreadlock) 
{
	int value = dreadlock->dlockOwner->client->pers.dreadlock;

	if (value == DLOCK_DOCKED)
	{
		Dreadlock_recall(dreadlock,true);
		return true; 
	}

	if (value == DLOCK_UNAVAILABLE)
	{
		Dreadlock_Die(dreadlock);
		return true; 
	}

	return false;
}


// Dreadlock touched something

void Dreadlock_Touch(edict_t *dreadlock, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	v;
	float	speed;

	if (dreadlock->flags & FL_ON)
		return;	// Prevents endless loop, since SV_FlyMove() calls Dreadlock_Touch()

	if (Q_stricmp(other->classname,"freed") == 0)
		return;

	dreadlock->flags |= FL_ON;	// turn on loop prevention

	VectorCopy(dreadlock->velocity,v);
	speed = VectorLength(v);
	SV_FlyMove(dreadlock,FRAMETIME,MASK_SOLID);	// clip velocity

	// Some vector of the velocity has been lost. If wandering, retain speed by changing the
	// remaining vectors. If catching up, don't bother. This is because catching up makes dreadlock
	// travel real fast, and a sudden high-velocity change looks odd.

	if (VectorLength(dreadlock->velocity) < DLOCK_WANDER_SPEED)
		VectorScale(dreadlock->velocity,speed/VectorLength(dreadlock->velocity),dreadlock->velocity);

	dreadlock->flags &= ~FL_ON;			// turn off loop prevention
}


// Can the Dreadlock see its owner?

qboolean Dreadlock_See_Owner(edict_t *dreadlock)
{
	vec3_t v,a;

    if (visible(dreadlock,dreadlock->dlockOwner))
	{
		// Pick a spot at the owner's head, not his origin.
		// This is to keep the dreadlock from getting too near the floor
		// when traveling toward the owner.

		VectorCopy(dreadlock->dlockOwner->s.origin,a);
		a[2] += dreadlock->dlockOwner->maxs[2];
        VectorSubtract(a,dreadlock->s.origin,v);
		VectorScale(v,0.5,dreadlock->velocity);

		// Don't get stuck on the ground

		if (dreadlock->groundentity && (dreadlock->velocity[2] <= 0))
			dreadlock->velocity[2] = 10;

        _AlignDirVelocity(dreadlock);   // align dir to velocity
        dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
		dreadlock->state = DLOCK_WANDER;
		return true;
	}
	return false;
}


// Teleport the Dreadlock closer to its owner if possible

void Dreadlock_Teleport(edict_t *dreadlock)
{
    int		i;
	edict_t	*owner = dreadlock->dlockOwner;
	vec3_t	s1,s2;
	vec3_t	candidates[] =
	{
		-50,-50,50,
		-50, 50,50,
		 50, 50,50,
		 50,-50,50
	};

	// can't see player or trail
	// teleport to player's neighborhood
    // try 4 spots around player

	gi.unlinkentity(dreadlock);				// put it back after teleport
	VectorCopy(dreadlock->s.origin,s1);		// temp save
	VectorCopy(dreadlock->s.old_origin,s2);	// temp save
	for (i = 0 ; i < 4 ; i++)
	{
		VectorAdd(owner->s.origin,candidates[i],dreadlock->s.origin);
		VectorCopy(dreadlock->s.origin,dreadlock->s.old_origin);
		if (CheckSpawnPoint(dreadlock))
			break;
	}

	if (i < 4)
	{
//		gi.sound(dreadlock,CHAN_VOICE,ACTIVATION_SOUND,1,ATTN_NORM,0);	// RSP 091700 - not during teleport
		dreadlock->s.event = EV_PLAYER_TELEPORT; // draw the teleport splash
		dreadlock->state = DLOCK_TELEPORT;		// teleport next frame, then continue to wander around
		VectorClear(dreadlock->velocity);		// clear the velocity
		dreadlock->svflags |= SVF_NOCLIENT;		// make invisible during teleport
	}
	else
	{
		gi.linkentity(dreadlock);				// put it back now
		Dreadlock_recall(dreadlock,true);		// total failure, so recall it
	}
}


// Altered _findlifeinradius() function

edict_t *FindOwnerEnemy(edict_t *dreadlock,float rad)
{
	edict_t *item = g_edicts;

	for ( ; item < &g_edicts[globals.num_edicts]; item++)
	{
		if (!(item->svflags & SVF_MONSTER))
			continue;

		if (!item->inuse)
			continue;

		if (item->solid == SOLID_NOT)
			continue;

		if (item->deadflag != DEAD_NO)
			continue;

		if (!item->enemy)
			continue;

		if ((item->enemy == dreadlock->dlockOwner) || (item->enemy == dreadlock))
			if (G_EntDist(dreadlock,item) <= rad)
				return item;
	}

	return NULL;
}


// Generalized Dreadlock thoughts.
// This is used once each second. On the frames between these 
// uses, Dreadlock_Think() is called to make sure the
// model animation continues, and to take care of movement
// toward an enemy or waypoint.

void Dreadlock_Action(edict_t *dreadlock) 
{
    int		i;
	float   dist;
    edict_t *item;
    edict_t *owner = dreadlock->dlockOwner;
	trace_t	tr;
	vec3_t	v,a;
    edict_t *trailspot;

	// Mark current owner location, if owner's visible

	if (visible(dreadlock,owner))
		VectorCopy(owner->s.origin,dreadlock->pos1);

    // If not currently battling an enemy, see if there's a new enemy to attack

    if ((dreadlock->state != DLOCK_ATTACK) && (dreadlock->state != DLOCK_RETURN_ATTACK) && (dreadlock->flags & FL_AGGRESSIVE))
    {
    	// is there an enemy attacking the dreadlock's owner or the dreadlock?

		item = FindOwnerEnemy(dreadlock,TARGETRADIUS);
		if (item)
		{
			dreadlock->state = DLOCK_ATTACK;
			dreadlock->enemy = dreadlock->goalentity = item;
		}
	}

    // If not attacking an enemy, or not already following Waypoints,
	// see if there's a nearby DLOCK Waypoint.
	// If there is, switch to following the Waypoint trail

    if ((dreadlock->state != DLOCK_ATTACK) && (dreadlock->state != DLOCK_RETURN_ATTACK) && (dreadlock->state != DLOCK_FOLLOW))
		if (AcquireWaypoint(dreadlock))
        	dreadlock->state = DLOCK_FOLLOW;

	// Dreadlock will trigger all target_clue entities w/in CLUERADIUS of it

	item = NULL;
	VectorCopy(dreadlock->s.origin,v);

	while (item = findradius(item,v,CLUERADIUS))
		if (Q_stricmp(item->classname,"target_clue") == 0)
			item->use(item,dreadlock->dlockOwner,dreadlock);
			
	// Once each second, if dreadlock didn't move, try to move him. This velocity
	// might get changed further down, but that's ok.
	if (VectorCompare(dreadlock->pos2,dreadlock->s.origin))
	{	// strafe a bit
		dreadlock->velocity[0] = crandom() * DLOCK_WANDER_SPEED;
		dreadlock->velocity[1] = crandom() * DLOCK_WANDER_SPEED;
		dreadlock->velocity[2] = crandom() * DLOCK_WANDER_SPEED/2;
	}

	VectorCopy(dreadlock->s.origin,dreadlock->pos2);

	// Do something based on current state

    switch(dreadlock->state)
    {

	default:
    case DLOCK_WANDER:

		// Go back and gib a freshly-killed enemy?

		if ((dreadlock->flags & FL_ATTACKING) && (level.time >= dreadlock->last_move_time))
		{
			dreadlock->state = DLOCK_RETURN_ATTACK;
			dreadlock->enemy = dreadlock->goalentity = dreadlock->oldenemy;
			dreadlock->oldenemy = NULL;
			dreadlock->flags &= ~FL_ATTACKING;	// no longer needed
			return;
		}

		// Special case to keep dreadlock from being stuck motionless on the floor

		if (dreadlock->groundentity)
			dreadlock->velocity[2] = 10;

		// leave if dreadlock hasn't completed his current tack

        if (level.time < dreadlock->touch_debounce_time)
			return;

		// If the dreadlock can still see its owner, check for how far away they are.
		// The dreadlock should stay between MINLEASH and MAXLEASH from the player.
		// If less than MINLEASH, move away.
		// If more than MAXLEASH, move closer.
		// If the dreadlock can't see its owner, move closer.

		if (visible(dreadlock,owner))
		{
        	// Is DLOCK w/in his leash range?

        	dist = G_EntDist(dreadlock,owner);
			if ((dist >= MINLEASH) && (dist <= MAXLEASH))
			{
				// randomly change direction

            	VectorSet(dreadlock->velocity,crandom()*DLOCK_WANDER_SPEED,crandom()*DLOCK_WANDER_SPEED,crandom()*DLOCK_WANDER_SPEED);

				// Stay on this course for a small amount of time

				dreadlock->touch_debounce_time = level.time + 3 + crandom();

            	_AlignDirVelocity(dreadlock);   // align dir to velocity
            	dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
				return;
			}

			if (dist < MINLEASH)
        	{   // too close, move away from owner

				VectorSubtract(dreadlock->s.origin,owner->s.origin,v);

				// don't be driven down into the floor

				if (v[2] < 0)
					v[2] *= -1;	// go up instead

				VectorNormalize(v);
				VectorSet(dreadlock->velocity,v[0]*DLOCK_WANDER_SPEED,v[1]*DLOCK_WANDER_SPEED,0);

				// Stay on this course for a small amount of time

				dreadlock->touch_debounce_time = level.time + 3 + crandom();

            	_AlignDirVelocity(dreadlock);   // align dir to velocity
            	dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
				return;
			}

        	// too far away, move toward owner

			// Can dreadlock see its owner?

        	if (Dreadlock_See_Owner(dreadlock))
				return;
		}

		// If we got here, the dreadlock can't see its owner.

        // Can dreadlock see the last recorded owner position?

		tr = gi.trace(dreadlock->s.origin,NULL,NULL,dreadlock->pos1,dreadlock,MASK_SOLID);
        VectorSubtract(dreadlock->s.origin,dreadlock->pos1,v);
		if ((VectorLength(v) > 36) && (tr.fraction == 1))
		{
			// Scale velocity so dreadlock gets there in 1 second

			// Pick a spot at the owner's head, not his origin.
			// This is to keep the dreadlock from getting too near the floor
			// when traveling toward the owner.

			VectorCopy(dreadlock->pos1,a);
			a[2] += dreadlock->dlockOwner->maxs[2];
			VectorSubtract(a,dreadlock->s.origin,v);
            VectorCopy(v,dreadlock->velocity);

			// don't reset touch_debounce_time

            _AlignDirVelocity(dreadlock);   // align dir to velocity
            dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
			return;
		}
			
		// can dreadlock see the player_trail?
						
		trailspot = NULL;
		for (i = 0 ; i < 8 ; i++)
		{
			trailspot = PlayerTrail_PickPrev(trailspot);
			if ((trailspot == NULL) || (VectorLength(trailspot->s.origin) == 0))
				break;
			if (visible(dreadlock,trailspot))
			{
				// Pick a spot at the owner's head, not his origin.
				// This is to keep the dreadlock from getting too near the floor
				// when traveling toward the owner.

				VectorCopy(trailspot->s.origin,a);
				a[2] += dreadlock->dlockOwner->maxs[2];
				VectorSubtract(dreadlock->s.origin,a,v);

				// Scale velocity so dreadlock gets there in 1 second
				VectorNegate(v,v);
                VectorCopy(v,dreadlock->velocity);
				dreadlock->monsterinfo.trail_time = trailspot->timestamp;
				dreadlock->state = DLOCK_CATCHUP;

				// don't reset touch_debounce_time

            	_AlignDirVelocity(dreadlock);   // align dir to velocity
            	dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
				return;
			}
		}
                    		
		// can't see player or trail, so teleport to player's neighborhood

		Dreadlock_Teleport(dreadlock);
        break;

    case DLOCK_ATTACK:

        // if no enemy, do nothing (for now)

        if (!dreadlock->enemy)
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            return;
        }

        // if enemy is destroyed, return to wandering

        if (!dreadlock->enemy->inuse || (dreadlock->enemy->health <= dreadlock->enemy->gib_health))
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            dreadlock->enemy = dreadlock->goalentity = NULL;
            return;
        }

		// if enemy is dead, but not gibbed, return to wandering for a while

        if (dreadlock->enemy->health <= 0)
        {
			dreadlock->last_move_time = level.time + 6;
            dreadlock->state = DLOCK_WANDER; // return to wandering
			dreadlock->oldenemy = dreadlock->enemy;	// save for RETURN_ATTACK
            dreadlock->enemy = dreadlock->goalentity = NULL;
			dreadlock->flags |= FL_ATTACKING;	// trigger RETURN_ATTACK after last_move_time is reached
            return;
        }

		// RSP 090100
		// if enemy is frozen via the matrix, or the teleportgun, return to wandering
		if (dreadlock->enemy->flags & FL_FROZEN)
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            dreadlock->enemy = dreadlock->goalentity = NULL;
            return;
        }

        dist = G_EntDist(dreadlock,dreadlock->enemy);

        if (dist > TARGETRADIUS)	// forget about him if he's too far away
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            dreadlock->enemy = dreadlock->goalentity = NULL;
            return;
        }

        if ((visible(dreadlock,dreadlock->enemy)) && (infront(dreadlock,dreadlock->enemy)) && (dist <= FIRERADIUS))
        	Dreadlock_Fire(dreadlock,dreadlock->dmg);

        break;

    case DLOCK_RETURN_ATTACK:

        // if no enemy, do nothing (for now)

        if (!dreadlock->enemy)
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            return;
        }

        // if enemy is destroyed, return to wandering

        if (!visible(dreadlock,dreadlock->enemy) || 
			!dreadlock->enemy->inuse ||
			(Q_stricmp(dreadlock->enemy->classname,"freed") == 0) ||
			(dreadlock->enemy->health <= dreadlock->enemy->gib_health) ||
			(dreadlock->enemy->solid != SOLID_BBOX))
        {
            dreadlock->state = DLOCK_WANDER; // return to wandering
            dreadlock->enemy = dreadlock->goalentity = NULL;
            return;
        }

       	Dreadlock_Fire(dreadlock,2*dreadlock->dmg);		// Gib it twice as quick

        break;

    case DLOCK_FOLLOW:

		// following waypoints; enemy is set to the target waypoint

        dist = G_EntDist(dreadlock,dreadlock->enemy);
		if (dist <= DLOCK_WAYPOINT_CLOSE_ENOUGH)
		{
			edict_t *next = NULL;

			// seek out next waypoint
            if (dreadlock->enemy->target != NULL)
			{
                next = G_PickTarget(dreadlock->enemy->target);

				// If the next waypoint is a dreadlock waypoint allow the move. Else stop here.

                if (next)
					if ((next->identity != ID_WAYPOINT) || (!(next->spawnflags & SPAWNFLAG_WAYPOINT_DLOCK)))
						next = NULL;
			}

			if (next == NULL)
			{
                dreadlock->state = DLOCK_WANDER; // return to wandering
                dreadlock->enemy = dreadlock->goalentity = NULL;
                return;
			}

            dreadlock->enemy = dreadlock->goalentity = next;
		}

		// WPO 100699 Doesn't appear to be needed, we'll see
//      Flyer_TouchTriggers(dreadlock,50.0);
        break;

	case DLOCK_CATCHUP:	// following player trail, trying to catch up; reached player trail spot

		// Can dreadlock see its owner?

        if (Dreadlock_See_Owner(dreadlock))
			return;

		// leave if dreadlock hasn't completed his current tack

        if (level.time < dreadlock->touch_debounce_time)
			return;

		// seek out next trail point

		trailspot = PlayerTrail_PickNext(dreadlock);
		if ((trailspot == NULL) || (VectorLength(trailspot->s.origin) == 0))
		{
			Dreadlock_Teleport(dreadlock);	// give up, get there real fast
			return;
		}

		dreadlock->monsterinfo.trail_time = trailspot->timestamp;
		VectorSubtract(dreadlock->s.origin,trailspot->s.origin,v);
		VectorNormalize(v);
        VectorSet(dreadlock->velocity,v[0]*(-DLOCK_CATCHUP_SPEED),v[1]*(-DLOCK_CATCHUP_SPEED),v[2]*(-DLOCK_CATCHUP_SPEED));
		dreadlock->monsterinfo.trail_time = trailspot->timestamp;

		// Stay on this course for a small amount of time

		dreadlock->touch_debounce_time = level.time + 3 + crandom();

        _AlignDirVelocity(dreadlock);   // align dir to velocity
        dreadlock->s.angles[PITCH] = 0; // stay horizontal, don't pitch up or down
		break;
    }
}


// This is called each FRAME to guarantee model animation and progress
// toward enemies or Waypoints

void Dreadlock_Think(edict_t *dreadlock)
{
	float dist;

    if (DeActivated(dreadlock))
		return;

	M_CheckGround (dreadlock);	// Always check for ground

	if (dreadlock->enemy)
		dist = G_EntDist(dreadlock,dreadlock->enemy);

	// If currently attacking, take care of any possible movement

	if (dreadlock->state == DLOCK_ATTACK)
        if ((!visible(dreadlock,dreadlock->enemy)) || (!infront(dreadlock,dreadlock->enemy)) || (dist > FIRERADIUS))
            Dreadlock_Move(dreadlock,dreadlock->enemy,DLOCK_ATTACK_SPEED*FRAMETIME);

	// If currently following waypoints, take care of any possible movement

	if (dreadlock->state == DLOCK_FOLLOW)
		if (dist > DLOCK_WAYPOINT_CLOSE_ENOUGH)
			Dreadlock_Move(dreadlock,dreadlock->enemy,DLOCK_WANDER_SPEED*FRAMETIME);

    // Control model animations

    if (++dreadlock->s.frame > DLOCK_LASTFRAME)
        dreadlock->s.frame = DLOCK_FIRSTFRAME;

	// Teleport if it's time to

	if (dreadlock->state == DLOCK_TELEPORT)
	{
		// teleporting, time to put dreadlock back
//		gi.sound(dreadlock,CHAN_VOICE,ACTIVATION_SOUND,1,ATTN_NORM,0);	// RSP 091700 - not during teleport
		dreadlock->svflags &= ~SVF_NOCLIENT;		// visible again
		dreadlock->s.event = EV_PLAYER_TELEPORT; // draw the teleport splash
		gi.linkentity(dreadlock);
		if (dreadlock->enemy && (dreadlock->enemy->enemy == dreadlock))
			dreadlock->state = DLOCK_ATTACK;
		else
			dreadlock->state = DLOCK_WANDER;
		dreadlock->touch_debounce_time = 0;
		dreadlock->count = 9;	// force following call to Dreadlock_Action()
	}

	// Call Dreadlock_Action every 10 frames

	if (++dreadlock->count == 10)
	{
		dreadlock->count = 0;
		Dreadlock_Action(dreadlock);
	}

    dreadlock->nextthink = level.time + FRAMETIME;
}


// Fire off the Dreadlock into the cruel world

void Launch_Dreadlock(edict_t *player, qboolean anywhere)	// RSP 092900 - add anywhere boolean
{
    edict_t *dreadlock;
    vec3_t  right,forward,offset; 

	dreadlock = G_Spawn();
	dreadlock->classname = "Dreadlock";
	dreadlock->owner = dreadlock;				// need to do this or we can walk thru dreadlock
	dreadlock->dlockOwner = player;
	player->client->pers.Dreadlock = dreadlock;
	dreadlock->movetype = MOVETYPE_FLY;	// Float around gently.
	dreadlock->solid = SOLID_BBOX;		// Enable touch capability.
	dreadlock->clipmask = MASK_SHOT;	// Ability to be hit by weapon's fire.
	dreadlock->model = "models/monsters/bots/hbot/tris.md2";		// RSP 082200
	dreadlock->s.modelindex = gi.modelindex(dreadlock->model);		// RSP 082200
	VectorSet(dreadlock->mins, -16, -16, -24);
	VectorSet(dreadlock->maxs, 16, 16, 8);
	dreadlock->dmg = DLOCK_DAMAGE;
	dreadlock->dmg_radius = 300;			// 300 unit radius damage upon explosion.
	dreadlock->health = player->client->pers.dreadlock_health;
	dreadlock->max_health = DLOCK_MAX_HEALTH;
    dreadlock->touch = Dreadlock_Touch;
    dreadlock->s.event = EV_PLAYER_TELEPORT; // draw the teleport splash
	dreadlock->takedamage = DAMAGE_YES;
	dreadlock->gravity = 1.5;
	dreadlock->mass = 200;
	dreadlock->identity = ID_DLOCK;
    dreadlock->flags |= FL_PASSENGER;		// possible BoxCar passenger
	dreadlock->flags |= FL_NO_KNOCKBACK;	// no momentum impact when dreadlock is hit by something
	dreadlock->flags |= FL_FLY;				// flyer

	// compute dreadlock spawning location, right in front of you

   	AngleVectors(player->client->v_angle,forward,NULL,NULL);
	VectorSet(offset,100,0,player->viewheight + 40);
   	G_ProjectSource(player->s.origin,offset,forward,right,dreadlock->s.origin);
	if (!CheckSpawnPoint(dreadlock))
	{
		if (!anywhere)
		{
			HelpMessage(player,"No room to launch Dreadlock!");	// RSP 091600 - replaces gi.centerprintf
			player->client->pers.Dreadlock = NULL;
			G_FreeEdict(dreadlock); 
			return;
		}

		// RSP 092900 - if you can't spawn the dreadlock in front of the player, randomly
		// select a location somewhere nearby until you're successful.
		while (true)
		{
			VectorSet(offset,crandom()*40,crandom()*40,crandom()*40);
			VectorAdd(player->s.origin,offset,dreadlock->s.origin);
			VectorCopy(dreadlock->s.origin,dreadlock->s.old_origin);
			if (CheckSpawnPoint(dreadlock))
				break;
		}
	}

	VectorCopy(dreadlock->s.origin,dreadlock->s.old_origin);

	// Set dreadlock's initial angles so it's facing away from you

	vectoangles(forward,dreadlock->s.angles);
	dreadlock->s.angles[2] = 0;

    // Player might have crosshairs on an enemy when launching the
    // dreadlock. If so, that enemy becomes dreadlock's enemy.

    dreadlock->enemy = dreadlock->goalentity = AcquireTarget(player);

    if (dreadlock->enemy)
    {
        dreadlock->state = DLOCK_ATTACK;	// Start by attacking enemy
        dreadlock->flags |= FL_AGGRESSIVE;	// Dreadlock now aggressive
    }
    else
        dreadlock->state = DLOCK_WANDER;	// Start by wandering around

	dreadlock->count = 0;					// used for keeping track of model frames
	dreadlock->think = Dreadlock_Think;
	dreadlock->nextthink = level.time + FRAMETIME;

    // Let Owner know that Dreadlock is now Active

	if (anywhere)
		HelpMessage(player,"I am here...\n");	// RSP 093000 - endgame scenario
	else
	{
		gi.sound(dreadlock,CHAN_VOICE,ACTIVATION_SOUND,1,ATTN_NORM,0);	// Play activation sound
		HelpMessage(player,"Dreadlock Activated\n");
	}

	gi.linkentity(dreadlock);
	player->client->pers.dreadlock = DLOCK_LAUNCHED; // Flag Dreadlock as ON.
}


// Dreadlock User Commands

void Cmd_Dreadlock_f(edict_t *player) 
{
    int     dreadlock_state = player->client->pers.dreadlock;
    edict_t *dreadlock = player->client->pers.Dreadlock;

	// Don't allow dead/respawning players to invoke

    if (!G_ClientInGame(player)) 
		return;

	// client issued status command

    if (Q_stricmp(gi.argv(1), "status") == 0)
	{
		DreadlockStatus(player);
		return;
	}

	// dreadlock isn't available

    if (dreadlock_state == DLOCK_UNAVAILABLE) 
	{
		HelpMessage(player,"You have no Dreadlock available!\n");	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(player,"You have no Dreadlock available!\n");
		return;
	}

	// client issued target command

    if (Q_stricmp(gi.argv(1), "target") == 0)
	{
        if (dreadlock_state == DLOCK_DOCKED) 
			Launch_Dreadlock(player,false);	// RSP 092900 - added boolean
		else
			DreadlockTarget(player);
		return;
	}

    // client toggled aggressiveness

    if (Q_stricmp(gi.argv(1),"aggressive") == 0)
	{
        edict_t *dreadlock;

        if (dreadlock_state == DLOCK_DOCKED)
		{
			HelpMessage(player,"Dreadlock is not active!\n");	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(player,"Dreadlock is not active!\n");
		}
		else	// dreadlock already active
		{
        	dreadlock = player->client->pers.Dreadlock;
            dreadlock->flags ^= FL_AGGRESSIVE;
        	if (dreadlock->flags & FL_AGGRESSIVE)
			{
				HelpMessage(player,"Dreadlock is now aggressive!\n");	// RSP 091600 - replaces gi.centerprintf
//				gi.centerprintf(player,"Dreadlock is now aggressive!\n");
			}
			else
			{
				HelpMessage(player,"Dreadlock is now passive!\n");	// RSP 091600 - replaces gi.centerprintf
//	            gi.centerprintf(player,"Dreadlock is now passive!\n");

            	// if there's an enemy targetting the dreadlock, target that enemy at the player

            	if (dreadlock->state == DLOCK_ATTACK)
				{
					if (dreadlock->enemy && (dreadlock->enemy->enemy == dreadlock))
                		dreadlock->enemy->enemy = dreadlock->enemy->goalentity = player;
            		dreadlock->enemy = dreadlock->goalentity = NULL;
            		dreadlock->state = DLOCK_WANDER;
				}
			}
        }
		return;
	}

    // Toggle the Dreadlock in and out of service

    if (dreadlock_state == DLOCK_DOCKED)     
		Launch_Dreadlock(player,false);	// activate the Dreadlock; RSP 092900 - added boolean
	else       
        Dreadlock_recall(player->client->pers.Dreadlock,true); // recall the Dreadlock
}


// RSP 100600
// Evil Guard bot
// This bot guards a location and fires on the player if the player gets too close.
// It will face the player all the time.
// It uses the model of the dreadlock.
// At some point, it will be told to allow the player to pass.
// It can also be told to fire on the player regardless of where the player is.

// Fire the Guardbot's blaster

#define GUARDBOT_FIRE_INTERVAL 10	// fire blaster once in this many frames

void Guardbot_Fire(edict_t *self)
{
    vec3_t  start,target;
    vec3_t  forward,ofs;

	if (!self->enemy)
		return;

    if (self->enemy->health <= 0)
		return;

	Pick_Client_Target(self,target);

    AngleVectors(self->s.angles,forward,NULL,NULL);
    VectorScale(forward,8,ofs);
    VectorAdd(self->s.origin,ofs,start);

    VectorSubtract(target,start,forward);
    VectorNormalize(forward);

    monster_fire_blaster(self,start,forward,self->dmg,600,MZ_BLASTER,EF_BLASTER);
}

// RSP 103100 - this is a table of sin() values for a circle of radius 1.0.
// 				it's used to add some vertical movement to the guardbot
//				as it stands guard.

#define GUARDBOT_VERT_POS	48	// number of positions
#define GUARDBOT_HEIGHT		16

float guardbot_vertical[] =
{
	 0.0000,
	 0.1305,
	 0.2588,
	 0.3827,
	 0.5000,
	 0.6088,
	 0.7071,
	 0.7934,
	 0.8660,
	 0.9239,
	 0.9659,
	 0.9914,
	 1.0000,
	 0.9914,
	 0.9659,
	 0.9239,
	 0.8660,
	 0.7934,
	 0.7071,
	 0.6088,
	 0.5000,
	 0.3827,
	 0.2588,
	 0.1305,
	 0.0000,
	-0.1305,
	-0.2588,
	-0.3827,
	-0.5000,
	-0.6088,
	-0.7071,
	-0.7934,
	-0.8660,
	-0.9239,
	-0.9659,
	-0.9914,
	-1.0000,
	-0.9914,
	-0.9659,
	-0.9239,
	-0.8660,
	-0.7934,
	-0.7071,
	-0.6088,
	-0.5000,
	-0.3827,
	-0.2588,
	-0.1305
};

// RSP 101900
// Check for player and back up if too close
/*
void Guardbot_Check_Backup(edict_t *self, edict_t *player)
{
	vec3_t v;
	
	if (!player)
		return;	// no clients to run from

	if (G_EntDist(self,player) < 256)						// too close?
	{
		VectorSubtract(self->s.origin,player->s.origin,v);	// find vector
		v[2] = 0;											// no z movement
    	VectorNormalize(v);
		VectorScale (v,4,v);
		SV_movestep (self,v,true,false);					// move away
	}
}
 */

void Guardbot_Think(edict_t *self)
{
	vec3_t	forward;
	edict_t	*enemy = level.sight_client;

    // Control model animations
    if (++self->s.frame > DLOCK_LASTFRAME)
        self->s.frame = DLOCK_FIRSTFRAME;

	// Adjust vertical position
	if (++(self->count2) == GUARDBOT_VERT_POS)
		self->count2 = 0;
	self->s.origin[2] = self->pos1[2] + self->attenuation*guardbot_vertical[self->count2];

	M_SetEffects (self);	// Put up power screen if needed

	// Can you see the enemy or their decoy?
	if (enemy && (enemy->health > 0) && !(enemy->svflags & SVF_NOCLIENT))
		if (enemy->decoy)
		{
			if (visible(self,enemy->decoy))
				enemy = enemy->decoy;	
			else if (!visible(self,enemy))	// can't see the decoy; can it see the player?
				enemy = NULL;	// can't see the player either
		}
		else if (!visible(self, enemy))	// no decoy; can it see the player?
			enemy = NULL;	// can't see the player

	if (enemy)	// something to look at?
	{
		self->enemy = enemy;

		// Align guardbot toward enemy
		VectorSubtract(enemy->s.origin,self->s.origin,forward);
		vectoangles(forward,self->s.angles);

		// Consider firing if aggressive
		if (self->flags & FL_AGGRESSIVE)	// RSP 101900
		{
			if (++self->count == GUARDBOT_FIRE_INTERVAL)
			{
				self->count = 0;

				// if superaggressive (shoot however far away the player is)
				// or if just aggressive and the player is w/in striking range, fire!
				if ((self->flags & FL_SUPERAGGRESSIVE) || (G_EntDist(self,enemy) < 256))
					Guardbot_Fire(self);
			}
		}
//		else	// Non-aggressive guardbots will back away from the player if too close
//			Guardbot_Check_Backup(self,enemy);
	}

	gi.linkentity(self);
	self->nextthink = level.time + FRAMETIME;
}


void Guardbot_Pain (edict_t *self, edict_t *other, float kick, int damage)
{
}

void Guardbot_Use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->flags |= FL_SUPERAGGRESSIVE;	// shoot at player however far away he is
}

/*QUAKED monster_guardbot (1 .5 0) (-16 -16 -24) (16 16 8) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_guardbot (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 8);
	VectorCopy(self->s.origin,self->pos1);
	self->movetype = MOVETYPE_FLY;
	self->solid = SOLID_BBOX;
	self->identity = ID_GUARDBOT;
	self->clipmask = MASK_SHOT;		// Ability to be hit by weapon's fire.
	self->model = "models/monsters/bots/hbot/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->health = DLOCK_MAX_HEALTH;
	self->max_health = DLOCK_MAX_HEALTH;
	self->takedamage = DAMAGE_YES;	// immortal, so health will always be reset to full after damage
	self->mass = 200;
	self->dmg = GUARDBOT_DAMAGE;

	self->flags |= (FL_NO_KNOCKBACK|FL_FLY|FL_AGGRESSIVE);	// no momentum impact when hit by something
															// flyer
															// RSP 101900 - won't budge, and will shoot

	self->think = Guardbot_Think;
	self->touch = Dreadlock_Touch;
	self->pain  = Guardbot_Pain;
	self->use   = Guardbot_Use;

	// How often to shoot
	self->count = rand() % GUARDBOT_FIRE_INTERVAL;

	self->count2 = rand() % GUARDBOT_VERT_POS;				// start at a random vertical offset
	self->attenuation = GUARDBOT_HEIGHT*(random()*0.5 + 0.5);
	self->s.origin[2] = self->pos1[2] + self->attenuation*guardbot_vertical[self->count2];

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = 50;
	self->svflags |= SVF_MONSTER;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

