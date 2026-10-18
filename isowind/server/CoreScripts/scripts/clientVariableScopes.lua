-- Place clientside variables in different categories to decide how they are synchronized,
-- saved and loaded
--
-- Note: Currently, only global variables are handled, not local or member variables
--
-- Descriptions:
-- * "ignored" is where you place variables that clients should not send packets about,
--   either because they are already handled in other packets or because they would cause
--   unnecessary packet spam
-- * "personal" is where you place variables that are always exclusive to specific players
--   and that should not be shared regardless of other server options
-- * "quest" is where you place variables that should be synchronized and shared across
--   players based on the value of config.shareJournal
-- * "kills" is where you place variables that should be handled the same as kill counts
--   and should be cleared whenever the regular kill counts are
-- * "factionRanks" is where you place variables that should be synchronized and shared across
--   players based on the value of config.shareFactionRanks
-- * "factionExpulsion" is where you place variables that should be synchronized and shared across
--   players based on the value of config.shareFactionExpulsion
-- * "worldwide" is where you place variables that are always shared across all players
--   because they affect the physical world in a way that should be visible to everyone,
--   i.e. they affect structures, mechanism states, water levels, and so on
local clientVariableScopes = {
    globals = {}
}

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Morrowind.esm") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- game state
                "random100",
                -- game settings
                "npcvoicedistance",
                -- time
                "gamehour", "timescale", "month", "day", "year",
                -- player character details
                "pcrace",
                -- player state
                "chargenstate", "pchascrimegold", "crimegolddiscount", "crimegoldturnin", "pchasgolddiscount",
                "pchasturnin",
                -- player equipment
                "wearinglegionuni", "wearingordinatoruni", "wearinghelmhhda", "wraithguardequipped", "tgglove",
                -- quest variables that are already set correctly without being synced
                "fargothwalk",
                -- not actually used at all
                "abelmawiacounter"
            },
            personal = {
                -- player state
                "pcvampire",
                -- tavern rents
                "rent_pelagiad_halfway", "rent_smora_gateway", "rent_smora_faras", "rent_ebon_six", "rent_balmora_south",
                "rent_balmora_council", "rent_balmora_lucky", "rent_aldruhn_skar", "rent_telaruhn_plot", "rent_telbran_sethan",
                "rent_telmora_covenant", "rent_vos_varo", "rent_balmora_eight", "rent_caldera_shenk", "rent_maargan_andus",
                "rent_vivec_lizard", "rent_vivec_flower", "rent_vivec_black", "rent_ghost_dusk",
                -- miscellaneous variables related to player-specific actions
                "chargenbreadstate"
            },
            quest = {
                -- main quest
                "hortatorvotes", "heartdestroyed", "destroyblight",
                -- reading of confidential documents
                "unsealededryno1", "unsealedodral1",
                -- assassination quests
                -- Note: These are related to kill counts, but they make more sense being shared across
                --       players who share quests than being shared across players who share kills
                "mt_legitkills", "mt_newcrimelevel", "mt_writdiscount",
                -- side quests for rescues
                "freedslavescounter", "madurarescued",
                -- other side quests
                "threadswebspinner", "monopolyvotes", "bone", "ownershiphhcs"
            },
            kills = {
                -- main quest
                "redoranmurdered", "telvannidead",
                -- side quests
                "ratskilled", "vampkills"
            },
            factionRanks = {
                -- membership in mini-factions
                "vampclan"
            },
            factionExpulsion = {
                -- faction expulsion forgiveness and timers
                "expredoran", "expmagesguild", "expfightersguild", "exptemple", "expmoragtong", "expimperialcult",
                "expimperiallegion", "expthievesguild"
            },
            worldwide = {
                -- mechanisms
                "gg_gate1_state", "gg_gate2_state",
                -- building construction
                "stronghold",
                -- whether duel arenas are occupied
                "duelactive"
            },
            unknown = {
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Tribunal.esm") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- player state
                "pcgold",
                -- quest variables that are already set correctly without being synced
                "mercenarynear"
            },
            personal = {
                -- tavern rent
                "rent_mh_guar",
                -- mercenary contracts
                "contractcalvusday", "contractcalvusmonth", "contract_calvus_days_left",
                -- fights started
                "kinghit"
            },
            quest = {
                -- main quest
                "dbattack", "mournholdattack", "fabattack", "shrinecleanse", "bladefix", "hasblade", "kgaveblade",
                "duelmiss", "karrodbribe", "karrodbeaten", "karrodfightstart", "karrodcheapshot",
                -- side quests
                "plaguerock", "plagueactivate", "plaguestage", "droth_var", "museumdonations", "matchmakeswitch",
                "matchmakefons", "matchmakegoval", "matchmakesunel"
            },
            kills = {
                -- main quest
                "gobchiefdead", "helsassdead"
            },
            worldwide = {
                -- weather overrides
                "mournweather"
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Bloodmoon.esm") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- player state
                "pcwerewolf",
                -- quest variables that are already set correctly without being synced
                "trackerpause"
            },
            personal = {
                -- player state
                "pcknownreset",
                -- tavern rents
                "rent_ravenrock"
            },
            quest = {
                -- main quest
                "foundbooze", "artoriachosen", "luciuschosen", "stones", "part", "aesliptalk", "skaalattack", "trackercount",
                "huntercount", "cariustalk",
                -- Raven Rock
                "colonyside", "maryndetect", "colonystock"
            },
            kills = {
                -- main quest
                "smugdead", "riekkilled", "deaddaedra", "caenlorndead", "werewolfdead", "werebdead", "huntersdead",
                "trackersdead", "trollsdead", "krishcount",
                -- side quests
                "draugrkilled", "colonynord"
            },
            worldwide = {
                -- building construction
                "colonystate",
                -- NPCs present
                "colonyservice", "carniusloc", "aferguard", "garnasguard", "gratianguard"
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Tamriel_Data.ESM") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- MWSE/openmw lua variables
                "MWSE_BUILD", "T_Glob_OpenMwLuaUsed",
                -- game state
                "TR_MapPos", "TR_CellX", "TR_CellY", "TR_Test", "T_Glob_cleanup_x", "T_Glob_cleanup_y", 
                "T_Glob_cleanup_z", "T_Glob_cleanup_state", "T_Glob_DWelk_cleanup", "T_Glb_GetTeleportingDisabled", 
                "T_Glob_PassTimeHours", "T_Glob_GetTeleportingDisabled", "T_Glob_Speech_Debug", "T_Glob_Speech_Sway", 
                "T_Glob_Speech_Haggle", "T_Glob_Speech_Debate", 
                -- card game
                "T_Glob_CardHortX", "T_Glob_CardHortY",    "T_Glob_CardHortZ", "T_Glob_CardHortReshapeX", "T_Glob_CardHortReshapeY",
                "T_Glob_CardHortCol1Len", "T_Glob_CardHortCol2Len", "T_Glob_CardHortCol3Len", "T_Glob_CardHortCol4Len",
                "T_Glob_CardHortCol5Len", "T_Glob_CardHortCol6Len", "T_Glob_CardHortCol1Lock", "T_Glob_CardHortCol2Lock", 
                "T_Glob_CardHortCol3Lock", "T_Glob_CardHortCol4Lock", "T_Glob_CardHortCol5Lock", "T_Glob_CardHortCol6Lock",
                "T_Glob_CardHortCol1Lock2", "T_Glob_CardHortCol2Lock2", "T_Glob_CardHortCol3Lock2", "T_Glob_CardHortCol4Lock2", 
                "T_Glob_CardHortCol5Lock2", "T_Glob_CardHortCol6Lock2", "T_Glob_CardHortSaveLoad", "T_Glob_CardHortActiveLen",
                "T_Glob_CardHortTop", "T_Glob_CardHortDummy", "T_Glob_CardHortState", "T_Glob_CardHortTracker", "T_Glob_CardHortRow",
                "T_Glob_CardHortRot", "T_Glob_CardHortRank", "T_Glob_CardHortCol", "T_Glob_CardHortHouse",
                -- Nine holes
                "T_Glob_NineholesBet", "T_Glob_NineholesGameState", "T_Glob_NineholesNpcId", "T_Glob_NineholesOpponentId",
                "T_Glob_NineholesPlayerColor", "T_Glob_NineholesPlayerTurn", "T_Glob_NineholesPosTakenBy_01a", "T_Glob_NineholesPosTakenBy_01b",
                "T_Glob_NineholesPosTakenBy_01c", "T_Glob_NineholesPosTakenBy_02a", "T_Glob_NineholesPosTakenBy_02b", "T_Glob_NineholesPosTakenBy_02c",
                "T_Glob_NineholesPosTakenBy_03a", "T_Glob_NineholesPosTakenBy_03b", "T_Glob_NineholesPosTakenBy_03c", "T_Glob_NineholesPracticeMode",
                "T_Glob_NineholesSelectedPin", 
                -- Bank temp variable that doesn't need to be saved
                "T_Glob_Bank_All_TempAmount",
                -- Unused
                "T_Glob_JNS_BountyClear"
            },
            personal = {
                -- player state
                "T_Glob_PorphyricInfected", "T_Glob_WereInfected", "T_Glob_VampDamageRemove",
                -- Bank accounts
                "T_Glob_Bank_All_CurrentBank", 
                "T_Glob_Bank_Bri_AcctAmount", "T_Glob_Bank_Bri_LoanAmount", "T_Glob_Bank_Bri_LoanDate", "T_Glob_Bank_Bri_LoanFail",
                "T_Glob_Bank_Cmp_AcctAmount", "T_Glob_Bank_Cmp_LoanAmount", "T_Glob_Bank_Cmp_LoanDate", "T_Glob_Bank_Cmp_LoanFail",
                "T_Glob_Bank_Cro_AcctAmount", "T_Glob_Bank_Cro_LoanAmount", "T_Glob_Bank_Cro_LoanDate", "T_Glob_Bank_Cro_LoanFail",
                "T_Glob_Bank_Hla_AcctAmount", "T_Glob_Bank_Hla_LoanAmount", "T_Glob_Bank_Hla_LoanDate", "T_Glob_Bank_Hla_LoanFail",
                "T_Glob_Bank_Mas_AcctAmount", "T_Glob_Bank_Mas_LoanAmount", "T_Glob_Bank_Mas_LoanDate", "T_Glob_Bank_Mas_LoanFail",
                -- stock market, variable tracking how many stocks a player owns
                "T_Glob_StockCompareATC", "T_Glob_StockCompareBIC", "T_Glob_StockCompareCMC", "T_Glob_StockCompareCWA",
                "T_Glob_StockCompareEEC", "T_Glob_StockCompareNWT", "T_Glob_StockCompareRHC", "T_Glob_StockCompareSTC",
                "T_Glob_StockCompareWSC", "T_Glob_StockCountPlayerATC", "T_Glob_StockCountPlayerBIC", "T_Glob_StockCountPlayerCMC",
                "T_Glob_StockCountPlayerCWA", "T_Glob_StockCountPlayerEEC", "T_Glob_StockCountPlayerNWT", "T_Glob_StockCountPlayerRHC",
                "T_Glob_StockCountPlayerSTC", "T_Glob_StockCountPlayerWSC", "T_Glob_StockPayout001", "T_Glob_StockPayout005",
                "T_Glob_StockPayout010", "T_Glob_StockPayout050", "T_Glob_StockPayout100", "T_Glob_StockPayoutAll",
                "T_Glob_StockRequest", "T_Glob_StockSellCount", "T_Glob_StockSellPrice", "T_Glob_StockStarted",
                "T_Glob_StockValueTradedToday"
            },
            quest = {
                -- reputation
                "T_Glob_Rep_Cyr", "T_Glob_Rep_HR", "T_Glob_Rep_Ham", "T_Glob_Rep_MW", 
                "T_Glob_Rep_PI", "T_Glob_Rep_Sky",
                -- stock market prices, these react to quest outcomes
                "T_Glob_StockBaseATC", "T_Glob_StockBaseBIC", "T_Glob_StockBaseCMC", "T_Glob_StockBaseCWA",
                "T_Glob_StockBaseEEC", "T_Glob_StockBaseNWT", "T_Glob_StockBaseRHC", "T_Glob_StockBaseSTC",
                "T_Glob_StockBaseWSC", "T_Glob_StockPriceATC", "T_Glob_StockPriceBIC", "T_Glob_StockPriceCMC",
                "T_Glob_StockPriceCWA", "T_Glob_StockPriceEEC", "T_Glob_StockPriceNWT", "T_Glob_StockPriceRHC",
                "T_Glob_StockPriceSTC", "T_Glob_StockPriceWSC"
            },
            kills = {
            },
            factionRanks = {
            },
            factionExpulsion = {
                -- Cyrodiil faction explusions
                "T_Glob_Exp_Cyr_FG", "T_Glob_Exp_Cyr_TG", "T_Glob_Exp_Itin_Priests", "T_Glob_Exp_King_Anv",
                -- Skyrim faction explusions
                "T_Glob_Exp_Sky_FG", "T_Glob_Exp_Sky_TG" 
            },
            worldwide = {
                -- mechanisms
                "T_Glob_SutchElevDir", "T_Glob_SutchElevRest", "T_Glob_SutchElevUpDownCounter",
                -- Daedric wards
                "T_Glob_DaeWardAState", "T_Glob_DaeWardBState", "T_Glob_DaeWardCState", "T_Glob_DaeWardDState",
                "T_Glob_DaeWardEState", "T_Glob_DaeWardFState", "T_Glob_DaeWardGState", "T_Glob_DaeWardHState",
                "T_Glob_DaeWardIState", "T_Glob_DaeWardJState", "T_Glob_DaeWardKState", "T_Glob_DaeWardLState",
                "T_Glob_DaeWardMState", "T_Glob_DaeWardNState", "T_Glob_DaeWardOState", "T_Glob_DaeWardPState",
                "T_Glob_DaeWardQState", "T_Glob_DaeWardRState", "T_Glob_DaeWardSState", "T_Glob_DaeWardTState",
                "T_Glob_DaeWardUState", "T_Glob_DaeWardVState", "T_Glob_DaeWardWState", "T_Glob_DaeWardZState",
                -- objects
                "T_Glob_KingOrgCoffer_Uses", "T_Glob_KingOrgCoffer_Gold", "T_Glob_PlesioHatchPC", "T_Glob_TR_PreviewEnabled",
                -- news
                "T_Glob_News_AbMonitor_Pick1", "T_Glob_News_AbMonitor_Pick2", "T_Glob_News_AbMonitor_Tracker1", "T_Glob_News_AbMonitor_Tracker2",
                "T_Glob_News_Bellman_Pick1", "T_Glob_News_Bellman_Pick2", "T_Glob_News_Bellman_Tracker1", "T_Glob_News_Bellman_Tracker2",
                "T_Glob_News_Echo_Pick1", "T_Glob_News_Echo_Pick2", "T_Glob_News_Echo_Tracker1", "T_Glob_News_Echo_Tracker2",
                -- PTR interop variables, set via startup scripts
                "T_Glob_Installed_ABC", "T_Glob_Installed_Bkm", "T_Glob_Installed_Els", "T_Glob_Installed_HR427",
                "T_Glob_Installed_Ham", "T_Glob_Installed_PC", "T_Glob_Installed_PI", "T_Glob_Installed_SHotN",
                "T_Glob_Installed_Sum", "T_Glob_Installed_TR", "T_Glob_Installed_TRFM", "T_Glob_Installed_Val",
                "T_Glob_VanillaOverride"
            },
            unknown = { 
            }
        }
    }
    
    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "TR_Mainland.ESM") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- game state
                "TR_m3_q_Kha_BellTarget", "TR_m3_q_Kha_BellTracker", "TR_m3_OE_TarhielHaybale", "TR_m3_OE_customsfine",
                "TR_m3_OE_customsnote", "TR_m3_OE_resettlement", "TR_m3_OE_esoldereward", "TR_m4_HH_ScribPie_Baker",
                -- player state
                "TR_m3_OE_armisticetheft", "TR_m3_OE_truearmistice", "tr_m3_bloodstone_help_T", "tr_m3_bloodstone_help_E", "tr_m3_bloodstone_help_T",
                "tr_m3_bloodstone_help_O", "tr_m3_bloodstone_help_B", "tr_m3_bloodstone_help_N", "tr_m3_bloodstone_help_L",
                -- player equipment
                "TR_Hla_ScripCount", "TR_m3_EEC_CounterfeitCount", "TR_m3_EEC_CounterfeitReward",
                "TR_m3_TT_FS_helm_on", "TR_m3_TT_InqMantle", "TR_m4_Bal_PowerChosen", "TR_m7_Ns_FortunaAllowAccess",
                "TR_m7_Ns_ScripGoldDiff", "TR_m7_Ns_ScripGoldSum",
                -- quest variables that are already set correctly without being synced
                "TR_m3_bloodstonecheck", "TR_m4_MenaanTowerDoorGlobal", "TR_m4_Shenjirra_Gate", "TR_m4_TG_AncylisDoorGlobal",
                "TR_m4_TG_AndoSquatterSkooma", "TR_m4_TT_And_GoldCounter", "TR_m4_TT_And_GoldCounterB", "TR_m4_TT_And_ScripCounter",
                "TR_m4_WillToGoOn_Willpower", "TR_m4_vampvictim_incell", "TR_m7_EEC_SickGuar", "TR_m7_HH_Alvynu2_SlipInDesk",
                "TR_m7_HOSewerAlert", "TR_m7_Ns_FortunaQuestActive", "TR_m7_Ns_JNS_4_AnderaFollow", "TR_m7_Ns_JNS_4_FollowState",
                "TR_m7_Ns_JNS_4_ShouldFollow", "TR_m7_Ns_MG_Cont_glb", "TR_m7_Ns_TG_7_GoldIngots", "TR_m7_Ns_TG_7_GoldReward",
                "TR_m7_q_CE_CanLeashWorkers", "TR_m7_q_CE_DetectedByWorker", "TR_m7_q_CE_Dividends",
                -- unused
                "TR_Kick_FG", "TR_Kick_TG", "TR_Kick_TT", "TR_Kick_IC", "TR_Kick_MG",
                "TR_NecMQ_BoethiahRep", "TR_NecMQ_ICDestroyerRitual", "TR_NecMQ_ICMadmanRitual", "TR_NecMQ_MephalaRep",
                "TR_m2_NisirelConfronted","TR_m2_Rent_Helnim_Flower", "TR_m2_Rent_Helnim_Racer", "TR_m3_OE_TowerGhostTimer",
                "TR_m3_OE_smuggleeggsfound", "TR_m4_Andoth_AndasVaultGlobal", "TR_m4_Bal_MagicalLink", "TR_m4_VA_spellsword_glb",
                "TR_m4_namirachest_glb", "TR_m4_q_vamplure_glb", "TR_m7_AI_JNS_7_Alert", "TR_m7_HH_invest_bank",
                "TR_m7_HH_invest_dravos", "TR_m7_HH_invest_nord", "TR_m7_HH_lefttocollect", "TR_m7_Ns_HH_LetterOpnd",
                "TR_m7_q_rarethil_intimidated",
                -- MWSE/openMW Lua
                "TR_m1_FWMG_SupportLua",
                -- Will cause packet spam if synced
                "TR_m7_HH_Alvynu_7_ShipTimer"
            },
            personal = {
                -- player state
                "TR_m2_q_35_PCVampire", "TR_m4_AndasGuestState", "TR_m4_Bal_AmuletDayUsed",
                -- tavern rents
                "TR_m3_Rent_Bosmora_Starlight", "TR_m3_Rent_ED_Velk", "TR_m3_Rent_Gorne_EmeraldHaven",
                "TR_m2_Rent_Hlersis_Spore", "TR_m2_Rent_Akamora_Gob", "TR_m2_Rent_InnBetween", "TR_m2_Rent_Necrom_Hostel",
                "TR_m3_Rent_Meralag_Glade", "TR_m3_Rent_Darnim_Windbrk", "TR_Rent_HuntedHound", "TR_m1_Rent_Avenue",
                "TR_m2_Rent_Helnim_Drake", "TR_m2_Rent_Mothrivra_Goblet", "TR_m3_Rent_AT_TS", "TR_m2_Rent_TelGilan_Racer",
                "TR_m1_Rent_Black_Ogre", "TR_m1_Rent_Dancing_Jug", "TR_m1_Rent_Howling_Noose", "TR_m1_Rent_Queens_Cutlass", 
                "TR_m1_Rent_Waters_Shadow", "TR_m3_Rent_Sailen_Toiling", "TR_m3_Rent_Moth_and_Tiger", "TR_m3_Rent_Empress_Katariah", 
                "TR_m3_Rent_Salty_Futtocks", "TR_m3_Rent_Vhul_Hound", "TR_m3_Rent_Aimrah_Inn", "TR_m3_Rent_AT_HC", "TR_m3_Rent_AT_LS",
                "TR_m4_Rent_Ando_Council_Club", "TR_m4_Rent_Bodrum", "TR_m4_Rent_Dancing_Cup", "TR_m4_Rent_Golden_Moons",
                "TR_m4_Rent_Grey_Lodge", "TR_m4_Rent_Guar_No_Name", "TR_m4_Rent_Lucky_Shalaasa", "TR_m4_Rent_Omaynis",
                "TR_m4_Rent_Teyn", "TR_m4_Rent_Uman", "TR_m7_Rent_AldMarak", "TR_m7_Rent_FightingChance", "TR_m7_Rent_HK_Hound",
                "TR_m7_Rent_HK_Mudcrab", "TR_m7_Rent_MaarBani", "TR_m7_Rent_Mavandes", "TR_m7_Rent_Narsis_Canyon", "TR_m7_Rent_Narsis_Dragon",
                "TR_m7_Rent_Narsis_Greenshade", "TR_m7_Rent_Narsis_LastDrop", "TR_m7_Rent_Narsis_Temple", "TR_m7_Rent_OthmuraMuck",
                "TR_m7_Rent_SapphireSlouch", "TR_m7_Rent_SeptG_Niben", "TR_m7_Rent_StormGP_Saxhleel",
                -- miscellaneous variables related to player-specific actions
                "TR_m2_IC_HauntingShield", "TR_m3_Sa_TreramAlert", "TR_m3_q_AT_Armiger_Bid", "TR_m4_AA_SehutuAggro",
                "TR_m3_Kha_Fountain_Cooldown", "TR_m3_Sa_PearlCurseTimer", "TR_m2_445_BlessingA", "TR_m2_445_BlessingS", "TR_m2_445_BlessingV",
                "TR_m4_Andas_VaultBreakInGlobal", "TR_m4_TG_ArantamoDispChange", "TR_m4_TT_OrcHostile", "TR_m4_WLCR_GuardsWarn",
                "TR_m4_q_AAB_moneystolen", "TR_m4_q_PickledFishStolen", "TR_m7_LongJump_var", "TR_m7_Narsis_2ndVaultPerm",
                "TR_m7_Ns_ArenaBetAmount", 
                -- Mercenary Contracts
                "TR_Merc_m7_Amata_DaysLeft", "TR_Merc_m7_Amata_K_Day", "TR_Merc_m7_Amata_K_Month",
                -- Other followers
                "TR_m4_AA_JSuhkurrStatus", "TR_m4_AA_KhesurraStatus", "TR_m4_AA_TharmadalionStatus", "TR_m7_AI_JNS_6_MarisFollow",
                -- Pets
                "TR_m3_OE_Pack_Guar_glob"
            },
            quest = {
                -- main quest
                "TR_Fac_NerevarineCount", "TR_NecMQ_CalmOrd", "TR_m2_445_KeyATracker", "TR_m2_445_KeySTracker",
                "TR_m2_445_KeyVTracker",
                -- reading of confidential documents
                "TR_m3_q_AT_recipe_sealed", "TR_m3_TT_IduraUnseal", "TR_m4_AndoHH_LetterID", "TR_m4_AndoHH_UnsealLetter",
                "TR_m7_HH_Dren_q_Orvas1_Seal", "TR_m7_HH_Dren_q_Vedam1_Seal", "TR_m7_HH_SvoshGlobal", "TR_m7_HH_UnsealedLlananu",
                "TR_m7_Ns_HH_SarithaSeal", "TR_m7_q_LetterToSodreru_seal", "tr_m7_oth_q_eldavelletter_seal",
                -- assassination quests
                -- Note: These are related to kill counts, but they make more sense being shared across
                --       players who share quests than being shared across players who share kills
                "TR_m3_Zanammu_LichDead", "TR_m2_q_27_guardkilled", "TR_m2_q_35_dead", "TR_m3_q_4_dead", "TR_m3_OE_TG_AntioDead",
                "TR_m3_TT_CalitiaPreKilled", "TR_m3_TT_Lloris5IndCount", "TR_m4_VysAssaDead", "TR_m3_KH_kraskiradead",
                -- other side quests
                "TR_M7_HR_AM_Q2_ShellDone","TR_m0_HH_NilenoLetter","TR_m0_YakinBaelQuestHook",
                "TR_m1_FG_Mashugsaved","TR_m1_FG_Salirazkill","TR_m1_FG_StalkerTrappedDay","TR_m1_FW_BrazenCrateSeen",
                "TR_m1_FW_IC6_LoreCount","TR_m1_FW_TG6_Attack", "TR_m1_FW_TG6_BoneBroken", "TR_m1_FW_TG6_TimesCaught",
                "TR_m1_FW_TG_StoneWorn","TR_m1_HT_FakeLedgerPlanted", "TR_m1_IC6_Daisychainglobal", "TR_m1_IL_Darnelltalked",
                "TR_m1_Niv_JustASip_IncarnOwner", "TR_m1_RunningTroubleRewardFW","TR_m1_TT_2_PTCommonersReward","TR_m1_TT_2_PTSlavesReward",
                "TR_m1_TT_3_LTCommonersReward","TR_m1_TT_5_RRTotalDealtWith","TR_m2_Ak_HrothdorMad", "TR_m2_CantKill_MT",
                "TR_m2_HT_Vaerin_RitualDone", "TR_m2_He_MG_LamplitSearch", "TR_m2_He_TG_Done","TR_m2_MG_Aka_drimsu",
                "TR_m2_q_Nm_Wake_done", "TR_m3_AT_Silsiplayer", "TR_m3_AT_TG_Q1_AggroCounter", "TR_m3_AT_TG_Q1_GoldCount",
                "TR_m3_AT_TG_Q1_OrdWarning", "TR_m3_Brother_HelmDropped", "TR_m3_Brother_HelmTaken", "TR_m3_Brother_TrialReset",
                "TR_m3_EEC_Cano3_TalkingTo", "TR_m3_EEC_GoldDelays", "TR_m3_EEC_GoldJodald", "TR_m3_EEC_GoldReward",
                "TR_m3_EEC_GoldWayrian", "TR_m3_EEC_GoldYaguz", "TR_m3_EEC_MarkiFound", "TR_m3_EEC_RansomRequested",
                "TR_m3_EEC_SilverspoonTalks", "TR_m3_Hal_IndorilReward", "TR_m3_IrvasaSorvasInformed", "TR_m3_MT_DilsNoteFound",
                "TR_m3_OE_DAdelay", "TR_m3_WLCR_KidnappersGreet", "TR_m3_q_AdosiBiranDiscount", "TR_m3_q_DedicationFound",
                "TR_m3_q_3_thieving", "TR_Pilgrimages", "TR_m3_q_4_info", "TR_m3_q_3_info", "TR_m3_q_3_infoKiseen", "TR_m3_q_3_infoElegel", 
                "TR_m3_q_3_infoTemple", "TR_m3_q_3_infoFarys", "TR_m3_q_3_infoFarysWife", "TR_m2_q_38_sabotage", "TR_m2_q_38_talkedto", 
                "TR_m2_q_38_status", "TR_m2_q_A8_6_rushNPC", "TR_m3_q_3_rumour", "TR_m3_Bosvau_stolen", "TR_m3_Bosvau_stolenvalue", 
                "TR_m3_q_3_guardsGone", "TR_m2_MG_Aka_seeds", "TR_m2_MG_Aka_Francine1", "TR_m2_MG_Aka_karma", "TR_m2_MG_Aka_Polodie1reward",
                "TR_m2_MG_Aka_tarry", "TR_m3_q_5_Journal_Read", "TR_m3_q_5_distantJulie", "TR_m3_q_OE_MG_GCount", "TR_m3_OE_FG_q_FledFromVermai", 
                "TR_m3_OE_FG_q_AureCTalk", "TR_m3_q_givebartsword", "TR_m3_q_fiendbladegot", "TR_m3_q_fienddisappear", "TR_m3_q_treasurebladestolen",
                "TR_m3_OE_RumaGlobal", "TR_m3_q_NelynFathisTimer", "TR_m3_AT_SilentNight_stage", "TR_m3_AT_SilentNight_Rat", 
                "TR_m3_q_TheRiftDral", "TR_m3_q_TheRiftTilresi", "TR_m3_OE_MG_HallOpen", "TR_m3_Aim_ShipSneak", "TR_m3_q_Kassad_QuestionedNumbe",
                "TR_m3_TT_ProverbCounter", "TR_m3_TT_Lloris4Indoril", "TR_m3_TT_Lloris4Hlaalu", "TR_m3_VysAssanudCheck", "TR_m3_TT_LatestRumorATGlobal", 
                "TR_m3_Kha_SY_convinced", "TR_m3_Kha_SY_final", "TR_m3_TT_RIP_garvs_heresy", "TR_m3_TT_RIP_refusecount", "TR_m4_TJ_Court_State",
                "TR_m3_q_A3_Seen_Basement", "TR_m3_OE_elysanadiamondstole", "TR_m4_AA_BuriedSilver", "TR_m4_AA_DamiloBodyReveal",
                "TR_m4_AA_GuarCounter", "TR_m4_AA_GuarNassuran", "TR_m4_AA_GuarSeresa", "TR_m4_AA_IssarbaddonHostile",
                "TR_m4_AA_KuvatRent", "TR_m4_AA_SehutuFriend", "TR_m4_AA_UrnuridunGuarState", "TR_m4_And_MaterialMatters_g",
                "TR_m4_AndoHH_CTRating1", "TR_m4_AndoHH_CTRating2", "TR_m4_AndoHH_EggOffice", "TR_m4_AndoHH_EggStorage",
                "TR_m4_AndoHH_GreefDay", "TR_m4_AndoHH_GreefHour", "TR_m4_AndoHH_MelsPay", "TR_m4_AndoHH_NalvynaReward",
                "TR_m4_AndoHH_ShipTrespass", "TR_m4_AndoHH_SujammaCheck", "TR_m4_AndoWharfHideoutGlobal", "TR_m4_Ando_AlchemistsTimeCtrl",
                "TR_m4_Ando_HemmetteArrestDay", "TR_m4_Ando_HemmetteDetected", "TR_m4_Ando_HemmetteRumor", "TR_m4_Ando_NevusaDeskState",
                "TR_m4_Ando_NevusaOffer", "TR_m4_Ando_NevusaPotionPlaced", "TR_m4_Bal0_ArmasSpoken", "TR_m4_FG_LlaranStateB",
                "TR_m4_FG_LlaranStateC", "TR_m4_GavrosCT", "TR_m4_GavrosSixthHouse", "TR_m4_HH_AND_HearingGlob",
                "TR_m4_HH_AndasPoint", "TR_m4_HH_CaravanReleaseDay", "TR_m4_HH_NalvosAlvuru", "TR_m4_HH_NalvosDrink",
                "TR_m4_HH_NalvosRedoran", "TR_m4_HH_OlvysAccept", "TR_m4_HH_Ulvo1_LedgerRead", "TR_m4_IL_JizirrFreed",
                "TR_m4_IL_arveladvice", "TR_m4_IL_dalsadvice", "TR_m4_IL_darraadvice", "TR_m4_IL_grudgeplan",
                "TR_m4_Om_ReRe_ConfCnt","TR_m4_Om_ReRe_ForemanConf", "TR_m4_SpokeToDarane", "TR_m4_TG_Ando_JoKaarFree",
                "TR_m4_TG_SheiAdvGlb", "TR_m4_TG_UrnCounter", "TR_m4_TT_TakeItems", "TR_m4_T_Nuccius_Alomon_Status",
                "TR_m4_T_Nuccius_Sujamma1", "TR_m4_T_Nuccius_Sujamma2", "TR_m4_T_Nuccius_Sujamma3", "TR_m4_VA_AndasGreetOnce",
                "TR_m4_VA_Ibinai_Truth", "TR_m4_VA_resetscout", "TR_m4_Vf_SenipalFreed", "TR_m4_q_AG_Votes",
                "TR_m4_q_AG_Votes", "TR_m4_q_AG_candidate", "TR_m4_q_Euphoria_RingUsed", "TR_m4_q_PW_HibdunDrank",
                "TR_m4_q_PW_ToldMusa", "TR_m4_q_WWWtalked", "TR_m4_q_drowned_infoalch", "TR_m4_q_drowned_infoissmi",
                "TR_m4_q_drowned_infomages", "TR_m4_q_drowned_infopriest", "TR_m4_q_drowned_timepassed", "TR_m7_AI_JNS_3_BaryonGlob",
                "TR_m7_AI_JNS_6_Glob", "TR_m7_AI_TT_HlerasConfrBefore", "TR_m7_AT_TG_Q6_Council_Booze", "TR_m7_EEC_Caravandelay",
                "TR_m7_EEC_Feast", "TR_m7_EEC_Narsis2_itemcount", "TR_m7_EEC_Narsis3_weightsplace", "TR_m7_EEC_Narsis_Book",
                "TR_m7_EEC_Narsis_Helm", "TR_m7_EEC_Narsis_Hide", "TR_m7_EEC_Narsis_Pot", "TR_m7_EEC_Narsis_Saltrice",
                "TR_m7_EEC_Narsis_Shield", "TR_m7_EEC_Narsis_Skirt", "TR_m7_FamilyAffair_RavosReturn", "TR_m7_FamilyAffair_TalkUlyne",
                "TR_m7_HH_Alvynu_4_SpeakerGhost", "TR_m7_HH_Alvynu_6_SpiritsDealt", "TR_m7_HH_AxeReturn", "TR_m7_HH_ElarFramed",
                "TR_m7_HH_FirstFamily_smug", "TR_m7_HH_HasGuarLiver", "TR_m7_HH_fixbreach", "TR_m7_HH_grandmasterask",
                "TR_m7_HL_FG_Q4_FightStarted", "TR_m7_HL_FG_Q5_SlavesFreed", "TR_m7_HL_FG_Q6_FightStarted", "TR_m7_HOGuideEnt",
                "TR_m7_HOGuideInf", "TR_m7_HOGuideRuin", "TR_m7_HO_HFS_HouseOffer", "TR_m7_HO_TT_01_Donations",
                "TR_m7_HO_TT_05_askedIda", "TR_m7_HO_TT_05_askedMG", "TR_m7_HO_TT_05_crooknotefound", "TR_m7_HO_TT_07_DadrysAngry",
                "TR_m7_HR_AldIuval_q1_ShojaFree", "TR_m7_HindsightPlanted", "TR_m7_IC_FeedPoorCountAI", "TR_m7_IC_HL_AlmsAnnalina",
                "TR_m7_IC_HL_AlmsBolnor", "TR_m7_IC_HL_AlmsCorter", "TR_m7_IC_HL_AlmsDithisi", "TR_m7_IC_HL_AlmsDoZhisi",
                "TR_m7_IC_HL_AlmsHvithar", "TR_m7_IC_HL_AlmsKhazor", "TR_m7_IC_HL_AlmsLinriah", "TR_m7_IC_HL_AlmsPeleri",
                "TR_m7_IC_HL_AlmsRuvene", "TR_m7_IC_HL_AlmsTaderi", "TR_m7_JNS_TheBoss_HammerReward", "TR_m7_Ns_ArenaCorrectDoor",
                "TR_m7_Ns_ArenaFightChoice", "TR_m7_Ns_BilchabulBetray", "TR_m7_Ns_BilchabulSuryvnAngry", "TR_m7_Ns_Caravan_Casino",
                "TR_m7_Ns_Caravan_Key", "TR_m7_Ns_FG_2_HasSilver", "TR_m7_Ns_FG_6_MinerState", "TR_m7_Ns_IC6_SavePriest",
                "TR_m7_Ns_IL_3_Glob", "TR_m7_Ns_JNS_2_SoldGreydust", "TR_m7_Ns_JNS_4_Caught", "TR_m7_Ns_JNS_7_AttackTong",
                "TR_m7_Ns_MG_Ench_Glb", "TR_m7_Ns_MG_QuestsDone_glb", "TR_m7_Ns_NucciusRumor", "TR_m7_Ns_StageFright_Mistakes",
                "TR_m7_Ns_TG_4_Greydust", "TR_m7_Ns_TG_8_GuardState","TR_m7_Ns_TG_8_GuardWarn", "TR_m7_Ns_TG_8_HeistComplete",
                "TR_m7_Ns_TG_9_GuardState", "TR_m7_Ns_TG_9_GuardWarn", "TR_m7_Ns_ToTheLastDropFound", "TR_m7_NymphAttack",
                "TR_m7_Oth_BrothersLove_Glob", "TR_m7_Oth_MG_ApproachEndRoom", "TR_m7_Oth_MG_ApproachSigilBarr", "TR_m7_Oth_MG_ApproachTeleKey",
                "TR_m7_Oth_MG_ApproachWaterRoom", "TR_m7_Oth_MG_MainDoorUnlocked", "TR_m7_Oth_MG_RubbleCleared", "TR_m7_Oth_MG_TeleDoorUnlocked",
                "TR_m7_SM_QuestStarted", "TR_m7_Sdr_Defiling_MuranAsked", "TR_m7_Sharai_TL_Access", "TR_m7_ShiSha_HH_Q4Flin",
                "TR_m7_ShiSha_Haunt_CleanCount", "TR_m7_TejNaFound", "TR_m7_VA_AniVamp_InfectCount", "TR_m7_q_AI_OrnadaCured",
                "TR_m7_q_CE_WorkersBought", "TR_m7_q_CE_WorkersCollected", "tr_m7_Jhasa-DarFreed"
            },
            kills = {
                "TR_HH_indorilspiritkill_glb", "TR_m1_HT_DralDead", "TR_m1_HT_EldaleDead", "TR_m1_HT_FarunaDead",
                "TR_m1_HT_MithrasDead", "TR_m1_HT_RathraDead", "TR_m1_HT_Rathra_q6_MineDead", "TR_m1_IL_cultistcount",
                "TR_m1_TT_5_DeadCount", "TR_m1_TT_7_VampDead", "TR_m1_q_ArthalDead", "TR_m1_q_DilnarDead",
                "TR_m1_q_VeranaDead", "TR_m2_Ak_HrothdorDead", "TR_m2_FG_cultist_counter", "TR_m2_HT_VaerinDead",
                "TR_m2_HT_Vaerin_DaedraDead", "TR_m2_WindbreakerSmugglerDead", "TR_m3_AT_TG_Q6_ThugsDeadCount", "TR_m3_KadanuranDeadCount",
                "TR_m3_OE_RumaDead", "TR_m3_Sa_IdrenieDead", "TR_m3_WLCR_FightDeath","TR_m3_WLCR_KidnappersDead",
                "TR_m3_q_RD_HlesGangDead", "TR_m4_AA_SehutuDead", "TR_m4_And_Bounty_Bone_Dead", "TR_m4_And_Bounty_Holst_Dead",
                "TR_m4_And_Bounty_Runat_Dead", "TR_m4_And_Bounty_Vyper_Dead", "TR_m4_And_Bounty_ra_Dead", "TR_m4_AndoHH_ZalanDeath",
                "TR_m4_Ando_HemmetteDead", "TR_m4_Ando_NevusaDead", "TR_m4_Bal_DevourerDead", "TR_m4_Barendreth_DeadOrd",
                "TR_m4_FG_AlitGlobal", "TR_m4_HH_WorkerDeathCounter", "TR_m4_LostTransit_AshDead_glb", "TR_m4_Om_ReRe_KillCount",
                "TR_m4_RR_ArgoBanditGlob", "TR_m4_TG_SquatterDead", "TR_m4_VA_DeadAndas", "TR_m4_VA_deadhlaalu_glb",
                "TR_m4_Vf_Mabrigash_Count", "TR_m4_VysAssaDead", "TR_m4_WillToGoOn_Dead", "TR_m4_baluathdead",
                "TR_m4_q_Credkill", "TR_m4_q_TTSCoraneDead", "TR_m4_q_TTSFranDead", "TR_m4_q_TTSGhostDead",
                "TR_m4_q_invadersdead_glb", "TR_m4_q_mm_Ashvudead", "TR_orlukhkillcount", "TR_m7_AI_JNS_5_SmugglerDead",
                "TR_m7_EEC_Shinathidead", "TR_m7_EEC_Tongdead", "TR_m7_EEC_farmerdeadcount", "TR_m7_EEC_killfarmers_glob",
                "TR_m7_ErnasiViranDead", "TR_m7_EssurnashpiDeadCount", "TR_m7_HH_Alvynu2_Murder", "TR_m7_HH_Alvynu_7_CCKilled",
                "TR_m7_HL_FG_Q2_DeadCount", "TR_m7_HL_FG_Q5_DeadCount", "TR_m7_HL_FG_Q6_DeadCount", "TR_m7_HO_HighAndDry_CrewKilled",
                "TR_m7_HR_AM_Q1_AtemuNixDeath", "TR_m7_HR_AM_Q2_Shellcountdown", "TR_m7_HR_AM_Q4_BanditDead", "TR_m7_HR_WM_NilusDead",
                "TR_m7_Kalk_Fight_GoblinDead", "TR_m7_Kalk_Fight_OrcDead", "TR_m7_KurikiDeadCount", "TR_m7_Narsis_SM_killed",
                "TR_m7_NsTT_ShinApostKillCount", "TR_m7_Ns_ArenaBeast", "TR_m7_Ns_JNS_3_GuardiansKilled", "TR_m7_Ns_JNS_4_JandieGangKille",
                "TR_m7_Ns_JNS_7_NegotiatorsDead", "TR_m7_Ns_StageFright_ActorKill", "TR_m7_Oth_MG_1_SquatterDead", "TR_m7_SuppliesSDPiratesGlobal",
                "TR_m7_q_Zarathil_Smugglers"
            },
            factionRanks = {
                -- Subfaction globals
                "TR_m3_TT_SpeakerState",
                -- Roundabout faction checks stored in globals?
                "TR_m4_Om_ReRe_FacReq"
            },
            factionExpulsion = {
                "TR_m7_HO_TT_04_MegaExpelled"
            },
            worldwide = {
                -- mechanisms
                "TR_Necrom_FanePortL", "TR_Necrom_FanePortR", "TR_m1_q_Bthal_CrystalTarget", "TR_m1_q_Bthal_CrystalTracker",
                "TR_m2_MzankhDoorState", "TR_Necrom_StairsState", "TR_Necrom_MachineState", "TR_Necrom_VaultPortR", 
                "TR_Necrom_VaultPortL", "TR_Necrom_DoorState", "TR_m2_445_grindertimer", "TR_m2_445_grinderangle1", 
                "TR_m3_Aim_GilaWallBreak", "TR_m3_Aim_LighthouseSecretDoor", "TR_m3_OE_pitgate", "TR_m3_OE_CuriaVaultGate2", 
                "TR_m3_OE_sewergate", "TR_m2_kmlz_Chef_WaterLevel", "TR_m3_OE_ETCensusBlockDoor", 
                "TR_m3_OE_CuriaVaultGate", "TR_m3_OE_CuriaVaultGlobal", "TR_m3_OE_TG_waterlevel", "TR_m3_OE_chapelsewerdoor",
                "TR_m3_OE_raathim_sarcophagus", "TR_m3_q_OE_UrienChest_glb", "TR_Necrom_AllowVaultEntry",
                "TR_m4_AndasLiftGlobal1", "TR_m4_AndasLiftGlobal2", "TR_m4_AndasSewerAccess", "TR_m4_AndasSewerGateState",
                "TR_m4_BthungthuvDoorGlobal", "TR_m4_FelmsLiftGlobal1", "TR_m4_FelmsLiftGlobal2", "TR_m4_ShalmuratGateAccess",
                "TR_m4_TG_AndoBaseLiftGlobal", "TR_m4_TG_AndoBaseLiftGlobal2", "TR_m4_UshuKurLiftGlobal", "TR_m4_UshuKurLiftGlobal2",
                "TR_m4_UshuKurLiftGlobal3", "TR_m4_UshuKurLiftGlobal4",
                -- building construction
                "TR_FM_Glob_State", "TR_m7_NVA_BuildStage", "TR_m3_OE_ETCensus_RepairState", "TR_m3_OE_ETCensus_Stanchion",
                "TR_m4_Oma_InnStage", "TR_m4_TG_AndoBaseBanners", "TR_m4_TG_AndoBaseBar", "TR_m4_TG_AndoBaseBeds",
                "TR_m4_TG_AndoBaseCleanUp", "TR_m4_TG_AndoBasePlants", "TR_m4_TG_AndoBaseRugs", "TR_m4_TG_AndoBaseTraining",
                "TR_m4_TG_AndoBaseTraps", "TR_m7_LA4_EggMineStage",
                -- objects
                "TR_m1_q_Bthalagstate", "TR_m2_HT_Vaerin_Q6Done", "TR_m3_fiendrandomizer_glb",
                "TR_m3_MaesabunMummyAwake", "TR_m3_AT_LatikaPitcher", "TR_m3_vontuswalk", "TR_m4_And_SheKindlySpoke_Stash",
                "TR_m4_AndasTombFlame", "TR_m4_AndasTombState", "TR_m4_AndoHH_CrateRemove", "TR_m4_AndoHH_ShipDisable",
                "TR_m4_AndoHH_ShipReleased", "TR_m4_TG_ThoriclesCheck", "TR_m4_VA_budaktrigger", "TR_m4_orlukhgate02_glb",
                "TR_m7_GauntletPoorL_Taken_glb", "TR_m7_GauntletPoorR_Taken_Glb", "TR_m7_HH_Alvynu_7_FlsgrtActive",
                "TR_m7_JNS_UddanuDaeCrossbowGlb", "TR_m7_SM_AlembicFound", "TR_m7_SM_CalcinatorFound", "TR_m7_SM_MortarFound",
                "TR_m7_SM_RetortFound", "TR_m7_Sharai_WinchesterStolen",
                -- people
                "TR_NecMQ_SchemerState","TR_m1_IL_Arloteleported","TR_m1_IL_Darnellgoonce", "TR_m1_IL_Darnellwin",
                "TR_m1_IL_guarstate", "TR_m1_TT_5_TimerOver", "TR_m1_q71_timeout","TR_m3_AT_RatFriend_Prison",
                "TR_m3_AT_TG_Q6_ThugsDisable", "TR_m3_AT_Toldmerchant", "TR_m3_EEC_VarusoFreed", "TR_m3_EEC_YakFreed",
                "TR_m3_EEC_YontusFreed", "TR_m3_MoveRathysMadalvel", "TR_m2_WM_Rethrathi", "TR_m2_q_29_shambaludridrea", 
                "TR_m4_TJ_OgrimStatus", "TR_m3_TT_Illene1_ChaseGlobal", "TR_m4_BahrundGlobal", "TR_m4_Bal_DevourerSpawned",
                "TR_m4_DredaseDevani", "TR_m4_FG_AndasInVault", "TR_m4_FG_JubalGlobal", "TR_m4_FG_OrblosGlobal",
                "TR_m4_HH_AnbarysGuardsMove", "TR_m4_HH_WorkerDisable", "TR_m4_SkelWiz_HlaaluEnabled", "TR_m4_TG_AndoSkoomaCat",
                "TR_m4_TJ_OgrimStatus", "TR_m4_T_Nuccius_Delay", "TR_m4_Uman_B2_DiraBrought", "TR_m4_q_TMM_Bolsleave",
                "TR_HH_GM_enable_trainers", "TR_m7_AI_JNS_4_Glob", "TR_m7_AI_TT_HouseSupport", "TR_m7_AI_TT_MuransFrustration",
                "TR_m7_FelvynGangEnable", "TR_m7_HH_Alvynu_4_IlvrinActive", "TR_m7_HH_Alvynu_5_AssState", "TR_m7_HH_Alvynu_6_TombState",
                "TR_m7_Ida_NetMoved", "TR_m7_JNS_TheBossAttacked", "TR_m7_MakingPeace_Return", "TR_m7_Ns_FortunaTongDisable",
                "TR_m7_Ns_IL_6_VanikenDeadGlb", "TR_m7_Ns_SlavesAvailableArg", "TR_m7_Ns_SlavesAvailableKha", "TR_m7_Ns_SlavesAvailableOth",
                "TR_m7_Oth_MG_1_SquatterState", "TR_m7_Proconsul_dead_glb", "TR_m7_VA_AniVamp_TestState", "TR_m7_VurvynOthrenDead",
                -- both
                "TR_m1_FG_StalkerTrapped", "TR_m3_Hal_TowerState", "TR_m3_veloth_shrine_spawncount", "TR_m4_Ando_HemmettePrison",
                "TR_m4_FG_UshuFree", "TR_m4_HH_SaboteurAction","TR_m4_HH_SavrethiAlive", "TR_m4_HH_TeraniRescued",
                "TR_m4_LostTransit_Control_glb", "TR_m4_LostTransit_Days_glb", "TR_m4_Om_ReRe_Ordinators", "TR_m4_TG_AndoAttack",
                "TR_m4_TG_AndoShei6Global", "TR_m4_TG_AndoSideQuest", "TR_m4_TJ_Court_State", "TR_m4_TT_AndothrenFinale",
                "TR_m7_IbbiSuen_MineState", "TR_m7_Ns_IL_3_Culprit", "TR_m7_Ns_MoonshinerEnds", "TR_m7_Ns_RescuePotLady_Moved",
                -- events
                "TR_FM_Glob_Weather", "TR_m3_OE_StendarrIdolsOutlawed", "TR_Thirr_Conflict_Score", "TR_Thirr_Conflict_Heat", 
                "TR_m3_TT_g_ritstart", "TR_m4_NirnBoundGlobal1", "TR_m4_OssurClannfearGlobal", "TR_m4_TT_DepartDay",
                "TR_m4_Vf_TimerGlb", "TR_m4_q_TTSCombatState", "TR_m7_Ns_ArenaCurrentDuel", "TR_m7_Ns_ArenaDuelActive",
                "TR_m7_Ns_ArenaDuelBegun", "TR_m7_Ns_ArenaGenericFight", "TR_m7_Ns_IL_7_Raid", "TR_m7_Ns_StageFright_ActState",
                "TR_m7_nightmotherattack"
            },
            unknown = {
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Cyr_Main.esm") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- not actually used at all
                "PC_FavorAkatosh", "PC_FavorArkay", "PC_FavorDibella", "PC_FavorJulianos",
                "PC_FavorKynareth", "PC_FavorMara", "PC_FavorStendarr", "PC_FavorTalos",
                "PC_FavorZenithar", "PC_Glb_ExpFightersGuild", "PC_m1_MG_Cha2_Debt",
                -- Player equipment
                "PC_m1_AFP_Costume_sc",
                -- Set correctly as is
                "PC_m0_Vva_TropVac_Dest", "PC_m0_Vva_TropVac_Hours", "PC_m1_Anv_Bounty_CrypsisHours"
            },
            personal = {
                -- tavern rents
                "PC_m1_Rented_Abecette", "PC_m1_Rented_AllFlags", "PC_m1_Rented_AnchorsRest", "PC_m1_Rented_BlindWatchtower",
                "PC_m1_Rented_Caravan", "PC_m1_Rented_Crossing", "PC_m1_Rented_Gosha", "PC_m1_Rented_IronMan",
                "PC_m1_Rented_OldSeawater", "PC_m1_Rented_Spearmouth", "PC_m1_Rented_Sunset",
                -- Mercenary Contracts
                "PC_m1_M_CylinaDaysLeft", "PC_m1_M_CylinaStartDay", "PC_m1_M_CylinaStartMonth",
                "PC_m1_M_SeguriusDaysLeft", "PC_m1_M_SeguriusStartDay", "PC_m1_M_SeguriusStartMonth",
                "PC_m1_M_TorbarnDaysLeft", "PC_m1_M_TorbarnStartDay", "PC_m1_M_TorbarnStartMonth",
                -- player actions
                "PC_m0_Vva_TropVac_TravelKaltan", "PC_m0_Vva_TropVac_TravelTitus", "PC_m1_Anv_BlkView_Detected", "PC_m1_Anv_BlkView_SoulGemType",
                "PC_m1_Anv_WorkOrc_Disp", "PC_m1_CrypsisCrew_Aggro", "PC_m1_IP_Lki4_StateRitual", "PC_m1_IP_Lki_DibellanKilled",
                "PC_m1_IP_Run2_Killed", "PC_m1_PadrulRingState", "PC_m1_SC_GarAge_ArchaeKilled", "PC_m1_SC_GarAge_AuroranSummoned",
                "PC_m1_SC_GarAge_BucynarelKeyst", "PC_m1_SC_GoatTrbls_Attacked", "PC_m1_TG_Anv4_BetAmount", "PC_m1_TG_Anv4_BetState",
                "PC_m1_TG_Anv4_WinAmount", "PC_m1_TG_Cha4_Detected",
                -- Once off rumor variables
                "PC_m1_Anv_WellMet_DialFilter", "PC_m0_Vva_TropVac_Rumor"
            },
            quest = {
                -- Miscellaneous quest related variables
                "PC_m1_Anv_AdvRead_BarubiState", "PC_m1_Anv_AdvRead_PotionOffered", "PC_m1_Anv_BlkView_Storming", "PC_m1_Anv_BookClub_State",
                "PC_m1_Anv_GlimpseBodyFound", "PC_m1_Anv_ImpCause_HasiDisobey", "PC_m1_Anv_OceanBlue_ATCGold", "PC_m1_BC_KhaRaji_SealedLtrGlb",
                "PC_m1_Cha_Cassynder_Drugs", "PC_m1_Cha_FigSpeech_AmulStolen", "PC_m1_Cha_PelLeg_Refuse", "PC_m1_FG_Anv5_PaintingOwner",
                "PC_m1_IP_Als3_NoteSkip", "PC_m1_IP_Als4_Convince", "PC_m1_IP_GS2_Donation", "PC_m1_IP_HY1_StateKuram",
                "PC_m1_IP_HY2_NoHostile", "PC_m1_IP_Lki4_KeladRattedOut",  "PC_m1_IP_Lki4_PCDrowner", "PC_m1_IP_Run1_CanPay",
                "PC_m1_IP_Run1_Donations", "PC_m1_IP_Run2_Cured", "PC_m1_IP_Run3_Persarine", "PC_m1_K1_HT1_SolvusTold",
                "PC_m1_K1_HT1_TowerRobbed", "PC_m1_K1_HT_HighAlert", "PC_m1_K1_MC2_Method", "PC_m1_K1_MC6_BoardConvinced",
                "PC_m1_K1_MC8_Aftermath", "PC_m1_K1_RP2_CultTalked", "PC_m1_K1_RP3_NoCash", "PC_m1_MG_BC1_TorioTalked",
                "PC_m1_MG_BC3_RingCheck", "PC_m1_MG_Cha4_AuroranSummon", "PC_m1_MG_Cha4_BigUpArdavan", "PC_m1_StrokeFort_BeatUp",
                "PC_m1_TG_Anv7_Outcome", "PC_m1_TG_Anv7_Points", "PC_m1_Tvy_NobleDebt_Convinced",
                -- Bounty quests
                "PC_m1_Anv_Bounty_AnnkaCaptured", "PC_m1_Anv_Bounty_MCCaptured", "PC_m1_Anv_Bounty_RueCaptured", "PC_m1_Anv_Bounty_RycimaCaptured"
            },
            kills = {
                -- Misc quests
                "PC_m1_Anv_BookClub_DeadCount", "PC_m1_Anv_MidLife_BanditDead", "PC_m1_Anv_MidLife_OrcDead","PC_m1_FG_Anv5_GuardDeath",
                "PC_m1_IP_HY2_BanditKilled", "PC_m1_K1_HT5_GuardDead", "PC_m1_K1_MC6_BoardKilled", "PC_m1_MG_Anv5_UndeadKilled",
                "PC_m1_MG_Cha4_GobsKilled", "PC_m1_Tvy_ThresvyDef_Kills"
            },
            factionRanks = {
            },
            factionExpulsion = {
            },
            worldwide = {
                -- mechanisms
                "PC_m1_FHe_GateState", "PC_m1_FSm_WallState",
                -- Actor/object state variables
                "PC_m1_Anv_Bounty_CrypsisMoved", "PC_m1_Anv_EnmanAirshipState", "PC_m1_Anv_EnmanAirshipState", "PC_m1_Anv_WellMet_TrackReymanus",
                "PC_m1_Anv_WellMet_TrackWynn", "PC_m1_Cha_GhastOrd_Moved", "PC_m1_Cha_GoldNets_Meeting", "PC_m1_Cha_MG_NymonaHatchGlob",
                "PC_m1_FHe_ElvState","PC_m1_IP_Als1_Return", "PC_m1_IP_HY_StateMove", "PC_m1_K1_HT1_StateSonkha",
                "PC_m1_K1_HT4_MeetDay", "PC_m1_K1_RP2_WallFixed", "PC_m1_K1_VT_KyroState", "PC_m1_TG_Cha4_DI",
                -- Arena state
                "PC_m1_AFP_DuelActive", "PC_m1_AFP_DuelCurrent", "PC_m1_AFP_DuelMulti", "PC_m1_TG_Anv4_MatchState"
            },
            unknown = {
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

if tableHelper.containsCaseInsensitiveString(clientDataFiles, "Sky_Main.esm") then

    local addedVariableScopes = {
        globals = {
            ignored = {
                -- game state
                "Sky_TempVar_glb",
                -- quest variables that are already set correctly without being synced
                "Sky_qRe_DSW4_BreadCounter_glb",
                -- not actually used at all
                "Sky_qRe_DH5_Wine_glb", "Sky_qRe_DSE_Register04_glb", "sky_qRe_KG4_AmbCount", "sky_qRe_KG4_Day", 
                "sky_qRe_KG4_Day2" 
            },
            personal = {
                -- tavern rents
                "Sky_Rent_DSE_Shadowkey", "Sky_Rent_DSW_DragonFountain", "Sky_Rent_DSW_NukraTikil", 
                "Sky_Rent_HA_Jhorcian", "Sky_Rent_KW_Dancing_Saber", "Sky_Rent_KW_Ruby_Drake",
                "Sky_Rent_LH_Daracam", "Sky_Rent_MER_Rhuma", "Sky_Rent_VF_EvenOddsInn",
                -- mercenary contracts
                "Sky_Merc_Rismund_DaysLeft", "Sky_Merc_KW_Rismund_K_Day", "Sky_Merc_KW_Rismund_K_Month",
                "Sky_Merc_DSW_Zanarhi_DaysLeft", "Sky_Merc_DSW_Zanarhi_K_Day", "Sky_Merc_DSW_Zanarhi_K_Month",
                -- miscellaneous variables related to player-specific actions
                "Sky_qRe_BM4_Door_glb", "Sky_qRe_HA1_SkullPlaced_glb", "Sky_qRe_KW1a_Journal_glb", "Sky_qRe_KW1b_Journal_glb",
                "Sky_qRe_KW1c_Journal_glb", "Sky_qRe_KW1d_Journal_glb", "Sky_qRe_KW_SogatAggro"
            },
            quest = {
                -- other side quests
                "Sky_qRe_DH2_glb", "Sky_qRe_DSB5_Counter_glb", "Sky_qRe_DSE1_Donation_glb",
                "Sky_qRe_DSTG3_Informants_glb", "Sky_qRe_DSTG7_MoveCael_glb", "Sky_qRe_DSW1_CacheFound_glb", "Sky_qRe_DSW1_Dagger_glb",
                "Sky_qRe_DSW1_Scimitar_glb", "Sky_qRe_DSW1_Saber_glb", "Sky_qRe_DSW2_Auth_glb", "Sky_qRe_KG5_SViir_glb",
                "Sky_qRe_KWFG4_Owner_glb", "Sky_qRe_KWTG8_Owner_glb", "Sky_qRe_MAI4_Counter_glb", "Sky_qRe_NAR1_Investigate_glb"
            },
            kills = {
                -- main quest
                "Sky_qRe_DSMQ_AlaktolDead", "Sky_qRe_DSMQ_JonaDead",
                -- side quests
                "Sky_qRe_DSW1_MesaraDead_glb", "Sky_qRe_BM_FhegainDead_glb", "Sky_qRe_HA1_KillCheck_glb", "Sky_qRe_KG1_glb", 
                "Sky_qRe_KG2_Counter_glb", "Sky_qRe_KG4_Counter", "Sky_qRe_KW3_Counter_glb", "sky_qRe_KWFG3_Counter",
                "Sky_qRe_KWMG6_Counter_glb", "Sky_qRe_VF1_Died_Glb", "Sky_qRe_MAI03_Counter_glb", "Sky_qRe_MAI4_Dead_glb",
                -- arena kill counts
                "Sky_qRe_DSE4_Count02_glb", "Sky_qRe_DSE4_Count03_glb", "Sky_qRe_DSE4_Count05_glb", "Sky_qRe_DSE4_Count07_glb"
            },
            factionRanks = {
                -- faction reputation
                "Sky_qRe_DSMG_Rep_Glb", "Sky_Rep_FireHand_glb",
                -- membership in mini-factions
                "Sky_qRe_DSE4_Owner_glb"
            },
            factionExpulsion = {
            },
            worldwide = {
                -- mechanisms
                "Sky_qRe_KWMG6_PenumbraState", "Sky_qRe_KWTG6_Button_glb",
                -- objects
                "Sky_qRe_DSW1_LetterState_glb",
                -- npc behavior
                "Sky_qRe_HA1_CultistState_glb", "Sky_qRe_HA3_Sick_glb", "Sky_qRe_KWMG4_Returned_glb", "Sky_qRe_VF1_Returned_Glb",
                -- arena State
                "Sky_qRe_DSE_ArenaFight_glb"
            },
            unknown = {
            }
        }
    }

    tableHelper.merge(clientVariableScopes, addedVariableScopes, true)
end

return clientVariableScopes