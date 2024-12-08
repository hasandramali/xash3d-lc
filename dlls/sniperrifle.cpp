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

enum sniper_e {
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER,
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


	PRECACHE_MODEL( "models/w_m40a1clip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/sniper_reload_first_seq.wav" );
	PRECACHE_SOUND( "weapons/sniper_reload_second_seq.wav" );
	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
	PRECACHE_SOUND( "weapons/sniper_fire.wav" );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	m_usSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
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
	return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "bow" );
}

void CSniperrifle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE; // cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	if ( m_iClip )
		SendWeaponAnim( SNIPER_HOLSTER );
	else
		SendWeaponAnim( SNIPER_HOLSTER );
}

void CSniperrifle::PrimaryAttack()
{
	if ( m_fInSpecialReload )
		return;

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if ( m_iClip <= 0 )
	{
		DefaultReload( 5, SNIPER_RELOAD1, 80.0 / 34 );
		if ( m_pPlayer->pev->fov != 0 )
		{
			SecondaryAttack();
		}
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		return;
	}

	if ( m_iClip > 0 )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_fire.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
		SendWeaponAnim( SNIPER_FIRE );
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

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

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = 1.75;

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

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
	pev->nextthink = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSniperrifle::Reload( void )
{
	int iResult;

	if ( m_pPlayer->pev->fov != 0 )
	{
		SecondaryAttack();
	}

	if (m_iClip == 0)
	{
		iResult = DefaultReload( 5, SNIPER_RELOAD1, 80.0 / 34 );
		m_fInSpecialReload = 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.25;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.25;
	}
	else
	{
		iResult = DefaultReload( SNIPERRIFLE_MAX_CLIP, SNIPER_RELOAD3, 2.25 );
	}
}

void CSniperrifle::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	if ( m_fInSpecialReload )
	{
		m_fInSpecialReload = 0;
		SendWeaponAnim( SNIPER_RELOAD2 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 49.0 / 27.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 27.0;
	}
	else
	{

		int iAnim;
		if (m_iClip <= 0)
		{
			iAnim = SNIPER_SLOWIDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
		}
		else
		{
			iAnim = SNIPER_SLOWIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 67.5 / 16;
		}

		SendWeaponAnim( iAnim );
	}

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CSniperAmmo : public CBasePlayerAmmo
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
		int bResult = ( pOther->GiveAmmo( AMMO_762BOX_GIVE, "762", _762_MAX_CARRY ) != -1 );
		if ( bResult )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_762, CSniperAmmo );
