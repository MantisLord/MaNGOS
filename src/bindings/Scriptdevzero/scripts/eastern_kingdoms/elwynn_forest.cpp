/* Copyright (C) 2006 - 2011 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Elwynn_Forest
SD%Complete: 50
SDComment: Quest support: 1786
SDCategory: Elwynn Forest
EndScriptData */

/* ContentData
npc_henze_faulk
EndContentData */

#include "precompiled.h"
#include "patrol_ai.h"

#include <vector>

/*######
## npc_henze_faulk
######*/

#define SAY_HEAL    -1000187

struct MANGOS_DLL_DECL npc_henze_faulkAI : public ScriptedAI
{
    uint32 lifeTimer;
    bool spellHit;

    npc_henze_faulkAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    void Reset()
    {
        lifeTimer = 120000;
        m_creature->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        m_creature->SetStandState(UNIT_STAND_STATE_DEAD);   // lay down
        spellHit = false;
    }

    void MoveInLineOfSight(Unit* /*who*/) { }

    void UpdateAI(const uint32 diff)
    {
        if (m_creature->IsStandState())
        {
            if (lifeTimer < diff)
                m_creature->AI()->ResetToHome();
            else
                lifeTimer -= diff;
        }
    }

    void SpellHit(Unit *Hitter, const SpellEntry *Spellkind)
    {
        if (Spellkind->Id == 8593 && !spellHit)
        {
            DoCastSpellIfCan(m_creature,32343);
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);
            m_creature->SetUInt32Value(UNIT_DYNAMIC_FLAGS, 0);
            //m_creature->RemoveAllAuras();
            DoScriptText(SAY_HEAL, m_creature, Hitter);
            spellHit = true;
        }
    }

};
CreatureAI* GetAI_npc_henze_faulk(Creature* pCreature)
{
    return new npc_henze_faulkAI(pCreature);
}

/*####################
 * npc_creepy_child ##
 * #################*/


enum Children
{
    DANA        = 804,
    CAMERON     = 805,
    JOSE        = 811,
    ARON        = 810,
    LISA        = 807,
    JOHN        = 806
};

enum CreepySounds
{
    CTHUN_DEATH_IS_CLOSE = 8580,
    CTHUN_YOU_WILL_DIE = 8585,
};

static const uint32 m_ChildEntries[5] = { JOHN, JOSE, ARON, LISA, DANA };
// The AI for the middle child.
struct MANGOS_DLL_DECL npc_creepy_child : public npc_patrolAI
{
    npc_creepy_child(Creature* pCreature) : npc_patrolAI(pCreature, 0.f, true)
    {
        Reset();
    }
    
    float m_ChildOffsets[5][2];
    float direction;
    std::vector<Unit*> m_ChildList;
    
    float m_ChildRotation;
    
    uint32 m_uiChildCatchingTimer;
    
    void Reset()
    {
        npc_patrolAI::Reset();
        
        m_uiChildCatchingTimer = 8000;
        direction = 0;
        m_ChildList.clear();
        m_ChildRotation = 0;
        
        InitialiseChildOffsets();
    }
    
    void UpdateAI(const uint32 uiDiff)
    {
        // Only do this once after reset.
        if (m_uiChildCatchingTimer)
        {
            if (m_uiChildCatchingTimer <= uiDiff)
            {
                GetChildren();
                m_creature->SetSplineFlags(SPLINEFLAG_WALKMODE);
                // Position the children around this creature.
                float origo[2];
                origo[0] = m_creature->GetPositionX();
                origo[1] = m_creature->GetPositionY();
                PositionChildren(origo);

                StartPatrol(0, true);
                
                m_uiChildCatchingTimer = 0;
            }
            else
                m_uiChildCatchingTimer -= uiDiff;
        }
        
        m_ChildRotation += m_ChildRotation > 2 * PI ? -m_ChildRotation : PI * (uiDiff / 1000.f);
        InitialiseChildOffsets(m_ChildRotation);
    }
    
    void MovementInform(uint32 movementType, uint32 pointId)
    {        
        npc_patrolAI::MovementInform(movementType, pointId);

        //if(pointId == 15)
        //    InitialiseChildOffsets(-0.5 * PI);
        
        float origo[2] = { GetTargetWaypoint().fX, GetTargetWaypoint().fY };
        
        PositionChildren(origo);
    }
    
    void PositionChildren(float origo[2])
    {
        if (m_ChildList.size() < 5)
            return;

        for (short i = 0; i < 5; ++i)
        {
            float pos[2];
            
            CalculateChildPosFromOffset(origo, m_ChildOffsets[i], pos);
            
            uint32 traveltime = m_creature->GetMotionMaster()->GetTotalTravelTime();
             
            m_ChildList[i]->MonsterMove(pos[0], pos[1], GetTargetWaypoint().fZ, traveltime);
        }
    }
    
    void GetChildren()
    {
        for (short i = 0; i < 5; ++i)
        {
            Unit* child = GetClosestCreatureWithEntry(m_creature, m_ChildEntries[i], 10.f);
            if (child)
            {
                m_ChildList.push_back(child);
                ((Creature*)child)->RemoveSplineFlag(SPLINEFLAG_WALKMODE);
            }
        }
    }
    
    // Give the children offsets that match a pentagram.
    void InitialiseChildOffsets(float rotation = 0.f)
    {
        for (short i = 0; i < 5; ++i)
        {
            if (i != 4)
                CalculateNewChildOffset(i < 2 ? 3.5f : 2.f, i * (PI / 2.f) + PI / 4.f + rotation, m_ChildOffsets[i]);
            else
                CalculateNewChildOffset(4.f, PI / 2 + rotation, m_ChildOffsets[i]);
        }
    }
    
    // Takes an offset and a center point and returns the childs position in the same
    // coordinates as the center was given as.
    inline void CalculateChildPosFromOffset(float origo[2], float offset[2], float* result)
    {
        result[0] = origo[0] + offset[0];
        result[1] = origo[1] + offset[1];
    }
    
    // Calculates a child's offset from origo for the given radius and angle.
    inline void CalculateNewChildOffset(float radius, float angle, float* result)
    {
        result[0] = radius * (float) cos(angle);
        result[1] = radius * (float) sin(angle);
    }
};

CreatureAI* GetAI_npc_creepy_child(Creature* pCreature)
{
    return new npc_creepy_child(pCreature);
}

/*####
# npc_maybell_maclure
####*/

enum
{
	MAYBELL_SAY_1					 = -1720048,

	SPELL_DRINK_POTION				 = 9956,

    QUEST_ID_THE_ESCAPE				 = 114,
};

struct MANGOS_DLL_DECL npc_maybell_maclureAI : public npc_escortAI
{
    npc_maybell_maclureAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }
	
	uint8 m_uiSpeechStep;
	uint32 m_uiSpeechTimer;
	bool m_bOutro;

    void Reset()
	{
		m_bOutro = false;
		m_uiSpeechStep = 1;
		m_uiSpeechTimer = 0;
	}

	void WaypointReached(uint32 uiPointId)
    {
	}

	void JustStartedEscort()
    {
    }
	
	void StartOutro()
	{
		m_bOutro = true; 
		m_uiSpeechTimer = 3000;
		m_uiSpeechStep = 1;
		m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP + UNIT_NPC_FLAG_QUESTGIVER);
	}

	void UpdateEscortAI(const uint32 uiDiff)
    {
		if (m_uiSpeechTimer && m_bOutro)							// handle RP at quest end
		{
			if (!m_uiSpeechStep)
				return;
		
			if (m_uiSpeechTimer <= uiDiff)
            {
                switch(m_uiSpeechStep)
                {
                    case 1:
                        DoScriptText(MAYBELL_SAY_1, m_creature);						
						m_uiSpeechTimer = 1000;
                        break;
					case 2:				// drink the potion
						m_creature->CastSpell(m_creature, SPELL_DRINK_POTION, false);
                        m_uiSpeechTimer = 6000;
                        break;
					case 3:
						m_creature->SetVisibility(VISIBILITY_OFF);
						m_uiSpeechTimer = 3000;
						break;
					case 4:
						m_uiSpeechTimer = 7000;
						break;
					case 5:
						m_creature->SetVisibility(VISIBILITY_ON);
						m_uiSpeechTimer = 3000;
						break;
					case 6:
						m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP + UNIT_NPC_FLAG_QUESTGIVER);
						m_bOutro = false;
						break;
                    /*default:
                        m_uiSpeechStep = 0;
                        return;*/
                }
                ++m_uiSpeechStep;
            }
            else
                m_uiSpeechTimer -= uiDiff;
		}
	}
};

CreatureAI* GetAI_npc_maybell_maclure(Creature* pCreature)
{
    return new npc_maybell_maclureAI(pCreature);
}

bool OnQuestRewarded_npc_maybell_maclure(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
	if (pQuest->GetQuestId() == QUEST_ID_THE_ESCAPE)
    {
		if (npc_maybell_maclureAI* pEscortAI = dynamic_cast<npc_maybell_maclureAI*>(pCreature->AI()))
			pEscortAI->StartOutro();
	}
	return true;
}

void AddSC_elwynn_forest()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "npc_henze_faulk";
    pNewscript->GetAI = &GetAI_npc_henze_faulk;
    pNewscript->RegisterSelf();
    
    pNewscript = new Script;
    pNewscript->Name = "npc_creepy_child";
    pNewscript->GetAI = &GetAI_npc_creepy_child;
    pNewscript->RegisterSelf();

	pNewscript = new Script;
    pNewscript->Name = "npc_maybell_maclure";
    pNewscript->GetAI = &GetAI_npc_maybell_maclure;
    pNewscript->pQuestRewardedNPC = &OnQuestRewarded_npc_maybell_maclure;
    pNewscript->RegisterSelf();
}
