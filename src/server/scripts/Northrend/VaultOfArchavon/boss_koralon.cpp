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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "vault_of_archavon.h"

enum Events
{
    // Koralon
    EVENT_BURNING_BREATH    = 1,
    EVENT_BURNING_FURY      = 2,
    EVENT_FLAME_CINDER      = 3,
    EVENT_METEOR_FISTS      = 4
};

enum Spells
{
    // Spells Koralon
    SPELL_BURNING_BREATH                        = 66665,
    SPELL_BURNING_FURY                          = 66721,
    SPELL_FLAME_CINDER_A                        = 66684,
    SPELL_FLAME_CINDER_B                        = 66681, // don't know the real relation to SPELL_FLAME_CINDER_A atm.
    SPELL_METEOR_FISTS                          = 66725,
    SPELL_METEOR_FISTS_DAMAGE                   = 66765,

    // Spells Flame Warder
    SPELL_FW_METEOR_FISTS_DAMAGE                = 66809
};

class boss_koralon : public CreatureScript
{
    public:
        boss_koralon() : CreatureScript("boss_koralon") { }

        struct boss_koralonAI : public BossAI
        {
            boss_koralonAI(Creature* creature) : BossAI(creature, DATA_KORALON)
            {
            }

            void JustEngagedWith(Unit* who) override
            {
                DoCast(me, SPELL_BURNING_FURY);

                events.ScheduleEvent(EVENT_BURNING_FURY, 20s);    /// @todo check timer
                events.ScheduleEvent(EVENT_BURNING_BREATH, 15s);  // 1st after 15sec, then every 45sec
                events.ScheduleEvent(EVENT_METEOR_FISTS, 75s);    // 1st after 75sec, then every 45sec
                events.ScheduleEvent(EVENT_FLAME_CINDER, 30s);    /// @todo check timer

                BossAI::JustEngagedWith(who);
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
                        case EVENT_BURNING_FURY:
                            DoCast(me, SPELL_BURNING_FURY);
                            events.ScheduleEvent(EVENT_BURNING_FURY, 20s);
                            break;
                        case EVENT_BURNING_BREATH:
                            DoCast(me, SPELL_BURNING_BREATH);
                            events.ScheduleEvent(EVENT_BURNING_BREATH, 45s);
                            break;
                        case EVENT_METEOR_FISTS:
                            DoCast(me, SPELL_METEOR_FISTS);
                            events.ScheduleEvent(EVENT_METEOR_FISTS, 45s);
                            break;
                        case EVENT_FLAME_CINDER:
                            DoCast(me, SPELL_FLAME_CINDER_A);
                            events.ScheduleEvent(EVENT_FLAME_CINDER, 30s);
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
            return GetVaultOfArchavonAI<boss_koralonAI>(creature);
        }
};

class spell_koralon_meteor_fists : public SpellScriptLoader
{
    public:
        spell_koralon_meteor_fists() : SpellScriptLoader("spell_koralon_meteor_fists") { }

        class spell_koralon_meteor_fists_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_koralon_meteor_fists_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_METEOR_FISTS_DAMAGE });
            }

            void TriggerFists(AuraEffect* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_METEOR_FISTS_DAMAGE, aurEff);
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_koralon_meteor_fists_AuraScript::TriggerFists, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_koralon_meteor_fists_AuraScript();
        }
};

class spell_koralon_meteor_fists_damage : public SpellScriptLoader
{
    public:
        spell_koralon_meteor_fists_damage() : SpellScriptLoader("spell_koralon_meteor_fists_damage") { }

        class spell_koralon_meteor_fists_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_koralon_meteor_fists_damage_SpellScript);

        public:
            spell_koralon_meteor_fists_damage_SpellScript()
            {
                _chainTargets = 0;
            }

        private:
            void FilterTargets(std::list<WorldObject*>& targets)
            {
                _chainTargets = uint8(targets.size());
            }

            void CalculateSplitDamage()
            {
                if (_chainTargets)
                    SetHitDamage(GetHitDamage() / (_chainTargets + 1));
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_koralon_meteor_fists_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
                OnHit += SpellHitFn(spell_koralon_meteor_fists_damage_SpellScript::CalculateSplitDamage);
            }

        private:
            uint8 _chainTargets;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_koralon_meteor_fists_damage_SpellScript();
        }
};

class spell_flame_warder_meteor_fists : public SpellScriptLoader
{
    public:
        spell_flame_warder_meteor_fists() : SpellScriptLoader("spell_flame_warder_meteor_fists") { }

        class spell_flame_warder_meteor_fists_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_flame_warder_meteor_fists_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_FW_METEOR_FISTS_DAMAGE });
            }

            void TriggerFists(AuraEffect* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_FW_METEOR_FISTS_DAMAGE, aurEff);
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_flame_warder_meteor_fists_AuraScript::TriggerFists, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_flame_warder_meteor_fists_AuraScript();
        }
};

void AddSC_boss_koralon()
{
    new boss_koralon();
    new spell_koralon_meteor_fists();
    new spell_koralon_meteor_fists_damage();
    new spell_flame_warder_meteor_fists();
}
