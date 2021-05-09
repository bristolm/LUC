// LUC Personal Teleporter

#include	"g_local.h"
#include	"luc_teleporter.h"
#include	"luc_powerups.h"

void Cmd_Store_Teleport_f(edict_t *ent)
{
	if (!luc_ent_has_powerup(ent, POWERUP_TELEPORTER))
		return;

	VectorCopy(ent->s.origin, ent->client->teleport_origin);
	VectorCopy(ent->s.angles, ent->client->teleport_angles);

	ent->client->teleport_stored = true;

	HelpMessage(ent, "Teleport Location Stored!\n");	// RSP 091600 - replaces gi.centerprintf
//	gi.centerprintf(ent, "Teleport Location Stored!\n");
}


void Cmd_Teleport_f(edict_t *ent)
{
	int	i;

	if (!luc_ent_has_powerup(ent, POWERUP_TELEPORTER))
		return;

	if (!ent->deadflag)
	{
		if (ent->client->teleport_stored)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BOSSTPORT);
			gi.WritePosition (ent->s.origin);
			gi.multicast(ent->s.origin, MULTICAST_PVS);

			// unlink to make sure it can't possibly interfere with KillBox
			gi.unlinkentity(ent);

			VectorCopy(ent->client->teleport_origin, ent->s.origin);
			VectorCopy(ent->client->teleport_origin, ent->s.old_origin);
			ent->s.origin[2] += 10;

			// clear the velocity and hold them in place briefly
			VectorClear(ent->velocity);
			ent->client->ps.pmove.pm_time = 160>>3;		// hold time
			ent->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

			// draw the teleport splash on the player
			ent->s.event = EV_PLAYER_TELEPORT;

			// set angles
			for (i = 0; i < 3; i++)
				ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->teleport_angles[i] - ent->client->resp.cmd_angles[i]);

			VectorClear(ent->s.angles);
			VectorClear(ent->client->ps.viewangles);
			VectorClear(ent->client->v_angle);

			// kill anything at the destination
			KillBox(ent);

			gi.linkentity(ent);
		}
		else
		{
			HelpMessage(ent, "You don't have a teleport\nlocation stored!\n");	// RSP 091600 - replaces gi.centerprintf
//			gi.centerprintf(ent, "You don't have a\nteleport location stored!\n");
		}
	}
	else
	{
		HelpMessage(ent, "Can't teleport dead bodies!\n");	// RSP 091600 - replaces gi.centerprintf
//		gi.centerprintf(ent, "Sorry. Can't teleport dead bodies!\n");
	}
}
