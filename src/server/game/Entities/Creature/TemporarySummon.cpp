/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"
#include "eliteFactor.h"

TempSummon::TempSummon(SummonPropertiesEntry const *properties, Unit *owner) :
Creature(), m_Properties(properties), m_type(TEMPSUMMON_MANUAL_DESPAWN),
m_timer(0), m_lifetime(0)
{
    m_summonerGUID = owner ? owner->GetGUID() : 0;
    m_unitTypeMask |= UNIT_MASK_SUMMON;
}

Unit* TempSummon::GetSummoner() const
{
    return m_summonerGUID ? ObjectAccessor::GetUnit(*this, m_summonerGUID) : NULL;
}

void TempSummon::Update(uint32 diff)
{
    Creature::Update(diff);

    if (m_deathState == DEAD)
    {
        UnSummon();
        return;
    }
    switch(m_type)
    {
        case TEMPSUMMON_MANUAL_DESPAWN:
            break;
        case TEMPSUMMON_TIMED_DESPAWN:
        {
            if (m_timer <= diff)
            {
                UnSummon();
                return;
            }

            m_timer -= diff;
            break;
        }
        case TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT:
        {
            if (!isInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;

            break;
        }

        case TEMPSUMMON_CORPSE_TIMED_DESPAWN:
        {
            if (m_deathState == CORPSE)
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            break;
        }
        case TEMPSUMMON_CORPSE_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == CORPSE || m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            break;
        }
        case TEMPSUMMON_DEAD_DESPAWN:
        {
            if (m_deathState == DEAD)
            {
                UnSummon();
                return;
            }
            break;
        }
        case TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == CORPSE || m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        case TEMPSUMMON_TIMED_OR_DEAD_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat() && isAlive())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        default:
            UnSummon();
            sLog->outError("Temporary summoned creature (entry: %u) have unknown type %u of ", GetEntry(), m_type);
            break;
    }
}

void TempSummon::InitStats(uint32 duration)
{
    ASSERT(!isPet());

    m_timer = duration;
    m_lifetime = duration;

    if (m_type == TEMPSUMMON_MANUAL_DESPAWN)
        m_type = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

    Unit *owner = GetSummoner();

		sLog->outDetail("TempSummon, InitStats. mindmg: %i",
			GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE));

		CreatureTemplate* info = (CreatureTemplate*)this->GetCreatureInfo();
		if(info) {
			sLog->outDetail("Has info. eliteFactor: %f", info->eliteFactor);
			if(info->eliteFactor == 1.0f) {
				bool setEliteFactor = false;
				float newBaseEliteFactor, newEffectiveEliteFactor;
				// set creature's factor to its owner's.
				if(owner) {
					Creature* co = owner->ToCreature();
					if(co) {
						setEliteFactor = true;
						const CreatureTemplate* ci = co->GetCreatureInfo();
						if(!ci) {
							sLog->outError("TempSummon: no data for owner of %i", info->Entry);
						} else {
							newBaseEliteFactor = ci->eliteFactor;
						}
						newEffectiveEliteFactor = co->m_eliteFactor;
					}
				}	//owner
				if(!setEliteFactor) {
					// TODO: try to set it from the current map.
					// if we're in an instance, it should be doable.
					const CreatureData* data = GetCreatureData();
					if(!data) {
						sLog->outError("TempSummon: no data for creature %i", info->Entry);
					} else {
						const MapEntry* mapEntry = sMapStore.LookupEntry(data->mapid);
						if(!mapEntry) {
							sLog->outError("TempSummon: no entry for map %d", data->mapid);
						} else if(mapEntry->IsDungeon()) {
							const MapDifficulty* mapDiff = GetMapDifficultyData(data->mapid,
								REGULAR_DIFFICULTY);
							if(!mapDiff) {
								sLog->outError("TempSummon: no mapDiff for map %d", data->mapid);
							} else {
								setEliteFactor = true;
								newBaseEliteFactor = (float)mapDiff->maxPlayers;
								newEffectiveEliteFactor = newBaseEliteFactor /
									owner->GetMap()->GetPlayers().getSize();
							}
						}	//mapEntry
					}	//data
				}	//eliteFactorSet
				if(setEliteFactor) {
					info->eliteFactor = newBaseEliteFactor;
					m_eliteFactor = newEffectiveEliteFactor;
					sLog->outDetail("TempSummon %i: eliteFactor set to %f (base), %f (effective)",
						info->Entry, info->eliteFactor, m_eliteFactor);
					UpdateAllStats();
					SetHealth(GetMaxHealth());
					ResetPlayerDamageReq();
				}
			}	//info->eliteFactor == 1.0f
		}	//info

		sLog->outDetail("TempSummon, InitStats 2. mindmg: %i",
			GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE));

		if (owner && isTrigger() && m_spells[0])
    {
        setFaction(owner->getFaction());
        SetLevel(owner->getLevel());
        if (owner->GetTypeId() == TYPEID_PLAYER)
            m_ControlledByPlayer = true;

				// copied from SelectLevel
				CreatureBaseStats const* stats =
					sObjectMgr->GetCreatureBaseStats(owner->getLevel(), info->unit_class);

				// health
				uint32 basehp = stats->GenerateHealth(info);

				SetCreateHealth(basehp);
				SetMaxHealth(basehp);
				SetHealth(basehp);
				ResetPlayerDamageReq();

				//damage
				float damagemod = (owner->getLevel() / (float)info->maxlevel);

				SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, info->mindmg * damagemod);
				SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, info->maxdmg * damagemod);

				SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE,info->minrangedmg * damagemod);
				SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE,info->maxrangedmg * damagemod);

				SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, info->attackpower * damagemod);
				UpdateAllStats();
				SetHealth(GetMaxHealth());
				ResetPlayerDamageReq();
				sLog->outDetail("TempSummon, second update. mindmg: %i",
					GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE));
    }

    if (!m_Properties)
        return;

    if (owner)
    {
        if (uint32 slot = m_Properties->Slot)
        {
            if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
            {
                Creature *oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                if (oldSummon && oldSummon->isSummon())
                    oldSummon->ToTempSummon()->UnSummon();
            }
            owner->m_SummonSlot[slot] = GetGUID();
        }
    }

    if (m_Properties->Faction)
        setFaction(m_Properties->Faction);
    else if (IsVehicle()) // properties should be vehicle
        setFaction(owner->getFaction());
}

void TempSummon::InitSummon()
{
    Unit* owner = GetSummoner();
    if (owner)
    {
        if (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
            owner->ToCreature()->AI()->JustSummoned(this);
        if (IsAIEnabled)
            AI()->IsSummonedBy(owner);
    }
}

void TempSummon::SetTempSummonType(TempSummonType type)
{
    m_type = type;
}

void TempSummon::UnSummon(uint32 msTime)
{
    if (msTime)
    {
        ForcedUnsummonDelayEvent *pEvent = new ForcedUnsummonDelayEvent(*this);

        m_Events.AddEvent(pEvent, m_Events.CalculateTime(msTime));
        return;
    }

    //ASSERT(!isPet());
    if (isPet())
    {
        ((Pet*)this)->Remove(PET_SAVE_NOT_IN_SLOT);
        ASSERT(!IsInWorld());
        return;
    }

    Unit* owner = GetSummoner();
    if (owner && owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
        owner->ToCreature()->AI()->SummonedCreatureDespawn(this);

    AddObjectToRemoveList();
}

bool ForcedUnsummonDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    m_owner.UnSummon();
    return true;
}

void TempSummon::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    if (m_Properties)
        if (uint32 slot = m_Properties->Slot)
            if (Unit* owner = GetSummoner())
                if (owner->m_SummonSlot[slot] == GetGUID())
                    owner->m_SummonSlot[slot] = 0;

    //if (GetOwnerGUID())
    //    sLog->outError("Unit %u has owner guid when removed from world", GetEntry());

    Creature::RemoveFromWorld();
}

Minion::Minion(SummonPropertiesEntry const *properties, Unit *owner) : TempSummon(properties, owner)
, m_owner(owner)
{
    ASSERT(m_owner);
    m_unitTypeMask |= UNIT_MASK_MINION;
    m_followAngle = PET_FOLLOW_ANGLE;
}

void Minion::InitStats(uint32 duration)
{
    TempSummon::InitStats(duration);

    SetReactState(REACT_PASSIVE);

    SetCreatorGUID(m_owner->GetGUID());
    setFaction(m_owner->getFaction());

    m_owner->SetMinion(this, true);
}

void Minion::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    m_owner->SetMinion(this, false);
    TempSummon::RemoveFromWorld();
}

bool Minion::IsGuardianPet() const
{
    return isPet() || (m_Properties && m_Properties->Category == SUMMON_CATEGORY_PET);
}

Guardian::Guardian(SummonPropertiesEntry const *properties, Unit *owner) : Minion(properties, owner)
, m_bonusSpellDamage(0)
{
    memset(m_statFromOwner, 0, sizeof(float)*MAX_STATS);
    m_unitTypeMask |= UNIT_MASK_GUARDIAN;
    if (properties && properties->Type == SUMMON_TYPE_PET)
    {
        m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
        InitCharmInfo();
    }
}

void Guardian::InitStats(uint32 duration)
{
    Minion::InitStats(duration);

    InitStatsForLevel(m_owner->getLevel());

    if (m_owner->GetTypeId() == TYPEID_PLAYER && HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
        m_charmInfo->InitCharmCreateSpells();

    SetReactState(REACT_AGGRESSIVE);
}

void Guardian::InitSummon()
{
    TempSummon::InitSummon();

    if (m_owner->GetTypeId() == TYPEID_PLAYER
        && m_owner->GetMinionGUID() == GetGUID()
        && !m_owner->GetCharmGUID())
        m_owner->ToPlayer()->CharmSpellInitialize();
}

Puppet::Puppet(SummonPropertiesEntry const *properties, Unit *owner) : Minion(properties, owner)
{
    ASSERT(owner->GetTypeId() == TYPEID_PLAYER);
    m_owner = (Player*)owner;
    m_unitTypeMask |= UNIT_MASK_PUPPET;
}

void Puppet::InitStats(uint32 duration)
{
    Minion::InitStats(duration);
    SetLevel(m_owner->getLevel());
    SetReactState(REACT_PASSIVE);
}

void Puppet::InitSummon()
{
    Minion::InitSummon();
    if (!SetCharmedBy(m_owner, CHARM_TYPE_POSSESS))
        ASSERT(false);
}

void Puppet::Update(uint32 time)
{
    Minion::Update(time);
    //check if caster is channelling?
    if (IsInWorld())
    {
        if (!isAlive())
        {
            UnSummon();
            // TODO: why long distance .die does not remove it
        }
    }
}

void Puppet::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    RemoveCharmedBy(NULL);
    Minion::RemoveFromWorld();
}
