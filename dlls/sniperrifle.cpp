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
#include "soundent.h"
#include "gamerules.h"

enum sniperrifle_e
{
	SNIPERRIFLE_LONGIDLE = 0,
	SNIPERRIFLE_IDLE1,
	SNIPERRIFLE_RELOAD,
	SNIPERRIFLE_DEPLOY,
	SNIPERRIFLE_FIRE1,
	SNIPERRIFLE_FIRE2,
	SNIPERRIFLE_FIRE3,
};

LINK_ENTITY_TO_CLASS( weapon_sniperrifle, CSniperrifle );

//=========================================================
//=========================================================

void CSniperrifle::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_sniperrifle" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), "models/w_m40a1.mdl" );
	m_iId = WEAPON_SNIPERRIFLE;

	m_iDefaultAmmo = SNIPERRIFLE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

void CSniperrifle::Precache( void )
{
	PRECACHE_MODEL( "models/v_m40a1.mdl" );
	PRECACHE_MODEL( "models/w_m40a1.mdl" );
	PRECACHE_MODEL( "models/p_m40a1.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" ); // brass shellTE_MODEL

	PRECACHE_MODEL( "models/w_m40a1clip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/sniper_reload_first_seq.wav" );
	PRECACHE_SOUND( "weapons/sniper_reload_second_seq.wav" );
	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
	PRECACHE_SOUND( "weapons/sniper_fire.wav" ); // H to the K

	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	m_usSNIPERRIFLE = PRECACHE_EVENT( 1, "events/sniper.sc" );
}

int CSniperrifle::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "762";
	p->iMaxAmmo1 = _762_MAX_CARRY;
	p->iMaxClip = SNIPERRIFLE_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SNIPERRIFLE;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

int CSniperrifle::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
		WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CSniperrifle::Deploy()
{
	return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPERRIFLE_DEPLOY, "sniperrifle" );
}

void CSniperrifle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE; // cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( m_iClip )
		SendWeaponAnim( SNIPERRIFLE_IDLE1 );
	else
		SendWeaponAnim( SNIPERRIFLE_IDLE1 );
}

void CSniperrifle::PrimaryAttack()
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if ( m_iClip <= 0 )
	{
		DefaultReload( 16, SNIPERRIFLE_RELOAD, 1.5 );
		if ( m_pPlayer->pev->fov != 0 )
		{
			SecondaryAttack();
		}
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

#ifdef CLIENT_DLL
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_762, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_762, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSNIPERRIFLE, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSniperrifle::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
	}
	else if ( m_pPlayer->pev->fov != 20 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
	}

	pev->nextthink = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSniperrifle::Reload( void )
{
	int iResult = 0;

	if ( m_iClip < 16 )
	{
		iResult = DefaultReload( 16, SNIPERRIFLE_RELOAD, 1.5 );
	}

	if ( m_pPlayer->pev->fov != 0 )
	{
		SecondaryAttack();
	}

	if ( iResult )
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	}
}

void CSniperrifle::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		iAnim = SNIPERRIFLE_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = SNIPERRIFLE_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CSniperrifleAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_m40a1clip.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_m40a1clip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = ( pOther->GiveAmmo( AMMO_SNIPERRIFLECLIP_GIVE, "556", _556MM_MAX_CARRY ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_sniperrifleclip, CSniperrifleAmmoClip );
