/*
 * Copyright (C) 2020+     Project "Sol" <https://gitlab.com/opfesoft/sol>, released under the GNU AGPLv3 license: https://gitlab.com/opfesoft/mod-npc-beastmaster/-/blob/master/LICENSE.md
 * Copyright (C) 2018-2020 AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/mod-npc-beastmaster/blob/master/LICENSE.md
 * Copyright (C) 2017      StygianTheBest <https://stygianthebest.github.io>, released under the GNU AGPLv3 license: https://github.com/StygianTheBest/AzerothCore-Content/blob/master/Modules/mod-npcbeastmaster/LICENSE.md
*/

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "Config.h"
#include "Pet.h"
#include "ScriptedGossip.h"
#include "ScriptedCreature.h"

std::vector<uint32> HunterSpells = { 883, 982, 2641, 6991, 48990, 1002, 1462, 6197 };
std::map<std::string, uint32> pets;
std::map<std::string, uint32> exoticPets;
std::map<std::string, uint32> rarePets;
std::map<std::string, uint32> rareExoticPets;
std::map<std::string, uint32> customAuras;
bool BeastMasterAnnounceToPlayer;
bool BeastMasterHunterOnly;
bool BeastMasterAllowExotic;
bool BeastMasterKeepPetHappy;
uint32 BeastMasterMinLevel;
bool BeastMasterHunterBeastMasteryRequired;

enum PetGossip
{
    PET_BEASTMASTER_HOWL            =     9036,
    PET_PAGE_SIZE                   =       13,
    PET_PAGE_START_PETS             =      501,
    PET_PAGE_START_EXOTIC_PETS      =      601,
    PET_PAGE_START_RARE_PETS        =      701,
    PET_PAGE_START_RARE_EXOTIC_PETS =      801,
    PET_PAGE_MAX                    =      901,
    PET_MAIN_MENU                   =       50,
    PET_REMOVE_SKILLS               =       80,
    PET_GOSSIP_HELLO                =   601026,
    PET_PAGE_START_CUSTOM_AURAS     = 10000101,
    PET_PAGE_MAX_CUSTOM_AURAS       = 10000201,
    PET_REMOVE_CUSTOM_AURAS         = 10000010,
};

enum PetSpells
{
    PET_SPELL_CALL_PET      =     883,
    PET_SPELL_TAME_BEAST    =   13481,
    PET_SPELL_BEAST_MASTERY =   53270,
    PET_MAX_HAPPINESS       = 1048000,
};

enum BeastMasterEvents
{
    BEASTMASTER_EVENT_EAT = 1,
};

class BeastMaster_CreatureScript : public CreatureScript
{

public:

    BeastMaster_CreatureScript() : CreatureScript("BeastMaster") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        // If enabled for Hunters only..
        if (BeastMasterHunterOnly)
        {
            if (player->getClass() != CLASS_HUNTER)
            {
                creature->MonsterWhisper("I am sorry, but pets are for hunters only.", player, false);
                return true;
            }
        }

        // Check level requirement
        if (player->getLevel() < BeastMasterMinLevel && BeastMasterMinLevel != 0)
        {
            std::ostringstream messageExperience;
            messageExperience << "Sorry " << player->GetName() << ", but you must reach level " << BeastMasterMinLevel << " before adopting a pet.";
            creature->MonsterWhisper(messageExperience.str().c_str(), player);
            return true;
        }

        // MAIN MENU
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_PETS);
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_RARE_PETS);

        if (BeastMasterAllowExotic || player->HasSpell(PET_SPELL_BEAST_MASTERY) || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec()))
        {
            if (player->getClass() != CLASS_HUNTER)
            {
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Exotic Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_EXOTIC_PETS);
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Exotic Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_RARE_EXOTIC_PETS);
            }
            else if (!BeastMasterHunterBeastMasteryRequired || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec()))
            {
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Exotic Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_EXOTIC_PETS);
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Exotic Pets", GOSSIP_SENDER_MAIN, PET_PAGE_START_RARE_EXOTIC_PETS);
            }
        }

        if (player->GetPet() && player->GetPet()->getPetType() == HUNTER_PET)
        {
            // Custom auras
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Toggle custom auras", GOSSIP_SENDER_MAIN, PET_PAGE_START_CUSTOM_AURAS);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Remove all custom auras", GOSSIP_SENDER_MAIN, PET_REMOVE_CUSTOM_AURAS);
        }

        // remove pet skills (not for hunters)
        if (player->getClass() != CLASS_HUNTER && player->HasSpell(PET_SPELL_CALL_PET))
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Unlearn Hunter Abilities", GOSSIP_SENDER_MAIN, PET_REMOVE_SKILLS);

        // Stables for hunters only - Doesn't seem to work for other classes
        if (player->getClass() == CLASS_HUNTER)
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Visit Stable", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_STABLEPET);

        // Pet Food Vendor
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy Pet Food", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_VENDOR);

        player->PlayerTalkClass->SendGossipMenu(PET_GOSSIP_HELLO, creature->GetGUID());

        // Howl
        player->PlayDirectSound(PET_BEASTMASTER_HOWL);

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (action == PET_MAIN_MENU)
        {
            // MAIN MENU
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Pets", action, PET_PAGE_START_PETS);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Pets", action, PET_PAGE_START_RARE_PETS);

            if (BeastMasterAllowExotic || player->HasSpell(PET_SPELL_BEAST_MASTERY) || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec()))
            {
                if (player->getClass() != CLASS_HUNTER)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Exotic Pets", action, PET_PAGE_START_EXOTIC_PETS);
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Exotic Pets", action, PET_PAGE_START_RARE_EXOTIC_PETS);
                }
                else if (!BeastMasterHunterBeastMasteryRequired || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec()))
                {
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Exotic Pets", action, PET_PAGE_START_EXOTIC_PETS);
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Browse Rare Exotic Pets", action, PET_PAGE_START_RARE_EXOTIC_PETS);
                }
            }

            // Custom auras
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Toggle custom auras", action, PET_PAGE_START_CUSTOM_AURAS);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Remove all custom auras", action, PET_REMOVE_CUSTOM_AURAS);

            // remove pet skills (not for hunters)
            if (player->getClass() != CLASS_HUNTER && player->HasSpell(PET_SPELL_CALL_PET))
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Unlearn Hunter Abilities", action, PET_REMOVE_SKILLS);

            // Stables for hunters only - Doesn't seem to work for other classes
            if (player->getClass() == CLASS_HUNTER)
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Visit Stable", action, GOSSIP_OPTION_STABLEPET);

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Buy Pet Food", action, GOSSIP_OPTION_VENDOR);
            player->PlayerTalkClass->SendGossipMenu(PET_GOSSIP_HELLO, creature->GetGUID());
        }
        else if (action >= PET_PAGE_START_PETS && action < PET_PAGE_START_EXOTIC_PETS)
        {
            // PETS
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back..", action, PET_MAIN_MENU);
            int page = action - PET_PAGE_START_PETS + 1;
            int maxPage = pets.size() / PET_PAGE_SIZE + (pets.size() % PET_PAGE_SIZE != 0);

            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", action, PET_PAGE_START_PETS + page - 2);

            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", action, PET_PAGE_START_PETS + page);

            AddGossip(player, action, pets, page, PET_PAGE_MAX);
            player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        }
        else if (action >= PET_PAGE_START_EXOTIC_PETS && action < PET_PAGE_START_RARE_PETS)
        {
            // EXOTIC BEASTS
            // Teach Beast Mastery or Spirit Beasts won't work properly
            if (! (player->HasSpell(PET_SPELL_BEAST_MASTERY) || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec())))
            {
                player->addSpell(PET_SPELL_BEAST_MASTERY, SPEC_MASK_ALL, false);
                std::ostringstream messageLearn;
                messageLearn << "I have taught you the art of Beast Mastery, " << player->GetName() << ".";
                creature->MonsterWhisper(messageLearn.str().c_str(), player);
            }

            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back..", action, PET_MAIN_MENU);
            int page = action - PET_PAGE_START_EXOTIC_PETS + 1;
            int maxPage = exoticPets.size() / PET_PAGE_SIZE + (exoticPets.size() % PET_PAGE_SIZE != 0);

            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", action, PET_PAGE_START_EXOTIC_PETS + page - 2);

            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", action, PET_PAGE_START_EXOTIC_PETS + page);

            AddGossip(player, action, exoticPets, page, PET_PAGE_MAX);
            player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        }
        else if (action >= PET_PAGE_START_RARE_PETS && action < PET_PAGE_START_RARE_EXOTIC_PETS)
        {
            // RARE PETS
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back..", action, PET_MAIN_MENU);
            int page = action - PET_PAGE_START_RARE_PETS + 1;
            int maxPage = rarePets.size() / PET_PAGE_SIZE + (rarePets.size() % PET_PAGE_SIZE != 0);

            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", action, PET_PAGE_START_RARE_PETS + page - 2);

            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", action, PET_PAGE_START_RARE_PETS + page);

            AddGossip(player, action, rarePets, page, PET_PAGE_MAX);
            player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        }
        else if (action >= PET_PAGE_START_RARE_EXOTIC_PETS && action < PET_PAGE_MAX)
        {
            // RARE EXOTIC BEASTS
            // Teach Beast Mastery or Spirit Beasts won't work properly
            if (! (player->HasSpell(PET_SPELL_BEAST_MASTERY) || player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec())))
            {
                player->addSpell(PET_SPELL_BEAST_MASTERY, SPEC_MASK_ALL, false);
                std::ostringstream messageLearn;
                messageLearn << "I have taught you the art of Beast Mastery, " << player->GetName() << ".";
                creature->MonsterWhisper(messageLearn.str().c_str(), player);
            }

            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back..", action, PET_MAIN_MENU);
            int page = action - PET_PAGE_START_RARE_EXOTIC_PETS + 1;
            int maxPage = rareExoticPets.size() / PET_PAGE_SIZE + (rareExoticPets.size() % PET_PAGE_SIZE != 0);

            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", action, PET_PAGE_START_RARE_EXOTIC_PETS + page - 2);

            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", action, PET_PAGE_START_RARE_EXOTIC_PETS + page);

            AddGossip(player, action, rareExoticPets, page, PET_PAGE_MAX);
            player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        }
        else if (action >= PET_PAGE_START_CUSTOM_AURAS)
        {
            // CUSTOM AURAS
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Back..", action, PET_MAIN_MENU);
            uint32 menuAction = action >= PET_PAGE_MAX_CUSTOM_AURAS ? sender : action;
            uint32 page = menuAction - PET_PAGE_START_CUSTOM_AURAS + 1;
            uint32 maxPage = customAuras.size() / PET_PAGE_SIZE + (customAuras.size() % PET_PAGE_SIZE != 0);

            if (page > 1)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Previous..", menuAction, PET_PAGE_START_CUSTOM_AURAS + page - 2);

            if (page < maxPage)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Next..", menuAction, PET_PAGE_START_CUSTOM_AURAS + page);

            AddGossip(player, menuAction, customAuras, page, PET_PAGE_MAX_CUSTOM_AURAS);
            player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        }
        else if (action == PET_REMOVE_CUSTOM_AURAS)
        {
            if (Pet* pet = player->GetPet())
            {
                if (QueryResult result = CharacterDatabase.PQuery("SELECT `CustomAura` FROM `mod_npc_beastmaster_custom_auras` WHERE `PlayerGUIDLow` = %u AND `PetID` = %u", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber()))
                    do
                    {
                        Field* fields = result->Fetch();
                        pet->RemoveAurasDueToSpell(fields[0].GetUInt32());
                    } while (result->NextRow());

                CharacterDatabase.PExecute("DELETE FROM `mod_npc_beastmaster_custom_auras` WHERE `PlayerGUIDLow` = %u AND `PetID` = %u", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber());
            }

            CloseGossipMenuFor(player);
        }
        else if (action == PET_REMOVE_SKILLS)
        {
            // remove pet and granted skills
            for (std::size_t i = 0; i < HunterSpells.size(); ++i)
                player->removeSpell(HunterSpells[i], SPEC_MASK_ALL, false);

            player->removeSpell(PET_SPELL_BEAST_MASTERY, SPEC_MASK_ALL, false);
            CloseGossipMenuFor(player);
        }
        else if (action == GOSSIP_OPTION_STABLEPET)
        {
            // STABLE
            player->GetSession()->SendStablePet(creature->GetGUID());
        }
        else if (action == GOSSIP_OPTION_VENDOR)
        {
            // VENDOR
            player->GetSession()->SendListInventory(creature->GetGUID());
        }

        if (action >= PET_PAGE_MAX_CUSTOM_AURAS) // CUSTOM AURAS
            ToggleCustomAura(player, creature, action);
        else if (action >= PET_PAGE_MAX && action < PET_REMOVE_CUSTOM_AURAS) // BEASTS
            CreatePet(player, creature, action);
        return true;
    }

    struct beastmasterAI : public ScriptedAI
    {
        beastmasterAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;

        void Reset() override
        {
            events.ScheduleEvent(BEASTMASTER_EVENT_EAT, urand(30000, 90000));
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case BEASTMASTER_EVENT_EAT:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_EAT_NO_SHEATHE);
                    events.ScheduleEvent(BEASTMASTER_EVENT_EAT, urand(30000, 90000));
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new beastmasterAI(creature);
    }

private:
    static void CreatePet(Player* player, Creature* creature, uint32 entry)
    {
        // Check if player already has a pet
        if (player->GetPet())
        {
            creature->MonsterWhisper("First you must abandon or stable your current pet!", player, false);
            CloseGossipMenuFor(player);
            return;
        }

        // Create Tamed Creature
        Pet* pet = player->CreateTamedPetFrom(entry - PET_PAGE_MAX, player->getClass() == CLASS_HUNTER ? PET_SPELL_TAME_BEAST : PET_SPELL_CALL_PET);
        if (!pet) { return; }

        // Set Pet Happiness
        pet->SetPower(POWER_HAPPINESS, PET_MAX_HAPPINESS);

        // Initialize Pet
        pet->AddUInt64Value(UNIT_FIELD_CREATEDBY, player->GetGUID());
        pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, player->getFaction());
        pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel());

        // Prepare Level-Up Visual
        pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel() - 1);
        pet->GetMap()->AddToMap(pet->ToCreature());

        // Visual Effect for Level-Up
        pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel());

        // Initialize Pet Stats
        pet->InitTalentForLevel();
        if (!pet->InitStatsForLevel(player->getLevel()))
        {
            pet->UpdateAllStats();
        }

        // Caster Pets?
        player->SetMinion(pet, true);

        // Save Pet
        pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
        player->PetSpellInitialize();
        pet->InitLevelupSpellsForLevel();
        pet->SavePetToDB(PET_SAVE_AS_CURRENT, 0);

        // Learn Hunter Abilities (only for non-hunters)
        if (player->getClass() != CLASS_HUNTER)
        {
            // Assume player has already learned the spells if they have Call Pet
            if (!player->HasSpell(PET_SPELL_CALL_PET))
            {
                for (std::size_t i = 0; i < HunterSpells.size(); ++i)
                    player->learnSpell(HunterSpells[i]);
            }
        }

        // Farewell
        std::ostringstream messageAdopt;
        messageAdopt << "A fine choice " << player->GetName() << "! Take good care of your " << pet->GetName() << " and you will never face your enemies alone.";
        creature->MonsterWhisper(messageAdopt.str().c_str(), player);
        CloseGossipMenuFor(player);
    }

    static void ToggleCustomAura(Player* player, Creature* creature, uint32 entry)
    {
        Pet* pet = player->GetPet();
        if (!pet)
        {
            creature->MonsterWhisper("You currently don't have a pet!", player, false);
            CloseGossipMenuFor(player);
            return;
        }

        uint32 spellId = entry - PET_PAGE_MAX_CUSTOM_AURAS;
        bool customAuraFound = false;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT `CustomAura` FROM `mod_npc_beastmaster_custom_auras` WHERE `PlayerGUIDLow` = %u AND `PetID` = %u", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber()))
            do
            {
                Field* fields = result->Fetch();
                if (fields[0].GetUInt32() == spellId)
                    customAuraFound = true;
            } while (result->NextRow());

        if (customAuraFound)
        {
            pet->RemoveAurasDueToSpell(spellId);
            CharacterDatabase.PExecute("DELETE FROM `mod_npc_beastmaster_custom_auras` WHERE `PlayerGUIDLow` = %u AND `PetID` = %u AND `CustomAura` = %u", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber(), spellId);
            creature->MonsterWhisper("Custom aura removed", player, false);
        }
        else
        {
            if (!pet->HasAura(spellId))
                pet->AddAura(spellId, pet);
            CharacterDatabase.PExecute("INSERT INTO `mod_npc_beastmaster_custom_auras` (`PlayerGUIDLow`, `PetID`, `CustomAura`) VALUES (%u, %u, %u)", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber(), spellId);
            creature->MonsterWhisper("Custom aura added", player, false);
        }
    }

    static void AddGossip(Player *player, uint32 action, std::map<std::string, uint32> &map, int page, int pageMax)
    {
        std::map<std::string, uint32>::iterator it;
        int count = 1;

        for (it = map.begin(); it != map.end(); it++)
        {
            if (count > (page - 1) * PET_PAGE_SIZE && count <= page * PET_PAGE_SIZE)
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, it->first, action, it->second + pageMax);

            count++;
        }
    }
};

class BeastMaster_WorldScript : public WorldScript
{
public:
    BeastMaster_WorldScript() : WorldScript("BeastMaster_WorldScript") { }

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        BeastMasterAnnounceToPlayer = sConfigMgr->GetBoolDefault("BeastMaster.Announce", true);
        BeastMasterHunterOnly = sConfigMgr->GetBoolDefault("BeastMaster.HunterOnly", true);
        BeastMasterAllowExotic = sConfigMgr->GetBoolDefault("BeastMaster.AllowExotic", false);
        BeastMasterKeepPetHappy = sConfigMgr->GetBoolDefault("BeastMaster.KeepPetHappy", false);
        BeastMasterMinLevel = sConfigMgr->GetIntDefault("BeastMaster.MinLevel", 10);
        BeastMasterHunterBeastMasteryRequired = sConfigMgr->GetIntDefault("BeastMaster.HunterBeastMasteryRequired", true);

        if (BeastMasterMinLevel > 80)
        {
            BeastMasterMinLevel = 10;
        }

        pets.clear();
        exoticPets.clear();
        rarePets.clear();
        rareExoticPets.clear();
        customAuras.clear();

        LoadMap(sConfigMgr->GetStringDefault("BeastMaster.Pets", ""), pets);
        LoadMap(sConfigMgr->GetStringDefault("BeastMaster.ExoticPets", ""), exoticPets);
        LoadMap(sConfigMgr->GetStringDefault("BeastMaster.RarePets", ""), rarePets);
        LoadMap(sConfigMgr->GetStringDefault("BeastMaster.RareExoticPets", ""), rareExoticPets);
        LoadMap(sConfigMgr->GetStringDefault("BeastMaster.CustomAuras", ""), customAuras);
    }

private:
    static void LoadMap(std::string param, std::map<std::string, uint32> &map)
    {
        std::string delimitedValue;
        std::stringstream stringStream;
        std::string name;
        int count = 0;

        stringStream.str(param);

        while (std::getline(stringStream, delimitedValue, ','))
        {
            if (count % 2 == 0)
            {
                name = delimitedValue;
            }
            else
            {
                uint32 id = atoi(delimitedValue.c_str());
                map[name] = id;
            }

            count++;
        }
    }
};

class BeastMaster_PlayerScript : public PlayerScript
{
    public:
    BeastMaster_PlayerScript() : PlayerScript("BeastMaster_PlayerScript") { }

    void OnLogin(Player* player) override
    {
        // Announce Module
        if (BeastMasterAnnounceToPlayer)
        {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00BeastMasterNPC |rmodule.");
        }
    }

    void OnBeforeUpdate(Player* player, uint32 /*p_time*/) override
    {
        if (BeastMasterKeepPetHappy && player->GetPet())
        {
            Pet* pet = player->GetPet();

            if (pet->getPetType() == HUNTER_PET)
            {
                pet->SetPower(POWER_HAPPINESS, PET_MAX_HAPPINESS);
            }
        }
    }

    void OnBeforeLoadPetFromDB(Player* /*player*/, uint32& /*petentry*/, uint32& /*petnumber*/, bool& /*current*/, bool& forceLoadFromDB) override
    {
        forceLoadFromDB = true;
    }

    void OnBeforeGuardianInitStatsForLevel(Player* /*player*/, Guardian* /*guardian*/, CreatureTemplate const* cinfo, PetType& petType) override
    {
        if (cinfo->IsTameable(true))
        {
            petType = HUNTER_PET;
        }
    }

    void OnAfterActivateSpec(Player* player, uint8 /*spec*/) override
    {
        if (player->getClass() == CLASS_HUNTER && !BeastMasterHunterBeastMasteryRequired && !player->HasTalent(PET_SPELL_BEAST_MASTERY, player->GetActiveSpec()))
        {
            player->addSpell(PET_SPELL_BEAST_MASTERY, SPEC_MASK_ALL, false);
            player->AddAura(PET_SPELL_BEAST_MASTERY, player);
        }
    }

    void OnAfterCastPetAuras(Player* player, Pet* pet) override
    {
        if (!player || !pet)
            return;

        if (QueryResult result = CharacterDatabase.PQuery("SELECT `CustomAura` FROM `mod_npc_beastmaster_custom_auras` WHERE `PlayerGUIDLow` = %u AND `PetID` = %u", player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber()))
            do
            {
                Field* fields = result->Fetch();
                uint32 spellId = fields[0].GetUInt32();
                if (!pet->HasAura(spellId))
                    pet->AddAura(spellId, pet);
            } while (result->NextRow());
    }
};

void AddBeastMasterScripts()
{
    new BeastMaster_WorldScript();
    new BeastMaster_CreatureScript();
    new BeastMaster_PlayerScript();
}
