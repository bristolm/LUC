/* Generic LUC routine prototypes
*/

void _DrawLaser(vec3_t start, vec3_t end);
void _LaserBoxTraceMe(edict_t *self);
void _LaserTraceBBOX(vec3_t origin, vec3_t mins, vec3_t maxs);
void _shuffleAngles(vec3_t angles);
void M_ChangePitch_ToIdeal(edict_t *ent);
edict_t *_findlifeinradius (edict_t *requester, edict_t *from, vec3_t org, float rad, 
							qboolean want_clients, qboolean want_monsters, qboolean want_deadstuff,
							float *distance);
void _MoveWithLockedEnt(edict_t *self);
void _LockToEnt(edict_t *lockee,edict_t *anchor);
void _AlignDirVelocity(edict_t *);
void _SetBaseAnglesFromPlaneNormal(vec3_t, edict_t *);
//qboolean _MassageAngles(vec3_t, vec3_t, vec3_t, float, float);	// RSP 072100 - not used
void _ExtendWithBounds(vec3_t start, float dist, vec3_t dir, vec3_t max, vec3_t min, vec3_t end);
void Forget_Player(edict_t *);					// RSP 050199
edict_t *G_Find_Waypoint(edict_t *,qboolean);	// RSP 050199
qboolean wellInfront (edict_t *, edict_t *, float);	// MRB 071999

//JBM 112209 - Added these 2 for use in the speaker sequencer
char **expandarray(char *, int);
int countcommas(char *);

//MRB 121598 - Added for some rotational translation help
#define APPLY_OFFSET	1
#define FIND_OFFSET		(-1)

#define _FindAngleOffset(r, o, s)	_AngleTranslation(r,o,s,FIND_OFFSET)
#define _ApplyAngleOffset(s,o, r)	_AngleTranslation(s,o,r,APPLY_OFFSET)

// WPO 081099 Added some general utilities (in luc_utils.c)
qboolean G_EntExists(edict_t *ent);
qboolean G_ClientNotDead(edict_t *ent);
qboolean G_ClientInGame(edict_t *ent);
void G_Spawn_Splash(int type, int count, int color, vec3_t start, vec3_t movdir, vec3_t origin);
void G_Spawn_Models(int type, short rec_no, vec3_t start, vec3_t endpos, vec3_t offset, vec3_t origin);
void G_Spawn_Trails(int type, vec3_t start, vec3_t endpos, vec3_t origin);
void G_Spawn_Sparks(int type, vec3_t start, vec3_t movdir, vec3_t origin);
void G_Spawn_Explosion(int type, vec3_t start, vec3_t origin);
float G_EntDist(edict_t *ent1, edict_t *ent2);
void G_Make_Debris(edict_t *ent, char *model1, char *model2, char *model3);
qboolean G_ContentsCheck (edict_t *ent, int contents);
//void Flyer_TouchTriggers (edict_t *flyer, float radius);  // WPO 100699 New util function
qboolean G_CheckGround(edict_t *ent);   // WPO 011199
void R_ProjectSource(vec3_t,vec3_t,vec3_t,vec3_t,vec3_t,vec3_t);	// RSP 072100
void Dump_Entity(edict_t *);	// RSP 080500
void Dump_Trace(trace_t *);		// RSP 090200
