/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#define	MEDKIT_BODYHIT_VOLUME 128
#define	MEDKIT_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_medkit, CMedkit )

enum medkit_e
{
	MEDKIT_IDLE = 0,
	MEDKIT_DRAW,
	MEDKIT_HOLSTER,
	MEDKIT_ATTACK1HIT,
	MEDKIT_ATTACK1MISS,
	MEDKIT_ATTACK2MISS,
	MEDKIT_ATTACK2HIT,
	MEDKIT_ATTACK3MISS,
#ifndef MEDKIT_IDLE_ANIM	
	MEDKIT_ATTACK3HIT
#else
	MEDKIT_ATTACK3HIT,
	MEDKIT_IDLE2,
	MEDKIT_IDLE3
#endif
};

void CMedkit::Spawn()
{
	Precache();
	m_iId = WEAPON_MEDKIT;
	SET_MODEL( ENT( pev ), "models/w_medkit.mdl" );
	m_iClip = -1;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.y = 255;

	FallInit();// get ready to fall down.
}

void CMedkit::Precache( void )
{
	PRECACHE_MODEL( "models/v_medkit.mdl" );
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_MODEL( "models/p_medkit.mdl" );
	PRECACHE_SOUND( "items/medshot4.wav" );

	m_usMedkit = PRECACHE_EVENT( 1, "events/medkit.sc" );
}

int CMedkit::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_MEDKIT;
	p->iWeight = MEDKIT_WEIGHT;
	return 1;
}

int CMedkit::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CMedkit::Deploy()
{
	return DefaultDeploy( "models/v_medkit.mdl", "models/p_medkit.mdl", MEDKIT_DRAW, "medkit" );
}

void CMedkit::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( MEDKIT_HOLSTER );
}

void Medkit_FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int		i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult	tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 2; j++ )
		{
			for( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CMedkit::PrimaryAttack()
{
	if( !DoHeal( 1 ) )
	{
#ifndef CLIENT_DLL
		SetThink( &CMedkit::DoHeal_f );
		SetNextThink( 0.1 );
#endif
	}
}

void CMedkit::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CMedkit::DoHeal_f( void )
{
	DoHeal( 0 );
}

int CMedkit::DoHeal( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				Medkit_FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usMedkit,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
		0, 0, 0 );
	}

	if( tr.flFraction >= 1.0 )
	{
		if( fFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
#ifdef MEDKIT_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ( ( m_iSwing++ ) % 2 ) + 1 )
		{
		case 0:
			SendWeaponAnim( MEDKIT_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( MEDKIT_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( MEDKIT_ATTACK3HIT );
			break;
		}

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

        // play thwack, smack, or dong sound
                float flVol = 1.0;
                int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();
			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
			pEntity->TakeHealth( gSkillData.medkitCapacity, DMG_GENERIC );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "items/medshot4.wav", 1, ATTN_NORM );
				//m_pPlayer->m_iWeaponVolume = MEDKIT_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "items/medshot4.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "items/mebshot4.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * MEDKIT_WALLHIT_VOLUME );

		SetThink( &CMedkit::Smack );
		SetNextThink( 0.2 );
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
	}
#ifdef MEDKIT_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

#ifdef MEDKIT_IDLE_ANIM
void CMedkit::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9 )
		{
			iAnim = MEDKIT_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
		}
		else
		{
			if( flRand > 0.5 )
			{
				iAnim = MEDKIT_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 30.0;
			}
			else
			{
				iAnim = MEDKIT_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
