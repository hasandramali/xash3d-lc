// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//	  Tyler Lund <halflife@bubblemod.org>
//
// LICENSE										
//											  
//	  Permission is granted to anyone to use this  software  for  
//	  any purpose on any computer system, and to redistribute it  
//	  in any way, subject to the following restrictions:		 
//											  
//	  1. The author is not responsible for the  consequences  of  
//		  use of this software, no matter how awful, even if they  
//		  arise from defects in it.					  
//	  2. The origin of this software must not be misrepresented,  
//		  either by explicit claim or by omission.			  
//	  3. Altered  versions  must  be plainly marked as such, and  
//		  must  not  be  misrepresented  (by  explicit  claim  or  
//		  omission) as being the original software.			 
//	  3a. It would be nice if I got  a  copy  of  your  improved  
//		version  sent to halflife@bubblemod.org. 
//	  4. This notice must not be removed or altered.		  
//											  
// ---------------------------------------------------------------

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "BMOD_flyingcrowbar.h"

LINK_ENTITY_TO_CLASS( flying_crowbar, CFlyingCrowbar );

void CFlyingCrowbar::Spawn()
{
	if( !pev->owner )
		UTIL_Remove( this );

	Precache();

	// The flying crowbar is MOVETYPE_TOSS, and SOLID_BBOX.
	// We want it to be affected by gravity, and hit objects
	// within the game.
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->dmg = RANDOM_LONG( 60, 50 );

	// Use the world crowbar model.
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");

	// Set the origin and size for the HL engine collision
	// tables.
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, Vector(-4, -4, -4), Vector(4, 4, 4) );

	// Store the owner for later use. We want the owner to be able
	// to hit themselves with the crowbar. The pev->owner gets cleared
	// later to avoid hitting the player as they throw the crowbar.
	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	// Set the think funtion.
	SetThink( &CFlyingCrowbar::FlyThink );
	pev->nextthink = gpGlobals->time + 0.25;

	// Set the touch function.
	SetTouch( &CFlyingCrowbar::SpinTouch );
}

void CFlyingCrowbar::Precache()
{
	PRECACHE_MODEL( "models/w_crowbar.mdl" );

	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );

	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );

	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );
}

void CFlyingCrowbar::SpinTouch( CBaseEntity *pOther )
{
	if ( ENT(pOther->pev) == pev->owner )
		return;

	if ( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		ClearMultiDamage();
		   pOther->TraceAttack( pev, pev->dmg, pev->velocity.Normalize(), &tr, DMG_ALWAYSGIB );
		if ( m_hOwner != NULL )
			ApplyMultiDamage( pev, m_hOwner->pev );
		else
			ApplyMultiDamage( pev, pev );
	}

	// If we hit a player, make a nice squishy thunk sound. Otherwise
	// make a clang noise and throw a bunch of sparks.
	//if ( pOther->IsPlayer() )
	if ( pOther->pev->takedamage )
	{
		switch( RANDOM_LONG( 0, 2 ) )
		{
			case 0:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100);
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod2.wav", 1.0, ATTN_NORM, 0, 100);
				break;
			case 2:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod3.wav", 1.0, ATTN_NORM, 0, 100);
				break;
		}
	}
	else
	{
		switch( RANDOM_LONG( 0, 1 ) )
		{
			case 0:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit1.wav", 1.0, ATTN_NORM, 0, 100);
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit2.wav", 1.0, ATTN_NORM, 0, 100);
				break;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
			UTIL_Sparks( pev->origin );
		}
	}

	// Don't draw the flying crowbar anymore.
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;

	// Spawn a crowbar weapon
	CBasePlayerWeapon *pItem = (CBasePlayerWeapon *)Create( "weapon_crowbar", pev->origin , pev->angles, edict() );

	// remove the weaponbox after 2 mins
	pItem->pev->nextthink = gpGlobals->time + 120;
	pItem->SetThink( &CBasePlayerWeapon::Kill );
	pItem->pev->angles.x = 0;
	pItem->pev->angles.z = 0;
	pItem->pev->solid = SOLID_TRIGGER;
	pItem->pev->spawnflags |= SF_NORESPAWN; // never respawn

	UTIL_SetSize( pItem->pev, Vector( 0,0,0 ), Vector( 0,0,0 ) );

	// Get the unit vector in the direction of motion.
	Vector vecDir = pev->velocity.Normalize( );

	// Trace a line along the velocity vector to get the normal at impact.
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + vecDir * 100, dont_ignore_monsters, ENT(pev), &tr);

	// Throw the weapon box along the normal so it looks kinda
	// like a ricochet. This would be better if I actually
	// calcualted the reflection angle, but I'm lazy. :)
	pItem->pev->velocity = tr.vecPlaneNormal * 250;

	// Remove this flying_crowbar from the world.
	UTIL_Remove( this );
}

void CFlyingCrowbar::FlyThink( void )
{
	pev->owner = NULL;

	// Only think every .25 seconds.
	pev->nextthink = gpGlobals->time + 0.25;

	// Make a whooshy sound.
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, 120);

	// If the crowbar enters water, make some bubbles.
	if (pev->waterlevel)
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}
