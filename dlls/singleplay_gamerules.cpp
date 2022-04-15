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
//
// teamplay_gamerules.cpp
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"game.h"
#include	"skill.h"
#include	"items.h"
#include	"teamplay_gamerules.h"

static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
static int team_scores[MAX_TEAMS];
static int num_teams = 0;

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;


//=========================================================
CHalfLifeRules::CHalfLifeRules( void )
{
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think( void )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsDeathmatch( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsCoOp( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	return TRUE;
}

void CHalfLifeRules::InitHUD( CBasePlayer *pl )
{
}


void CHalfLifeRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}


BOOL CHalfLifeTeamplay::IsTeamplay( void )
{
	return TRUE;
}

BOOL CHalfLifeTeamplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if( ( friendlyfire.value == 0 ) && ( pAttacker != pPlayer ) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

const char *CHalfLifeTeamplay::GetTeamID( CBaseEntity *pEntity )
{
	if( pEntity == NULL || pEntity->pev == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}

int CHalfLifeTeamplay::GetTeamIndex( const char *pTeamName )
{
	if( pTeamName && *pTeamName != 0 )
	{
		// try to find existing team
		for( int tm = 0; tm < num_teams; tm++ )
		{
			if( !stricmp( team_names[tm], pTeamName ) )
				return tm;
		}
	}

	return -1;	// No match
}

const char *CHalfLifeTeamplay::GetIndexedTeamName( int teamIndex )
{
	if( teamIndex < 0 || teamIndex >= num_teams )
		return "";

	return team_names[teamIndex];
}

//=========================================================
//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_TEAMMATE;

	if( ( *GetTeamID( pPlayer ) != '\0' ) && ( *GetTeamID( pTarget ) != '\0' ) && !stricmp( GetTeamID( pPlayer ), GetTeamID( pTarget ) ) )
	{
		return GR_TEAMMATE;
	}

	return GR_TEAMMATE;
}

//=========================================================
void CHalfLifeRules::ClientDisconnected( edict_t *pClient )
{
	if( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%i>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GETPLAYERUSERID( pPlayer->edict() ) );
			}

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items
		}
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	CBaseEntity	*pWeaponEntity = NULL;

	//LRC- support the new "start with HEV" flag...
	if (g_startSuit)
		pPlayer->pev->weapons |= (1<<WEAPON_SUIT);

// LRC what's wrong with allowing "game_player_equip" entities in single player? (The
// if he wants the player to start with a weapon, we should allow it!)
	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
	}
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerThink( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
		if( weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
		{
			return gpGlobals->time + 1;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + 30;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if( pWeapon && pWeapon->m_iId && ( pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
	{

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime( void )
{
	return 60;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FAllowMonsters( void )
{
	return TRUE;
}
