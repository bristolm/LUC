#include "g_local.h"

/*
==============================================================================
PLAYER TRAIL
==============================================================================

This is a circular list containing the a list of points of where
the player has been recently.  It is used by monsters for pursuit.

.origin      the spot
.owner       forward link (not used)
.aiment      backward link (not used)
.angles[YAW] YAW from previous spot to this one
.timestamp   when the player was there
*/


#define	TRAIL_LENGTH	8

edict_t		*trail[TRAIL_LENGTH];
int         trail_head;             // index of oldest spot in the list
qboolean	trail_active = false;

#define NEXT(n)		(((n) + 1) & (TRAIL_LENGTH - 1))
#define PREV(n)		(((n) - 1) & (TRAIL_LENGTH - 1))


void PlayerTrail_Init (void)
{
	int		n;

	if (deathmatch->value /* FIXME || coop */)
		return;

	for (n = 0; n < TRAIL_LENGTH; n++)
	{
		trail[n] = G_Spawn();
		trail[n]->classname = "player_trail";
	}

	trail_head = 0;
	trail_active = true;
}


/* RSP 070600 - Added comments
   PlayerTrail_Add() adds the current player location to the list, in
   the slot indexed by trail_head. It stores the timestamp when the
   player was at this spot. A spot is added when the player can no longer
   see the previous spot in the list. It stores the YAW angle from the
   previous spot to this spot. This is used by anything following
   the trail, and presumably they've reached the previous spot, and want
   to know what YAW to use to get to the next spot.
 */

void PlayerTrail_Add (vec3_t spot)
{
	vec3_t temp;

	if (!trail_active)
		return;

	VectorCopy(spot,trail[trail_head]->s.origin);

	trail[trail_head]->timestamp = level.time;

	VectorSubtract(spot,trail[PREV(trail_head)]->s.origin,temp);
    trail[trail_head]->s.angles[YAW] = vectoyaw (temp);

	trail_head = NEXT(trail_head);
}


/* RSP 061799 - Not used
void PlayerTrail_New (vec3_t spot)
{
	if (!trail_active)
		return;

	PlayerTrail_Init ();
	PlayerTrail_Add (spot);
}
 */


/* RSP 070600 - Added comments
   PlayerTrail_PickFirst() returns the first trail spot that was laid
   down AFTER the one the tracker last recorded, IF that spot can be
   seen from the tracker's current location, OR if it can't, and the
   previous spot also can't be seen.
 */

edict_t *PlayerTrail_PickFirst (edict_t *self)
{
	int		marker;
	int		n;

	if (!trail_active)
		return NULL;

	for (marker = trail_head, n = TRAIL_LENGTH; n; n--)
	{
        if (trail[marker]->timestamp <= self->monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	if (visible(self, trail[marker]))
		return trail[marker];

	if (visible(self, trail[PREV(marker)]))
		return trail[PREV(marker)];

	return trail[marker];
}


/* RSP 070600 - Added Comments
   PlayerTrail_PickNext() returns the first trail spot that was laid
   down AFTER the one the tracker last recorded, even if that spot can't
   be seen from the tracker's current location.
 */

edict_t *PlayerTrail_PickNext (edict_t *self)
{
	int		marker;
	int		n;

	if (!trail_active)
		return NULL;

	for (marker = trail_head, n = TRAIL_LENGTH; n; n--)
	{
        if (trail[marker]->timestamp <= self->monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	// RSP 070700 - Fix ID bug of returning a valid trail spot if the monsterinfo.trail_time
	// is beyond ALL the timestamps. Bad ID.

	if (trail[marker]->timestamp > self->monsterinfo.trail_time)
		return trail[marker];
	else
		return NULL;
}

// RSP 070600
// Find the trail spot previous to the one given
edict_t *PlayerTrail_PickPrev(edict_t *spot)
{
    int marker;
    int n;

    if (spot == NULL)
        return trail[PREV(trail_head)];	// return most recent location

    // find index of current spot

    for (marker = trail_head, n = TRAIL_LENGTH; n; n--)
    {
        if (spot == trail[marker])
            break;
        marker = PREV(marker);
    }
    marker = PREV(marker);
    return trail[marker];
}


/* RSP 070600 - Added Comments
   PlayerTrail_LastSpot() returns the latest spot in the list.
 */

edict_t *PlayerTrail_LastSpot (void)
{
	return trail[PREV(trail_head)];
}
