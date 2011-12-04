/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Felwood
SD%Complete: 95
SDComment: Quest support: 4101, 4102
SDCategory: Felwood
EndScriptData */

/* ContentData
npcs_riverbreeze_and_silversky
EndContentData */

#include "ScriptPCH.h"

/*######
## npcs_riverbreeze_and_silversky
######*/

#define GOSSIP_ITEM_BEACON  "Please make me a Cenarion Beacon"

class npcs_riverbreeze_and_silversky : public CreatureScript
{
public:
    npcs_riverbreeze_and_silversky() : CreatureScript("npcs_riverbreeze_and_silversky") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            creature->CastSpell(player, 15120, false);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        uint32 eCreature = creature->GetEntry();

        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (eCreature == 9528)
        {
            if (player->GetQuestRewardStatus(4101))
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                player->SEND_GOSSIP_MENU(2848, creature->GetGUID());
            } else if (player->GetTeam() == HORDE)
            player->SEND_GOSSIP_MENU(2845, creature->GetGUID());
            else
                player->SEND_GOSSIP_MENU(2844, creature->GetGUID());
        }

        if (eCreature == 9529)
        {
            if (player->GetQuestRewardStatus(4102))
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                player->SEND_GOSSIP_MENU(2849, creature->GetGUID());
            } else if (player->GetTeam() == ALLIANCE)
            player->SEND_GOSSIP_MENU(2843, creature->GetGUID());
            else
                player->SEND_GOSSIP_MENU(2842, creature->GetGUID());
        }

        return true;
    }

};

/*######
## npc_niby_the_almighty
######*/
enum
{
    QUEST_KROSHIUS     = 7603,

    NPC_IMPSY          = 14470,

    SPELL_SUMMON_POLLO = 23056,

    SAY_NIBY_1         = -1000635,
    SAY_NIBY_2         = -1000636,
    EMOTE_IMPSY_1      = -1000637,
    SAY_IMPSY_1        = -1000638,
    SAY_NIBY_3         = -1000639
};

class npc_niby_the_almighty : public CreatureScript
{
public:
	npc_niby_the_almighty() : CreatureScript("npc_niby_the_almighty") { }

	CreatureAI* GetAI(Creature* pCreature)
	{
		return new npc_niby_the_almightyAI(pCreature);
	}

	bool OnQuestReward(Player* pPlayer, Creature* pCreature, const Quest* pQuest, uint32 slot)
	{
		if (pQuest->GetQuestId() == QUEST_KROSHIUS)
		{
			if (npc_niby_the_almightyAI* pNibyAI = CAST_AI(npc_niby_the_almightyAI, pCreature->AI()))
			{
				pNibyAI->StartEvent();
			}
		}
		return true;
	}

	struct npc_niby_the_almightyAI : public ScriptedAI
	{
		npc_niby_the_almightyAI(Creature* pCreature) : ScriptedAI(pCreature){ Reset(); }

		uint32 m_uiSummonTimer;
		uint8  m_uiSpeech;

		bool m_bEventStarted;

		void Reset()
		{
			m_uiSummonTimer = 500;
			m_uiSpeech = 0;

			m_bEventStarted = false;
		}

		void StartEvent()
		{
			Reset();
			m_bEventStarted = true;
			me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
		}

		void UpdateAI(const uint32 uiDiff)
		{
			if (m_bEventStarted)
			{
				if (m_uiSummonTimer <= uiDiff)
				{
					switch (m_uiSpeech)
					{
					case 1:
						me->GetMotionMaster()->Clear();
						me->GetMotionMaster()->MovePoint(0, 5407.19f, -753.00f, 350.82f);
						m_uiSummonTimer = 6200;
						break;
					case 2:
						DoScriptText(SAY_NIBY_1, me);
						m_uiSummonTimer = 3000;
						break;
					case 3:
						DoScriptText(SAY_NIBY_2, me);
						DoCast(me, SPELL_SUMMON_POLLO,true);
						m_uiSummonTimer = 2000;
						break;
					case 4:
						if (Creature* pImpsy = GetClosestCreatureWithEntry(me, NPC_IMPSY, 20.0))
						{
							DoScriptText(EMOTE_IMPSY_1, pImpsy);
							DoScriptText(SAY_IMPSY_1, pImpsy);
							m_uiSummonTimer = 2500;
						}
						else
						{
							m_uiSummonTimer = 40000;
							++m_uiSpeech;
						}
						break;
					case 5:
						DoScriptText(SAY_NIBY_3, me);
						m_uiSummonTimer = 40000;
						break;
					case 6:
						me->GetMotionMaster()->MoveTargetedHome();
						me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
						m_bEventStarted = false;
					}
					++m_uiSpeech;
				}
				else
					m_uiSummonTimer -= uiDiff;
			}
		}
	};
};

void AddSC_felwood()
{
    new npcs_riverbreeze_and_silversky();
    new npc_niby_the_almighty();
}
