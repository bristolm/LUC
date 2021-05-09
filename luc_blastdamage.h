// functions
void SpawnBlastDamage(char *model, trace_t *tr, float radius);

// 03/13/00 WPO Decreased duration and maximums from 10/50
#define DAMAGE_DURATION	5.0		// fade out in 5 seconds
#define MAX_BLASTS      25
#define BLAST_MODEL		"models/weaphits/bullet/tris.md2"

#define BULLET_RADIUS   2       // radius of bullet mark
