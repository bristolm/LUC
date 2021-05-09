// m_move.c -- monster movement

#include "g_local.h"
#include "luc_m_gwhite.h"

#define	STEPSIZE	18

#define GWHITE_LOOKAHEAD 150	// RSP 092000: Distance to look ahead for obstructions. Prior to this,
								// it was 70, which is only 20 in front of the shark's nose. Too short.
								// Two body lengths out in front seems better.

#define GW_MAXSTUCKTIME    1	// RSP 041099: maximum time the shark can be stuck before beginning
								// to rotate in place to free himself. On 071400, changed
								// this from 20 to 1 so the shark will spend less time
								// hung up in a corner.

extern qboolean enemy_vis;		// RSP 032699

/*
=============
M_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/

qboolean M_CheckBottom (edict_t *ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;
	
	VectorAdd(ent->s.origin,ent->mins, mins);
	VectorAdd(ent->s.origin,ent->maxs, maxs);

//  if all of the points under the corners are solid world, don't bother
//  with the tougher checks
//  the corners must be within 16 of the midpoint

	start[2] = mins[2] - 1;
	for	(x = 0 ; x <= 1 ; x++)
		for	(y = 0 ; y <= 1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (gi.pointcontents (start) != CONTENTS_SOLID)
				goto realcheck;
		}

	return true;		// we got out easy

realcheck:
//
// check it for real...
//
	start[2] = mins[2];
	
// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*STEPSIZE;
	trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint	
	for	(x = 0 ; x <= 1 ; x++)
		for	(y = 0 ; y <= 1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);
			
			if ((trace.fraction != 1.0) && (trace.endpos[2] > bottom))
				bottom = trace.endpos[2];
			if ((trace.fraction == 1.0) || (mid - trace.endpos[2] > STEPSIZE))
				return false;
		}

	return true;
}

// CheckSharkCorners() returns TRUE if the shark will fit in the XY plane.
// It returns false if SOLID is encountered at
// any of 4 bounding box corners checked. This is a special check to make
// SURE the shark isn't wandering into the walls. Since this is only to
// check horizontal movements of the shark, we only need to check either the
// top plane of the bounding box or the bottom. This code checks the bottom.

qboolean CheckSharkPoint(edict_t *ent)
{
	vec3_t c;

	// check 4 corners of bounding box

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
					return true;
			}
		}
	}

	// got here, so we cannot move
	return false;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
//FIXME since we need to test end position contents here, can we avoid doing
//it again later in catagorize position?
// RSP 032699 - added doitanyway flag to force the move
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink, qboolean doitanyway)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	float		stepsize;
	vec3_t		test;
	int			contents;
	qboolean	sharkfits;

// try the move	
	VectorCopy (ent->s.origin, oldorg);
	VectorAdd (ent->s.origin, move, neworg);

	// RSP 022099: Jellyfish varies z slightly
	if ((ent->flags & FL_SWIM) && (ent->identity == ID_JELLY))	// RSP 022799 swimming jellies
		if ((rand()&7) == 1) // change direction slightly every so often
			move[2] += rand()&1 ? 2 : -2;

	if (ent->identity == ID_GREAT_WHITE)
	{
		// RSP 071400 - Shark varies z slightly. Use FL_ON to say whether you're moving
		// the shark UP (ON) or DOWN (OFF). Reset after 3 seconds; select another change.

		// First, make sure shark is still in water. If not, start him gliding down.

		contents = gi.pointcontents(ent->s.origin);
		if (!(contents & MASK_WATER))	// if not in the water,
			ent->flags &= ~FL_ON;		// down he goes
		else if (level.time >= ent->touch_debounce_time)
		{
			if (rand() & 1)		// 50% chance of changing direction 
			{
				if (rand() & 1)	// 50% chance of gliding up
					ent->flags |= FL_ON;
				else			// 50% chance of gliding down
					ent->flags &= ~FL_ON;
			}
			ent->touch_debounce_time = level.time + 3;	// don't consider changing again for 3 seconds
		}

		if (ent->flags & FL_ON)
			move[2]++;
		else
			move[2]--;
	}

	// flying monsters don't step up

	if (ent->flags & (FL_SWIM | FL_FLY))
	{	// MRB 102098 to get things to hover ... normally it runs through trying 
		//   a vertical move first - now it may do it on the 2nd pass (thereby not moving vertically)
		//   on the second pass instead
		int check_z = (ent->monsterinfo.aiflags & AI_NOSTEP ? 1 : 0);

		// try one move with vertical motion, then one without
		for (i = 0 ; i < 2 ; i++)
		{
			VectorAdd (ent->s.origin, move, neworg);

			if ((i == check_z) && (ent->goalentity || ent->enemy))
			{
				edict_t *moveto_ent = (ent->enemy ? ent->enemy : ent->goalentity);

				dz = ent->s.origin[2] - moveto_ent->s.origin[2];

				// RSP 032699 - shark can't just vertically match you so fast
				if (ent->identity == ID_GREAT_WHITE)
				{
					if (dz > 0)
						neworg[2] -= 2;
					else if (dz < 0)
						if (ent->waterlevel >= WL_WAISTWET)
							neworg[2] += 2;
				}
				else if (moveto_ent->client)
				{
					if (ent->identity == ID_GBOT)	// RSP 042500 gbots move slower
					{
						if (dz > (random()*40 + 40))
							neworg[2] -= 4;
						else if (dz < (random()*30 + 30))
							neworg[2] += 4;
						neworg[2] += rand()&1 ? 4 : 2;	// add a bit of randomness
					}
					else
					{
						if (dz > 40)
							neworg[2] -= 8;
						else if (!((ent->flags & FL_SWIM) && (ent->waterlevel < WL_WAISTWET)))
							if (dz < 30)
								neworg[2] += 8;
					}
				}
				else if (ent->goalentity)
				{	// This is more bee-lineish
					int		dist;
					vec3_t	v;

					if (ent->monsterinfo.currentmove)
					{	// Re-grab the distance
						mmove_t	*move = ent->monsterinfo.currentmove;
						dist = move->frame[ent->s.frame - move->firstframe].dist * ent->monsterinfo.scale;
					}
					else	// Figure it out ...
						dist = sqrt(move[0] * move[0] + move[1] * move[1]);

					VectorSubtract(ent->goalentity->s.origin,ent->s.origin,v);
					VectorNormalize(v);
					VectorMA(ent->s.origin,(float)dist,v,neworg);
				}
				else
					return false;	// RSP 051899 - lost goalentity for some reason
			}

			trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, neworg, ent, MASK_MONSTERSOLID); // RSP 012099

			// RSP 032699 - If this trace failed, and you were changing your z height, try it again
			// without the z height change
			if ((trace.fraction < 1) && (ent->s.origin[2] != neworg[2]))
			{
				neworg[2] = ent->s.origin[2];	// reset z
				move[2] = 0;					// RSP 102600
				trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, neworg, ent, MASK_MONSTERSOLID);
			}

			// fly monsters don't enter water voluntarily
			if (ent->flags & FL_FLY)
			{
				if (ent->waterlevel == WL_DRY)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);
					if (contents & MASK_WATER)
						return false;
				}
			}

			// swim monsters don't exit water voluntarily
			if ((ent->flags & FL_SWIM) && (move[2] > 0))	// RSP 090200 - add check for moving up
			{
				contents = gi.pointcontents(trace.endpos);
				if (!(contents & MASK_WATER))
					trace.endpos[2] = ent->s.origin[2]; // RSP 012099
			}

			// RSP 042500
			// Don't let Supervisors and Gbots come too close to the player.
			// Allow movement away from the player, but not toward.

			if (((ent->identity == ID_SUPERVISOR) || (ent->identity == ID_GBOT)) &&
				ent->enemy			&&
				ent->enemy->client	&&
				(trace.fraction == 1))
			{
				float dist_to_player_before,dist_to_player_after;	// distance measurements

				VectorSubtract (oldorg, ent->enemy->s.origin, test);
				dist_to_player_before = VectorLength (test);
				VectorSubtract (trace.endpos, ent->enemy->s.origin, test);
				dist_to_player_after = VectorLength (test);

				if (dist_to_player_before > dist_to_player_after)	// if true, he's moving closer
					if (dist_to_player_after <= 400)				// check for being inside the limit
						return false;
			}

			if (ent->identity == ID_GREAT_WHITE)
			{
				// RSP 071200 - further shark movement check to keep him out of the walls
				// We have to do this because gi.trace will FAIL at this task sometimes, and give
				// a trace.fraction of 1 when it really should have given a trace.fraction < 1. Bad ID.

				sharkfits = false;
				if (trace.fraction == 1)
				{
//					sharkfits = CheckSharkPoint(ent);

					// RSP 092000 - I'm not sure why this was the way it was, but instead of
					// checking where the shark currently is, which is what it was doing, it should
					// be checking the spot where the shark wants to go. So hold the current origin
					// in v and update it to the end of the trace, do the check, then restore the
					// current origin. If the shark fits in the new spot, sharkfits will be TRUE.
					// Otherwise, it will be FALSE.

					VectorCopy(ent->s.origin,test);			// save the origin
					VectorCopy(trace.endpos,ent->s.origin);	// move shark to new spot
					sharkfits = CheckSpawnPoint(ent);		// see if shark fits in new spot
					VectorCopy(test,ent->s.origin);			// restore
				}

				// RSP 030599
				// At least with the shark, once his bounding box is inside a wall, gi.trace() will not
				// tell you that you hit the wall. You have to check to see if the start of the trace--
				// which goes from the corners of the box--is inside a solid.

				if (sharkfits || ((trace.fraction == 1) && (doitanyway || !trace.startsolid)))	// RSP 030599 Stay out of walls
				{
					VectorCopy (trace.endpos, ent->s.origin);

					if (relink)
					{
						gi.linkentity (ent);
						G_TouchTriggers (ent);
					}
					return true;
				}
			}
			else	// something other than the shark
				if ((trace.fraction == 1) && !trace.startsolid)
				{
					VectorCopy (trace.endpos, ent->s.origin);

					if (relink)
					{
						gi.linkentity (ent);
						G_TouchTriggers (ent);
					}
					return true;
				}

			if (!ent->enemy)
				break;
		}
		
		return false;
	}

//  push down from a step height above the wished position

	if (!(ent->monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy (neworg, end);
	end[2] -= stepsize*2;

	trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}

	// don't go in to water
	if (ent->waterlevel == WL_DRY)
	{
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + ent->mins[2] + 1;	
		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if (ent->flags & FL_PARTIALGROUND)
		{
			VectorAdd (ent->s.origin, move, ent->s.origin);
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			ent->groundentity = NULL;
			return true;
		}
	
		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ent->s.origin);

	if (!M_CheckBottom (ent))
	{
		if (ent->flags & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct

			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			return true;
		}
		VectorCopy (oldorg, ent->s.origin);
		return false;
	}

	if (ent->flags & FL_PARTIALGROUND)
		ent->flags &= ~FL_PARTIALGROUND;

	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;

// the move is ok
	if (relink)
	{
		gi.linkentity (ent);
		G_TouchTriggers (ent);
	}

	return true;
}

//============================================================================

// RSP 012099: Delete the M_ChangeYaw_MoveExt() and M_ChangeYaw_ChkExt() functions.

//============================================================================

// RSP 012099: tables of values to add to origin of gwhite to test turning possibilities

#define GW_TURN_SHORT 100
#define GW_TURN_LONG  140

float gw_turnleft[8][2] =
{
	GW_TURN_SHORT,GW_TURN_SHORT,
	0,GW_TURN_LONG,
	-GW_TURN_SHORT,GW_TURN_SHORT,
	-GW_TURN_LONG,0,
	-GW_TURN_SHORT,-GW_TURN_SHORT,
	0,-GW_TURN_LONG,
	GW_TURN_SHORT,-GW_TURN_SHORT,
	GW_TURN_LONG,0
};

float gw_turnright[8][2] =
{
	GW_TURN_LONG,0,
	GW_TURN_SHORT,GW_TURN_SHORT,
	0,GW_TURN_LONG,
	-GW_TURN_SHORT,GW_TURN_SHORT,
	-GW_TURN_LONG,0,
	-GW_TURN_SHORT,-GW_TURN_SHORT,
	0,-GW_TURN_LONG,
	GW_TURN_SHORT,-GW_TURN_SHORT
};


// RSP 012099: End of tables

/* RSP 071400 - Instead of varying the bounding box, let's try a square one 
				for a while and see how the shark behaves.

// RSP 012099: M_ChangeBBox changes the bounding box for the shark

void M_ChangeBBox(edict_t *ent)
{
	float	yaw = ent->s.angles[YAW];
	vec3_t	oldmins,oldmaxs;		// RSP 032699

	VectorCopy(ent->mins,oldmins);
	VectorCopy(ent->maxs,oldmaxs);

	if ((67.5 < yaw && yaw < 112.5) || (247.5 < yaw && yaw < 292.5))
	{
		ent->mins[0] = -35;
		ent->mins[1] = -70;
		ent->maxs[0] = 35;
		ent->maxs[1] = 70;
	}
	else if ((0 <= yaw && yaw < 22.5) || (337.5 < yaw && yaw <= 360) || (157.5 < yaw && yaw <= 202.5))
	{
		ent->mins[0] = -70;
		ent->mins[1] = -35;
		ent->maxs[0] = 70;
		ent->maxs[1] = 35;
	}
	else 
	{
		ent->mins[0] = ent->mins[1] = -50;
		ent->maxs[0] = ent->maxs[1] = 50;
	}

	ent->mins[2] = -30;
	ent->maxs[2] = 19;

	// RSP 032699 - check BBOX corners for insolid
	// RSP 071100 - replace with call to CheckSpawnPoint()

	if (CheckSpawnPoint(ent))
		return;

	// If we got here, at least one BBOX corner is in a solid, so we can't
	// switch to the new BBOX right now. It will happen later when the shark
	// has moved away from his encroachment on a wall.

	VectorCopy(oldmins,ent->mins);
	VectorCopy(oldmaxs,ent->maxs);
}
 */

/*
===============
M_ChangeYaw

===============
*/
qboolean M_ChangeYaw (edict_t *ent) // RSP 012099 Need to know if successful or not
{
	float	ideal;
	float	current;
	float	move;
	float	speed;
	float	yaw;			// RSP 012099
	qboolean turning_right;	// RSP 012099

//	int old_index;			// RSP 032699 - no longer used

	// Flag for count of redoing the pivots
//	int redoing_pivot = 0;	// RSP 012099: not needed
	
	current = anglemod(ent->s.angles[YAW]);
	ideal = ent->ideal_yaw;

	// RSP 012099: Take care of minute differences (from here to // END below)

	move = ideal - current;

	if (move == 0)
	{
		// RSP 032699
		switch(ent->identity)
		{
		case ID_GREAT_WHITE: 
			gwhite_straightenout(ent);		// Stop turning frames if turning
			break;
		case ID_PLASMABOT:
		case ID_BEAMBOT:
		case ID_CLAWBOT:	// RSP 072900
			workerbot_straightenout(ent);	// RSP 072900 - Stop turning frames if turning
			break;
		case ID_FATMAN:	// RSP 083000 - reset need for fatman turning (idle) sound
			ent->state = 0;
			break;
		default:
			break;
		}
		return true;
	}

	if (move < -180)
		move += 360;
	else if (move > 180)
		move -= 360;

	if ((-11.0 <= move) && (move <= 11.0))	// RSP 041099
	{
		ent->s.angles[YAW] = ent->ideal_yaw;
		if (ent->identity == ID_GREAT_WHITE)
			gwhite_straightenout(ent);		// Stop turning frames if turning
		else if (ent->identity == ID_FATMAN);// RSP 083000 - reset need for fatman turning (idle) sound
			ent->state = 0;
		return true;
	}

	// End of taking care of minute differences

	speed = ent->yaw_speed;

	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	turning_right = (move < 0);	// RSP 032699

//RedoPivot:	
	// MRB 070498 - here is my attempt at pivot frames ...
	if (ent->monsterinfo.pivot)
		ent->monsterinfo.pivot (ent, move);

	// RSP 041099 - At this point, if ent is the shark, ent->turn_direction is set
	// to TD_LEFT, TD_RIGHT, or TD_STRAIGHT, done by gwhite_pivot.

	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

// RSP 012099: Delete the teamchain code

	// RSP 012099: Check pending turns for obstructions. Try intended direction. If blocked,
	//				try opposite direction. If both blocked, back up a bit and try the
	//				sequence until unblocked. If you can't break free after 25 tries, give
	//				up.
	//
	//				This code goes down to "// END Check pending turns for obstructions"

	if (ent->identity != ID_GREAT_WHITE)	// If not the shark, then simply set new YAW
	{
		ent->s.angles[YAW] = anglemod (current + move);

		if ((ent->identity == ID_FATMAN) && (ent->state == 0))
		{	// RSP 082900 - make turning (idle) sound if fatman
			gi.sound(ent,CHAN_BODY,gi.soundindex("fatman/idle.wav"),1,ATTN_NORM,0);
			ent->state = 1;
		}
	}
	else	// if the shark, then there's some extra math
	{
		vec3_t	corner;
		trace_t	t;
		int		c;

		if (ent->turn_direction == TD_STRAIGHT)
		{
			ent->s.angles[YAW] = anglemod (current + move);
			return true;
		}

		c = (int)current - 1;
		if (c < 0)
			c = 359;
		c /= 45;
		yaw = (int)anglemod (current + move);

		VectorCopy(ent->s.origin,corner);
		if (ent->turn_direction == TD_RIGHT)
		{
			corner[0] += gw_turnright[c][0];
			corner[1] += gw_turnright[c][1];
		}
		else	// LEFT
		{
			corner[0] += gw_turnleft[c][0];
			corner[1] += gw_turnleft[c][1];
		}

		t = gi.trace (ent->s.origin, NULL, NULL, corner, ent, MASK_MONSTERSOLID);

		if ((t.fraction < 1) && !(ent->flags & FL_ROTATEONLY))
		{
			vec3_t v;
			float  anglediff;

			// RSP 032699 - Up against a wall. Change ideal_yaw to align with the wall
			// keep moving forward
			vectoangles(t.plane.normal,v);
			anglediff = ent->s.angles[YAW] - v[YAW];
			if (anglediff < 0)
				anglediff += 360;
			if (anglediff >= 180)
			{
				anglediff = v[YAW] - 90;
				if (anglediff < 0)
					anglediff += 360;
				ent->ideal_yaw = anglediff; // turn to the left, align with wall
				if (fabs(ent->ideal_yaw - ent->s.angles[YAW]) > 10)
					ent->s.angles[YAW] += 10;
				else
					ent->s.angles[YAW] = ent->ideal_yaw;
			}
			else
			{
				ent->ideal_yaw = ((int)(v[YAW] + 90)) % 360; // turn to the right, align with wall
				if (fabs(ent->ideal_yaw - ent->s.angles[YAW]) > 10)
					ent->s.angles[YAW] -= 10;
				else
					ent->s.angles[YAW] = ent->ideal_yaw;
			}

		}
		else
			ent->s.angles[YAW] = anglemod (current + move);

		// Change pivot frames when turning
		move = ent->ideal_yaw - ent->s.angles[YAW];
		if (ent->ideal_yaw > ent->s.angles[YAW])
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (ent->monsterinfo.pivot)
			ent->monsterinfo.pivot(ent,move);
		
		// RSP 071400 - trying a fixed, square bounding box for a while
		// instead of changing it all the time.

		// Set new bounding box based on current YAW
		// There are 3 box shapes. Horizontal if heading EW, Vertical if heading NS,
		// and Square if heading NW,NE,SW,SE.

//		M_ChangeBBox(ent);	// Change bounding box
	}

	return true;

	// END Check pending turns for obstructions
}


// RSP 012099:  Completely changed SV_StepDirection so that failed attempts by
//				the shark allow him to back off and try another direction
/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
qboolean SV_StepDirection (edict_t *ent, float yaw, float dist)
{
	vec3_t		move,oldorigin;
	float		delta;
	qboolean	results = false;
	qboolean	obstruction = false;
	float		oldyaw; // store previous ideal yaw
	vec3_t		v_left,v_right,forward;
	vec3_t		neworg;
	trace_t		tr_ahead,tr_left,tr_right;
	float		anglediff;	// RSP 032699

	if (ent->identity == ID_GREAT_WHITE)
	{
		// RSP 032699 - First of all, before you do any of this, check the shark's
		// health. If low, he'll be sent after a jelly if there are any around.
		// If the shark is chasing a jelly, then make sure the ideal_yaw is properly
		// set, because the jelly moves around.

		gwhite_checkhealth(ent);	// Will set enemy to a jelly if health is low
									// and there's a jelly to be had.

		if (ent->enemy && (ent->enemy->identity == ID_JELLY))
		{
			VectorSubtract(ent->enemy->s.origin,ent->s.origin,move);
			r_vectoangles(move,forward);	// RSP 071100 - use more accurate version
											// in case the shark is covering a long distance
			yaw = forward[YAW];
		}
		
		oldyaw = ent->ideal_yaw;	// save previous ideal yaw
		ent->ideal_yaw = yaw;		// set new ideal yaw
		if (!M_ChangeYaw (ent))
		{
			ent->ideal_yaw = oldyaw;	// restore previous ideal yaw
			return results;				// Can't turn in that direction
		}

		// Look straight ahead to see if there's an obstruction.
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorNormalize(forward);
		VectorMA(ent->s.origin,GWHITE_LOOKAHEAD,forward,neworg);
		tr_ahead = gi.trace (ent->s.origin,ent->mins,ent->maxs,neworg,ent,MASK_MONSTERSOLID);

		// If something's obstructing the path, turn right or left.
		// Unless the obstruction is the enemy, then you go straight for him.
		// If you're forcing a rotation because the shark's in a corner, then don't change
		// to left or right rotation; leave it alone.
		if ((tr_ahead.fraction < 1) &&
			!(ent->enemy && (tr_ahead.ent == ent->enemy)) &&
			!(ent->flags & FL_ROTATEONLY))	// RSP 041099 - already rotating?
		{

			// Look ahead to the right
			VectorCopy(ent->s.angles,v_right);
			v_right[YAW] -= 90;
			if (v_right[YAW] < 0)
				v_right[YAW] += 360;
			AngleVectors (v_right, forward, NULL, NULL);
			VectorNormalize(forward);
			VectorMA(ent->s.origin,GWHITE_LOOKAHEAD*10,forward,neworg);
			tr_right = gi.trace (ent->s.origin,ent->mins,ent->maxs,neworg,ent,MASK_MONSTERSOLID);

			// Look ahead to the left
			VectorCopy(ent->s.angles,v_left);
			v_left[YAW] = ((int)(v_left[YAW] + 90)) % 360;
			AngleVectors (v_left, forward, NULL, NULL);
			VectorNormalize(forward);
			VectorMA(ent->s.origin,GWHITE_LOOKAHEAD*10,forward,neworg);
			tr_left = gi.trace (ent->s.origin,ent->mins,ent->maxs,neworg,ent,MASK_MONSTERSOLID);

			// Examine the results, looking for the most room, left or right

			if (tr_right.fraction < tr_left.fraction)
			{	// turn to the left
				ent->ideal_yaw = ((int)(ent->s.angles[YAW] + 90)) % 360;
				ent->s.angles[YAW] = ((int)(ent->s.angles[YAW] + 10)) % 360;
			}
			else	// turn to the right
			{
				anglediff = ent->s.angles[YAW] - 90;
				if (anglediff < 0)
					anglediff += 360;
				ent->ideal_yaw = anglediff; 
				anglediff = ent->s.angles[YAW] - 10;
				if (anglediff < 0)
					anglediff += 360;
				ent->s.angles[YAW] = anglediff; 
			}

			// Change pivot frames when turning
			// RSP 041099 - send the correct delta to pivot routine, otherwise the creature
			// will look like it's pivoting one way while turning the other.
			if (ent->monsterinfo.pivot)
			{
				delta = ent->ideal_yaw - ent->s.angles[YAW];
				if (delta > 0)
				{
					if (delta >= 180)
						delta -= 360;
				}
				else
				{
					if (delta <= -180)
						delta += 360;
				}
				ent->monsterinfo.pivot(ent,delta);
			}
		}

		// There's no obstruction, move the requested distance
		yaw = ent->s.angles[YAW]*M_PI*2 / 360;
		move[0] = cos(yaw)*dist;
		move[1] = sin(yaw)*dist;
		move[2] = 0;
		if (!SV_movestep(ent,move,false,true))
		{
			// RSP 040899 - Shark is stuck against a wall, so let him rotate until he
			// has a good forward direction and can get out.
			if (ent->flags & FL_ROTATEONLY)	// already rotating?
				ent->s.angles[YAW] = anglemod(ent->s.angles[YAW]+10);
			else	// stuck, but not rotating yet. check the pre-rotation counter
			{
				if (++(ent->count2) >= GW_MAXSTUCKTIME)
				{
					ent->count2 = 0;	// reset counter
					ent->ideal_yaw = anglemod(ent->s.angles[YAW]+180);
					ent->s.angles[YAW] = anglemod(ent->s.angles[YAW]+10);
					ent->flags |= FL_ROTATEONLY;
					ent->monsterinfo.currentmove = &gwhite_move_tleftstart;
				}
			}
		}
		else	// Free!
		{
			ent->count2 = 0;				// reset counter
			ent->flags &= ~FL_ROTATEONLY;	// no longer rotating in place
		}
		gi.linkentity (ent);
		G_TouchTriggers (ent);
		return true;
	}

	// Original code for monsters other than sharks

	ent->ideal_yaw = yaw;
	M_ChangeYaw (ent);
	
	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	VectorCopy (ent->s.origin, oldorigin);
	if (SV_movestep (ent, move, false, false))	// RSP 032699
	{
		delta = ent->s.angles[YAW] - ent->ideal_yaw;
		if ((delta > 45) && (delta < 315))
		{		// not turned far enough, so don't take the step
			VectorCopy (oldorigin, ent->s.origin);
		}
		gi.linkentity (ent);
		G_TouchTriggers (ent);
		return true;
	}
	gi.linkentity (ent);
	G_TouchTriggers (ent);
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom (edict_t *ent)
{
	ent->flags |= FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/

#define	DI_NODIR -1

void SV_NewChaseDir (edict_t *actor, edict_t *enemy, float dist)
{
	float	deltax,deltay;
	float	d[3];
	float	tdir, olddir, turnaround;
	vec3_t  a,v;	// RSP 032699

	//FIXME: how did we get here with no enemy
	if (!enemy)
		return;

	olddir = anglemod( (int)(actor->ideal_yaw/45)*45 );
	turnaround = anglemod(olddir - 180);

	deltax = enemy->s.origin[0] - actor->s.origin[0];
	deltay = enemy->s.origin[1] - actor->s.origin[1];

	// RSP 032699
	// Shark can't deal with only 8 directions. It has to be more
	// accurate than that.
	if (actor->identity == ID_GREAT_WHITE)
	{
		// RSP 032699 - if shark, only change ideal_yaw if not turning
		if (actor->turn_direction == TD_STRAIGHT)
		{
			qboolean oktomove = true;

			v[0] = deltax;
			v[1] = deltay;
			v[2] = 0;
			vectoangles(v,a);

			// Don't chase a player if the player is behind the shark.
			// If shark's chasing a jelly, let him continue
			if (enemy->identity != ID_JELLY)
				if (a[YAW] > actor->s.angles[YAW])
					if ((a[YAW] - actor->s.angles[YAW] > 90) && (a[YAW] - actor->s.angles[YAW] < 270))
						oktomove = false;
					else
						oktomove = true;
				else
					if ((actor->s.angles[YAW] - a[YAW] > 90) && (actor->s.angles[YAW] - a[YAW] < 270))
						oktomove = false;
					else
						oktomove = true;

			if (oktomove)
				SV_StepDirection(actor,a[YAW],dist);
			else
			{
				Forget_Player(actor);
				actor->monsterinfo.walk (actor);
			}
		}
		else
			SV_StepDirection(actor,actor->ideal_yaw,dist);
		return;
	}

	if (deltax > 10)
		d[1] = 0;
	else if (deltax < -10)
		d[1] = 180;
	else
		d[1] = DI_NODIR;
	if (deltay < -10)
		d[2] = 270;
	else if (deltay>10)
		d[2] = 90;
	else
		d[2] = DI_NODIR;

// try direct route
	if ((d[1] != DI_NODIR) && (d[2] != DI_NODIR))
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 225;	// RSP 012099: Typo (was 215)
		if ((tdir != turnaround) && SV_StepDirection(actor,tdir,dist))
			return;
	}

// try other directions

	if (((rand()&3) & 1) || (abs(deltay) > abs(deltax)))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if ((d[1] != DI_NODIR) &&
		(d[1] != turnaround) &&
		SV_StepDirection(actor,d[1],dist))
	{
		return;
	}

	if ((d[2] != DI_NODIR) &&
		(d[2] != turnaround) &&
		SV_StepDirection(actor,d[2],dist))
	{
		return;
	}

/* there is no direct path to the player, so pick another direction */

	if ((olddir != DI_NODIR) && SV_StepDirection(actor,olddir,dist))
		return;


	if (rand()&1) 	/*randomly determine direction of search*/
	{
		for (tdir = 0 ; tdir <= 315 ; tdir += 45)
			if ((tdir != turnaround) && SV_StepDirection(actor, tdir, dist))
				return;
	}
	else
	{
		for (tdir = 315 ; tdir >= 0 ; tdir -= 45)
			if ((tdir != turnaround) && SV_StepDirection(actor, tdir, dist))
				return;
	}

	if ((turnaround != DI_NODIR) && SV_StepDirection(actor, turnaround, dist))
		return;

	actor->ideal_yaw = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!M_CheckBottom (actor))
		SV_FixCheckBottom (actor);
}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough (edict_t *ent, edict_t *goal, float dist)
{
	int		i;
	vec3_t	a,v;	// RSP 032699
	float	adiff;	// RSP 032699
	
	for (i = 0 ; i < 3 ; i++)
	{
		if (goal->absmin[i] > ent->absmax[i] + dist)
			return false;
		if (goal->absmax[i] < ent->absmin[i] - dist)
			return false;
	}

	// Creature is close enough to its goal

	// Special handling for the shark

	if (ent->identity == ID_GREAT_WHITE)
	{
		// RSP 032699 - Shark can't nab goal if goal is out of the water
//		if (!(gi.pointcontents(goal->absmin) & (CONTENTS_SLIME|CONTENTS_WATER)))
		if (goal->waterlevel == WL_DRY)	// RSP 092000
			return false;

		// Is the goal in front of the shark? If not (off to the side), then
		// the shark can't nab you and must swim by

		VectorSubtract(goal->s.origin,ent->s.origin,v);
		vectoangles(v,a);
		adiff = anglemod(anglemod(ent->s.angles[YAW]) - anglemod(a[YAW]));
		if ((adiff > 45) || (adiff < 315))
			return false;
	}

	return true;
}


/*
======================
M_MoveToGoal
======================
*/
void M_MoveToGoal (edict_t *ent, float dist)
{
	edict_t *goal;

	// RSP 022099: If a water or land jelly, randomly change movement every so often
	// The jelly isn't interested in the player unless the player gets too close, then
	// the jelly flees.
	if (ent->identity == ID_JELLY)
	{
		if ((rand()&7) == 1) // change direction slightly every so often
		{
			ent->ideal_yaw += rand()&1 ? 15 : -15; // vary by 15 degrees left or right
			if (ent->ideal_yaw < 0)
				ent->ideal_yaw += 360;
			else if (ent->ideal_yaw >= 360)
				ent->ideal_yaw -= 360;
		}
		if (!SV_StepDirection (ent, ent->ideal_yaw, dist))
		{	// 180-degree turn if the jelly can't move forward
			ent->ideal_yaw += 180;
			if (ent->ideal_yaw < 0)
				ent->ideal_yaw += 360;
			else if (ent->ideal_yaw >= 360)
				ent->ideal_yaw -= 360;
			SV_StepDirection (ent, ent->ideal_yaw, dist);
		}
		return;
	}

	// RSP 041099: If a shark, randomly change movement every so often when the shark has
	// no enemy and is not currently doing a forced rotation to get out of a corner.
	if ((ent->identity == ID_GREAT_WHITE) &&
		 (random() < 0.05) &&
		!(ent->enemy) &&
		!(ent->flags & FL_ROTATEONLY) &&
		 (ent->count2 == 0) &&
		 (ent->turn_direction == TD_STRAIGHT))
	{
		ent->ideal_yaw += rand()&1 ? 10 : -10; // vary by 10 degrees left or right
		if (ent->ideal_yaw < 0)
			ent->ideal_yaw += 360;
		else if (ent->ideal_yaw >= 360)
			ent->ideal_yaw -= 360;
	}

	goal = ent->goalentity;
	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return;

// if the next step hits the enemy, return immediately
	if (ent->enemy && SV_CloseEnough (ent, ent->enemy, dist))
		return;

// bump around...
	if (ent->identity == ID_GREAT_WHITE)
	{
		SV_StepDirection (ent, ent->ideal_yaw, dist);
		if (ent->inuse)
			SV_NewChaseDir (ent, goal, dist);
	}
	else	// everything other than shark
		if (((rand()&3) == 1) || !SV_StepDirection (ent, ent->ideal_yaw, dist))
			if (ent->inuse)
				SV_NewChaseDir (ent, goal, dist);
}


/*
===============
M_walkmove
===============
*/
qboolean M_walkmove (edict_t *ent, float yaw, float dist)
{
	vec3_t move;
	
	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return false;

	yaw = yaw*M_PI*2 / 360;
	
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	return (SV_movestep(ent,move,true,false));	// RSP 072800 - simplify
}
