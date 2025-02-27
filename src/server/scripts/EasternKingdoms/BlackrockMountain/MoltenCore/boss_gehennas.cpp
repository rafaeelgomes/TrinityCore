/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
SDName: Boss_Gehennas
SD%Complete: 90
SDComment: Adds MC NYI
SDCategory: Molten Core
EndScriptData */

#include "ScriptMgr.h"
#include "molten_core.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_GEHENNAS_CURSE    = 19716,
    SPELL_RAIN_OF_FIRE      = 19717,
    SPELL_SHADOW_BOLT       = 19728,
};

enum Events
{
    EVENT_GEHENNAS_CURSE    = 1,
    EVENT_RAIN_OF_FIRE      = 2,
    EVENT_SHADOW_BOLT       = 3,
};

class boss_gehennas : public CreatureScript
{
    public:
        boss_gehennas() : CreatureScript("boss_gehennas") { }

        struct boss_gehennasAI : public BossAI
        {
            boss_gehennasAI(Creature* creature) : BossAI(creature, BOSS_GEHENNAS)
            {
            }

            void JustEngagedWith(Unit* victim) override
            {
                BossAI::JustEngagedWith(victim);
                events.ScheduleEvent(EVENT_GEHENNAS_CURSE, 12s);
                events.ScheduleEvent(EVENT_RAIN_OF_FIRE, 10s);
                events.ScheduleEvent(EVENT_SHADOW_BOLT, 6s);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GEHENNAS_CURSE:
                            DoCastVictim(SPELL_GEHENNAS_CURSE);
                            events.ScheduleEvent(EVENT_GEHENNAS_CURSE, 22s, 30s);
                            break;
                        case EVENT_RAIN_OF_FIRE:
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                                DoCast(target, SPELL_RAIN_OF_FIRE);
                            events.ScheduleEvent(EVENT_RAIN_OF_FIRE, 4s, 12s);
                            break;
                        case EVENT_SHADOW_BOLT:
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1))
                                DoCast(target, SPELL_SHADOW_BOLT);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT, 7s);
                            break;
                        default:
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetMoltenCoreAI<boss_gehennasAI>(creature);
        }
};

void AddSC_boss_gehennas()
{
    new boss_gehennas();
}
