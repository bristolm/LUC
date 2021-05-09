#include "g_local.h"


qboolean Warp_Use(edict_t *ent)
{
	vec3_t		start, forward, end;
	trace_t		tr;
	vec3_t		savepos1, savepos2;

	// locate our start point by adding our viewheight to our origin
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight;

	// give us our forward viewing angle
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);

	// multiply this forward unit vector by 8192, 
	// add it to the start position, and store this
	// endpoint in end
	VectorMA(start, 8192, forward, end);

	// trace a straight line, ignoring our entity
	// stop on basically anything else
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_PLAYERSOLID);

	// backup a bit from wherever we hit ...
	VectorMA(start, (tr.fraction * 8192) - 24, forward, end);	// RSP 070700 back up 24 instead of 100

	// and get the teleport point
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_PLAYERSOLID);

	// now teleport there

	// unlink to make sure it can't possibly interfere with KillBox

	gi.unlinkentity(ent);

	// make sure we don't do something silly like
	// try and materialize inside something
	VectorCopy(ent->s.origin,savepos1);
	VectorCopy(ent->s.old_origin,savepos2);
	VectorCopy(tr.endpos,ent->s.origin);
	VectorCopy(tr.endpos,ent->s.old_origin);

	if (!CheckSpawnPoint(ent))
	{
		HelpMessage(ent,"Cannot teleport to desired location!");	// RSP 091600 - replaces gi.centerprintf
		gi.centerprintf(ent,"Cannot teleport to desired location!");
		VectorCopy(savepos1,ent->s.origin);
		VectorCopy(savepos2,ent->s.old_origin);
		gi.linkentity(ent);	// RSP 070700 - need to do this, else game over
		return false;
	}

	// clear the velocity and hold them in place briefly
	VectorClear(ent->velocity);
	ent->client->ps.pmove.pm_time = 160>>3;		// hold time
	ent->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash on the player
	ent->s.event = EV_PLAYER_TELEPORT;

	// RSP 070700 - moved this down from up above, just in case you
	// weren't able to t-port after all.

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BOSSTPORT);
	gi.WritePosition (savepos1);
	gi.multicast(savepos1,MULTICAST_PVS);

	// kill anything at the destination
	KillBox(ent);

	// RSP 081700 - face back the way you came
	ent->s.angles[YAW] = anglemod(ent->s.angles[YAW] + 180);
	ent->client->ps.viewangles[YAW] = anglemod(ent->client->ps.viewangles[YAW] + 180);
	ent->client->v_angle[YAW] = anglemod(ent->client->v_angle[YAW] + 180);
	ent->client->ps.pmove.delta_angles[YAW] = ANGLE2SHORT(180);

	gi.linkentity(ent);

	return true;
}
