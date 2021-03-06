/*
 * Copyright (C) 2009 - 2010 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
 
#include "ScriptPCH.h"
#include "icecrown_citadel.h"

enum Yells
{
	SAY_ENTER_ZONE       =    -1000559,
	SAY_AGGRO            =    -1000560,
	SAY_BONE_STORM       =    -1000561,
	SAY_BONESPIKE_1      =    -1000562,
	SAY_BONESPIKE_2      =    -1000563,
	SAY_BONESPIKE_3      =    -1000564,
	SAY_KILL_1           =    -1000565,
	SAY_KILL_2           =    -1000566,
	SAY_DEATH            =    -1000567,
	SAY_BERSERK          =    -1000568,
};

enum Spells
{
	SPELL_SABER_SLASH_10_NORMAL	= 69055,
	SPELL_SABER_SLASH_25_NORMAL = 69055,
	SPELL_SABER_SLASH_10_HEROIC = 70814,
	SPELL_SABER_SLASH_25_HEROIC = 70814,
	SPELL_COLD_FLAME_10_NORMAL = 69146,
	SPELL_COLD_FLAME_25_NORMAL = 70823,
	SPELL_COLD_FLAME_10_HEROIC = 70824,
	SPELL_COLD_FLAME_25_HEROIC = 70825,
	SPELL_COLD_FLAME_SPAWN	   = 69138,
	SPELL_BONE_SPIKE_GRAVEYARD = 69057,
	SPELL_BONE_SPIKE_IMPALING  = 69065,
	SPELL_BONE_STORM_CHANNEL	=	69076,
	SPELL_BONE_STORM_10_NORMAL	=	69075,
	SPELL_BONE_STORM_25_NORMAL	=	70835,
	SPELL_BONE_STORM_10_HEROIC	=	70835,
	SPELL_BONE_STORM_25_HEROIC	=	70836,
	SPELL_BERSERK				=   26662,
};

enum NPC
{
	CREATURE_BONE_SPIKE	=	38711,
	CREATURE_COLD_FLAME	=	36672,
};


struct BoneSpikeAI : public Scripted_NoMovementAI
{
    BoneSpikeAI(Creature *pCreature) : Scripted_NoMovementAI(pCreature)
    {
        BoneSpikeGUID = 0;
    }

    uint64 BoneSpikeGUID;

    void SetPrisoner(Unit* uPrisoner)
    {
        BoneSpikeGUID = uPrisoner->GetGUID();
    }

    void Reset()
	{ 
		BoneSpikeGUID = 0; 
	}

    void JustDied(Unit *killer)
    {
        if (killer->GetGUID() != me->GetGUID())

        if (BoneSpikeGUID)
        {
            Unit* Bone = Unit::GetUnit((*me), BoneSpikeGUID);
            if (Bone)
                Bone->RemoveAurasDueToSpell(SPELL_BONE_SPIKE_IMPALING);
        }
    }

	void KilledUnit(Unit *victim)
    {
        if (victim->GetGUID() != me->GetGUID())

        if (BoneSpikeGUID)
        {
            Unit* Bone = Unit::GetUnit((*me), BoneSpikeGUID);
            if (Bone)
                Bone->RemoveAurasDueToSpell(SPELL_BONE_SPIKE_IMPALING);
        }
    }

    void UpdateAI(const uint32 diff)
    {
	  Unit* temp = Unit::GetUnit((*me),BoneSpikeGUID);
        if ((temp && temp->isAlive() && !temp->HasAura(SPELL_BONE_SPIKE_IMPALING)) || !temp)
            me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
    }
};

struct Boss_Lord_MarrowgarAI : public ScriptedAI
{
    Boss_Lord_MarrowgarAI(Creature *pCreature) : ScriptedAI(pCreature)
    {
     pInstance = pCreature->GetInstanceData();
    }
    
    ScriptedInstance* pInstance;

	uint32 m_uiSaberSlashTimer;
	uint32 m_uiBoneSpikeGraveyardTimer;
	uint32 m_uiBoneStormChanelTimer;
	uint32 m_uiBoneStormTimer;
	uint32 m_uiBoneStormRemoveTimer;
	uint32 m_uiColdFlameTimer;
	uint32 m_uiBerserkTimer;

	bool Intro;

    void Reset()
    {
		m_uiSaberSlashTimer = 6000;
		m_uiBoneSpikeGraveyardTimer	= 15000;
		m_uiColdFlameTimer	= 15000;
		m_uiBoneStormChanelTimer = 30000;
		m_uiBoneStormTimer = 1500;
		m_uiBoneStormRemoveTimer = 20000;
		m_uiBerserkTimer = 600000;

		me->SetSpeed(MOVE_RUN, 2.5f, true);
		me->SetSpeed(MOVE_WALK, 1.8f, true);

		Intro = false;

		if (pInstance)
            pInstance->SetData(DATA_MARROWGAR_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* who)
    {
		DoScriptText(SAY_AGGRO, me);

		if (pInstance)
            pInstance->SetData(DATA_MARROWGAR_EVENT, IN_PROGRESS);
    }

	void JustDied(Unit* killer)
    {  
		DoScriptText(SAY_DEATH, me);

		if (pInstance)
            pInstance->SetData(DATA_MARROWGAR_EVENT, DONE);
    }

	void KilledUnit(Unit *victim)
    {
        DoScriptText(RAND(SAY_KILL_1,SAY_KILL_2), me);
    }

	 void MoveInLineOfSight(Unit *who)
     {
        if (!Intro && me->IsWithinDistInMap(who,55.0f))
        {
                DoScriptText(SAY_ENTER_ZONE, me);
                Intro = true;
        }

        ScriptedAI::MoveInLineOfSight(who);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!UpdateVictim())
            return;
	
		if (m_uiBerserkTimer <= uiDiff)
		{
			DoScriptText(SAY_BERSERK, me);
			DoCast(SPELL_BERSERK);
			m_uiBerserkTimer = 1000000;
		} 
		else m_uiBerserkTimer -= uiDiff;

				if (getDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
				{
					if (!me->HasAura(SPELL_BONE_STORM_CHANNEL))
		                {
			                if (m_uiBoneSpikeGraveyardTimer < uiDiff)
                                {
									DoScriptText(RAND(SAY_BONESPIKE_1,SAY_BONESPIKE_2,SAY_BONESPIKE_3), me);
				                    uint32 count = RAID_MODE(1,3,1,3); 
                                    for (uint8 i = 1; i <= count; i++)
                                        {
					                        Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true);
					                        if (pTarget && !pTarget->HasAura(SPELL_BONE_SPIKE_IMPALING))
						                        {
							                        Creature* Bone = me->SummonCreature(CREATURE_BONE_SPIKE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 999999);
							                        CAST_AI(BoneSpikeAI, Bone->AI())->SetPrisoner(pTarget);
							                        Bone->CastSpell(pTarget, SPELL_BONE_SPIKE_IMPALING, true);
						                        }
				                            }
				                            m_uiBoneSpikeGraveyardTimer = 15000;
							     } else m_uiBoneSpikeGraveyardTimer -= uiDiff;

			if (m_uiColdFlameTimer <= uiDiff)
				{
						me->SummonCreature(CREATURE_COLD_FLAME, me->GetPositionX()+20, me->GetPositionY()+20, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 8000);
						me->SummonCreature(CREATURE_COLD_FLAME, me->GetPositionX()-20, me->GetPositionY()-20, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 8000);
						me->SummonCreature(CREATURE_COLD_FLAME, me->GetPositionX()+20, me->GetPositionY()-20, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 8000);
						me->SummonCreature(CREATURE_COLD_FLAME, me->GetPositionX()-20, me->GetPositionY()+20, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 8000);
						m_uiColdFlameTimer = 15000;
				} 
				else m_uiColdFlameTimer -= uiDiff;
					}
				}

				if (getDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
				{
				if (m_uiBoneSpikeGraveyardTimer < uiDiff)
				{
                    DoScriptText(RAND(SAY_BONESPIKE_1,SAY_BONESPIKE_2,SAY_BONESPIKE_3), me);
					uint32 count = RAID_MODE(1,3,1,3);
					for (uint8 i = 1; i <= count; i++)
					{
		            Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true);
					if (pTarget && !pTarget->HasAura(SPELL_BONE_SPIKE_IMPALING))
					{
						Creature* Bone = me->SummonCreature(CREATURE_BONE_SPIKE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 999999);
						CAST_AI(BoneSpikeAI, Bone->AI())->SetPrisoner(pTarget);
						Bone->CastSpell(pTarget, SPELL_BONE_SPIKE_IMPALING, true);
					}
				}
				m_uiBoneSpikeGraveyardTimer = 15000;
            } else m_uiBoneSpikeGraveyardTimer -= uiDiff;
				}

		if (!me->HasAura(SPELL_BONE_STORM_CHANNEL))
		{
			if (m_uiBoneStormChanelTimer <= uiDiff)
			{
				DoCast(SPELL_BONE_STORM_CHANNEL);
			    DoScriptText(SAY_BONE_STORM, me);
				m_uiBoneStormChanelTimer = 30000;
			}
			else m_uiBoneStormChanelTimer -= uiDiff;

		if (m_uiSaberSlashTimer <= uiDiff)
		{
            if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
			{
			    DoCast(RAID_MODE(SPELL_SABER_SLASH_10_NORMAL,SPELL_SABER_SLASH_25_NORMAL,SPELL_SABER_SLASH_10_HEROIC,SPELL_SABER_SLASH_10_HEROIC));
			    m_uiSaberSlashTimer = 6000;
			}
		} 
		else m_uiSaberSlashTimer -= uiDiff;

		}

		if (me->HasAura(SPELL_BONE_STORM_CHANNEL))
		{
			if (m_uiBoneStormTimer <= uiDiff)
			{
				DoCastAOE(RAID_MODE(SPELL_BONE_STORM_10_NORMAL,SPELL_BONE_STORM_25_NORMAL,SPELL_BONE_STORM_10_HEROIC,SPELL_BONE_STORM_25_HEROIC));
				m_uiBoneStormTimer = 1500;
			}
			else m_uiBoneStormTimer -= uiDiff;

			if (m_uiBoneStormRemoveTimer <= uiDiff)
			{
                me->RemoveAurasDueToSpell(SPELL_BONE_STORM_CHANNEL);
				m_uiBoneStormRemoveTimer = 20000;
			}
			else m_uiBoneStormRemoveTimer -= uiDiff;
		}

		DoMeleeAttackIfReady();
	}
};

struct Cold_FlameAI : public ScriptedAI
{
	Cold_FlameAI(Creature *pCreature) : ScriptedAI(pCreature)
    {
     m_pInstance = pCreature->GetInstanceData();
    }
	ScriptedInstance* m_pInstance;
	uint32 m_uiColdFlameTimer;
    void Reset()
    {
        float x, y, z;
        me->GetNearPoint(me, x, y, z, 1, 50, M_PI*2*rand_norm());
        me->GetMotionMaster()->MovePoint(0, x, y, z);
        me->SetVisibility(VISIBILITY_OFF);
		DoCast(me, RAID_MODE(SPELL_COLD_FLAME_10_NORMAL,SPELL_COLD_FLAME_25_NORMAL,SPELL_COLD_FLAME_10_HEROIC,SPELL_COLD_FLAME_25_HEROIC));
		me->SetReactState(REACT_PASSIVE);
		me->SetSpeed(MOVE_WALK, 1.5f, true);
		m_uiColdFlameTimer = 1000;
    }
	void UpdateAI(const uint32 uiDiff)
    {
		if(m_uiColdFlameTimer <= uiDiff)
		{
			DoCast(me, RAID_MODE(SPELL_COLD_FLAME_10_NORMAL,SPELL_COLD_FLAME_25_NORMAL,SPELL_COLD_FLAME_10_HEROIC,SPELL_COLD_FLAME_25_HEROIC));
			m_uiColdFlameTimer = 1000;
		} 
		else m_uiColdFlameTimer -= uiDiff;
		
	}
};

CreatureAI* GetAI_Cold_Flame(Creature* pCreature)
{
    return new Cold_FlameAI(pCreature);
}

CreatureAI* GetAI_Boss_Lord_Marrowgar(Creature* pCreature)
{
    return new Boss_Lord_MarrowgarAI(pCreature);
}

CreatureAI* GetAI_BoneSpike(Creature* pCreature)
{
    return new BoneSpikeAI(pCreature);
}

void AddSC_boss_marrowgar()
{
    Script* NewScript;
    NewScript = new Script;
    NewScript->Name = "Boss_Lord_Marrowgar";
    NewScript->GetAI = &GetAI_Boss_Lord_Marrowgar;
    NewScript->RegisterSelf();

	NewScript = new Script;
    NewScript->Name = "Cold_Flame";
    NewScript->GetAI = &GetAI_Cold_Flame;
    NewScript->RegisterSelf();	

	NewScript = new Script;
    NewScript->Name = "BoneSpike";
    NewScript->GetAI = &GetAI_BoneSpike;
    NewScript->RegisterSelf();	
}
