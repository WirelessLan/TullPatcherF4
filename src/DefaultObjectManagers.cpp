#include "DefaultObjectManagers.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace DefaultObjectManagers {
	constexpr std::string_view TypeName = "DefaultObjectManager";

	enum class FilterType {
		kFormID
	};

	std::string_view FilterTypeToString(FilterType a_value) {
		switch (a_value) {
		case FilterType::kFormID: return "FilterByFormID";
		default: return std::string_view{};
		}
	}

	enum class ElementType {
		kObjects
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kObjects: return "Objects";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kSet
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kSet: return "Set";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct ObjectData {
				std::string Use;
				std::string ObjectID;
			};

			OperationType OpType;
			std::optional<ObjectData> OpData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct ObjectData {
			std::unordered_map<RE::DEFAULT_OBJECT, RE::TESForm*> SetObjectMap;
		};

		std::optional<ObjectData> Objects;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSDefaultObjectManager*, PatchData> g_patchMap;

	std::unordered_map<std::string, RE::DEFAULT_OBJECT> g_defaultObjectsMap = {
		{ "SittingAngleLimit", RE::DEFAULT_OBJECT::kSittingAngleLimit },
		{ "AllowPlayerShout", RE::DEFAULT_OBJECT::kAllowPlayerShout },
		{ "Gold", RE::DEFAULT_OBJECT::kGold },
		{ "Lockpick", RE::DEFAULT_OBJECT::kLockpick },
		{ "SkeletonKey", RE::DEFAULT_OBJECT::kSkeletonKey },
		{ "PlayerFaction", RE::DEFAULT_OBJECT::kPlayerFaction },
		{ "GuardFaction", RE::DEFAULT_OBJECT::kGuardFaction },
		{ "BattleMusic", RE::DEFAULT_OBJECT::kBattleMusic },
		{ "DeathMusic", RE::DEFAULT_OBJECT::kDeathMusic },
		{ "DungeonClearedMusic", RE::DEFAULT_OBJECT::kDungeonClearedMusic },
		{ "PlayerVoiceMale", RE::DEFAULT_OBJECT::kPlayerVoiceMale },
		{ "PlayerVoiceMaleChild", RE::DEFAULT_OBJECT::kPlayerVoiceMaleChild },
		{ "PlayerVoiceFemale", RE::DEFAULT_OBJECT::kPlayerVoiceFemale },
		{ "PlayerVoiceFemaleChild", RE::DEFAULT_OBJECT::kPlayerVoiceFemaleChild },
		{ "EatPackageDefaultFood", RE::DEFAULT_OBJECT::kEatPackageDefaultFood },
		{ "VoiceEquip", RE::DEFAULT_OBJECT::kVoiceEquip },
		{ "PotionEquip", RE::DEFAULT_OBJECT::kPotionEquip },
		{ "EveryActorAbility", RE::DEFAULT_OBJECT::kEveryActorAbility },
		{ "CommandedActorAbility", RE::DEFAULT_OBJECT::kCommandedActorAbility },
		{ "DrugWearsOffImageSpace", RE::DEFAULT_OBJECT::kDrugWearsOffImageSpace },
		{ "FootstepSet", RE::DEFAULT_OBJECT::kFootstepSet },
		{ "LandscapeMaterial", RE::DEFAULT_OBJECT::kLandscapeMaterial },
		{ "DragonLandZoneMarker", RE::DEFAULT_OBJECT::kDragonLandZoneMarker },
		{ "DragonCrashZoneMarker", RE::DEFAULT_OBJECT::kDragonCrashZoneMarker },
		{ "CombatStyle", RE::DEFAULT_OBJECT::kCombatStyle },
		{ "DefaultPackList", RE::DEFAULT_OBJECT::kDefaultPackList },
		{ "WaitForDialoguePackage", RE::DEFAULT_OBJECT::kWaitForDialoguePackage },
		{ "VirtualLocation", RE::DEFAULT_OBJECT::kVirtualLocation },
		{ "PersistAllLocation", RE::DEFAULT_OBJECT::kPersistAllLocation },
		{ "PathingTestNPC", RE::DEFAULT_OBJECT::kPathingTestNPC },
		{ "ActionSwimStateChange", RE::DEFAULT_OBJECT::kActionSwimStateChange },
		{ "ActionLook", RE::DEFAULT_OBJECT::kActionLook },
		{ "ActionLeftAttack", RE::DEFAULT_OBJECT::kActionLeftAttack },
		{ "ActionLeftReady", RE::DEFAULT_OBJECT::kActionLeftReady },
		{ "ActionLeftRelease", RE::DEFAULT_OBJECT::kActionLeftRelease },
		{ "ActionLeftInterrupt", RE::DEFAULT_OBJECT::kActionLeftInterrupt },
		{ "ActionRightAttack", RE::DEFAULT_OBJECT::kActionRightAttack },
		{ "ActionRightReady", RE::DEFAULT_OBJECT::kActionRightReady },
		{ "ActionRightRelease", RE::DEFAULT_OBJECT::kActionRightRelease },
		{ "ActionRightInterrupt", RE::DEFAULT_OBJECT::kActionRightInterrupt },
		{ "ActionDualAttack", RE::DEFAULT_OBJECT::kActionDualAttack },
		{ "ActionDualRelease", RE::DEFAULT_OBJECT::kActionDualRelease },
		{ "ActionActivate", RE::DEFAULT_OBJECT::kActionActivate },
		{ "ActionJump", RE::DEFAULT_OBJECT::kActionJump },
		{ "ActionFall", RE::DEFAULT_OBJECT::kActionFall },
		{ "ActionLand", RE::DEFAULT_OBJECT::kActionLand },
		{ "ActionMantle", RE::DEFAULT_OBJECT::kActionMantle },
		{ "ActionSneak", RE::DEFAULT_OBJECT::kActionSneak },
		{ "ActionVoice", RE::DEFAULT_OBJECT::kActionVoice },
		{ "ActionVoiceReady", RE::DEFAULT_OBJECT::kActionVoiceReady },
		{ "ActionVoiceRelease", RE::DEFAULT_OBJECT::kActionVoiceRelease },
		{ "ActionVoiceInterrupt", RE::DEFAULT_OBJECT::kActionVoiceInterrupt },
		{ "ActionIdle", RE::DEFAULT_OBJECT::kActionIdle },
		{ "ActionSprintStart", RE::DEFAULT_OBJECT::kActionSprintStart },
		{ "ActionSprintStop", RE::DEFAULT_OBJECT::kActionSprintStop },
		{ "ActionDraw", RE::DEFAULT_OBJECT::kActionDraw },
		{ "ActionSheath", RE::DEFAULT_OBJECT::kActionSheath },
		{ "ActionLeftPowerAttack", RE::DEFAULT_OBJECT::kActionLeftPowerAttack },
		{ "ActionRightPowerAttack", RE::DEFAULT_OBJECT::kActionRightPowerAttack },
		{ "ActionDualPowerAttack", RE::DEFAULT_OBJECT::kActionDualPowerAttack },
		{ "ActionLeftSyncAttack", RE::DEFAULT_OBJECT::kActionLeftSyncAttack },
		{ "ActionRightSyncAttack", RE::DEFAULT_OBJECT::kActionRightSyncAttack },
		{ "ActionStaggerStart", RE::DEFAULT_OBJECT::kActionStaggerStart },
		{ "ActionBlockHit", RE::DEFAULT_OBJECT::kActionBlockHit },
		{ "ActionBlockAnticipate", RE::DEFAULT_OBJECT::kActionBlockAnticipate },
		{ "ActionRecoil", RE::DEFAULT_OBJECT::kActionRecoil },
		{ "ActionLargeRecoil", RE::DEFAULT_OBJECT::kActionLargeRecoil },
		{ "ActionBleedoutStart", RE::DEFAULT_OBJECT::kActionBleedoutStart },
		{ "ActionBleedoutStop", RE::DEFAULT_OBJECT::kActionBleedoutStop },
		{ "ActionIdleStop", RE::DEFAULT_OBJECT::kActionIdleStop },
		{ "ActionWardHit", RE::DEFAULT_OBJECT::kActionWardHit },
		{ "ActionForceEquip", RE::DEFAULT_OBJECT::kActionForceEquip },
		{ "ActionShieldChange", RE::DEFAULT_OBJECT::kActionShieldChange },
		{ "ActionPathStart", RE::DEFAULT_OBJECT::kActionPathStart },
		{ "ActionPathEnd", RE::DEFAULT_OBJECT::kActionPathEnd },
		{ "ActionLargeMovementDelta", RE::DEFAULT_OBJECT::kActionLargeMovementDelta },
		{ "ActionFlyStart", RE::DEFAULT_OBJECT::kActionFlyStart },
		{ "ActionFlyStop", RE::DEFAULT_OBJECT::kActionFlyStop },
		{ "ActionHoverStart", RE::DEFAULT_OBJECT::kActionHoverStart },
		{ "ActionHoverStop", RE::DEFAULT_OBJECT::kActionHoverStop },
		{ "ActionBumpedInto", RE::DEFAULT_OBJECT::kActionBumpedInto },
		{ "ActionSummonedStart", RE::DEFAULT_OBJECT::kActionSummonedStart },
		{ "ActionDialogueTalking", RE::DEFAULT_OBJECT::kActionDialogueTalking },
		{ "ActionDialogueListen", RE::DEFAULT_OBJECT::kActionDialogueListen },
		{ "ActionDialogueListenPositive", RE::DEFAULT_OBJECT::kActionDialogueListenPositive },
		{ "ActionDialogueListenNegative", RE::DEFAULT_OBJECT::kActionDialogueListenNegative },
		{ "ActionDialogueListenQuestion", RE::DEFAULT_OBJECT::kActionDialogueListenQuestion },
		{ "ActionDialogueListenNeutral", RE::DEFAULT_OBJECT::kActionDialogueListenNeutral },
		{ "ActionDialogueEnter", RE::DEFAULT_OBJECT::kActionDialogueEnter },
		{ "ActionDialogueExit", RE::DEFAULT_OBJECT::kActionDialogueExit },
		{ "ActionDeath", RE::DEFAULT_OBJECT::kActionDeath },
		{ "ActionDeathWait", RE::DEFAULT_OBJECT::kActionDeathWait },
		{ "ActionIdleWarn", RE::DEFAULT_OBJECT::kActionIdleWarn },
		{ "ActionMoveStart", RE::DEFAULT_OBJECT::kActionMoveStart },
		{ "ActionMoveStop", RE::DEFAULT_OBJECT::kActionMoveStop },
		{ "ActionTurnRight", RE::DEFAULT_OBJECT::kActionTurnRight },
		{ "ActionTurnLeft", RE::DEFAULT_OBJECT::kActionTurnLeft },
		{ "ActionTurnStop", RE::DEFAULT_OBJECT::kActionTurnStop },
		{ "ActionMoveForward", RE::DEFAULT_OBJECT::kActionMoveForward },
		{ "ActionMoveBackward", RE::DEFAULT_OBJECT::kActionMoveBackward },
		{ "ActionMoveLeft", RE::DEFAULT_OBJECT::kActionMoveLeft },
		{ "ActionMoveRight", RE::DEFAULT_OBJECT::kActionMoveRight },
		{ "ActionKnockdown", RE::DEFAULT_OBJECT::kActionKnockdown },
		{ "ActionGetUp", RE::DEFAULT_OBJECT::kActionGetUp },
		{ "ActionIdleStopInstant", RE::DEFAULT_OBJECT::kActionIdleStopInstant },
		{ "ActionRagdollInstant", RE::DEFAULT_OBJECT::kActionRagdollInstant },
		{ "ActionWaterwalkStart", RE::DEFAULT_OBJECT::kActionWaterwalkStart },
		{ "ActionReload", RE::DEFAULT_OBJECT::kActionReload },
		{ "ActionBoltCharge", RE::DEFAULT_OBJECT::kActionBoltCharge },
		{ "ActionSighted", RE::DEFAULT_OBJECT::kActionSighted },
		{ "ActionSightedRelease", RE::DEFAULT_OBJECT::kActionSightedRelease },
		{ "ActionMelee", RE::DEFAULT_OBJECT::kActionMelee },
		{ "ActionFireSingle", RE::DEFAULT_OBJECT::kActionFireSingle },
		{ "ActionFireCharge", RE::DEFAULT_OBJECT::kActionFireCharge },
		{ "ActionFireChargeHold", RE::DEFAULT_OBJECT::kActionFireChargeHold },
		{ "ActionFireAuto", RE::DEFAULT_OBJECT::kActionFireAuto },
		{ "ActionFireEmpty", RE::DEFAULT_OBJECT::kActionFireEmpty },
		{ "ActionThrow", RE::DEFAULT_OBJECT::kActionThrow },
		{ "ActionEnterCover", RE::DEFAULT_OBJECT::kActionEnterCover },
		{ "ActionExitCover", RE::DEFAULT_OBJECT::kActionExitCover },
		{ "ActionCoverSprintStart", RE::DEFAULT_OBJECT::kActionCoverSprintStart },
		{ "ActionShuffle", RE::DEFAULT_OBJECT::kActionShuffle },
		{ "ActionPipboy", RE::DEFAULT_OBJECT::kActionPipboy },
		{ "ActionPipboyClose", RE::DEFAULT_OBJECT::kActionPipboyClose },
		{ "ActionPipboyZoom", RE::DEFAULT_OBJECT::kActionPipboyZoom },
		{ "ActionPipboyStats", RE::DEFAULT_OBJECT::kActionPipboyStats },
		{ "ActionPipboyInventory", RE::DEFAULT_OBJECT::kActionPipboyInventory },
		{ "ActionPipboyData", RE::DEFAULT_OBJECT::kActionPipboyData },
		{ "ActionPipboyMap", RE::DEFAULT_OBJECT::kActionPipboyMap },
		{ "ActionPipboyTab", RE::DEFAULT_OBJECT::kActionPipboyTab },
		{ "ActionPipboyTabPrevious", RE::DEFAULT_OBJECT::kActionPipboyTabPrevious },
		{ "ActionPipboySelect", RE::DEFAULT_OBJECT::kActionPipboySelect },
		{ "ActionPipboyRadioOn", RE::DEFAULT_OBJECT::kActionPipboyRadioOn },
		{ "ActionPipboyRadioOff", RE::DEFAULT_OBJECT::kActionPipboyRadioOff },
		{ "ActionPipboyLoadHolotape", RE::DEFAULT_OBJECT::kActionPipboyLoadHolotape },
		{ "ActionPipboyInspect", RE::DEFAULT_OBJECT::kActionPipboyInspect },
		{ "ActionNonSupportContact", RE::DEFAULT_OBJECT::kActionNonSupportContact },
		{ "ActionInteractionEnter", RE::DEFAULT_OBJECT::kActionInteractionEnter },
		{ "ActionInteractionExit", RE::DEFAULT_OBJECT::kActionInteractionExit },
		{ "ActionInteractionExitAlternate", RE::DEFAULT_OBJECT::kActionInteractionExitAlternate },
		{ "ActionInteractionExitQuick", RE::DEFAULT_OBJECT::kActionInteractionExitQuick },
		{ "ActionIntimidate", RE::DEFAULT_OBJECT::kActionIntimidate },
		{ "ActionGunChargeStart", RE::DEFAULT_OBJECT::kActionGunChargeStart },
		{ "ActionGunDown", RE::DEFAULT_OBJECT::kActionGunDown },
		{ "ActionGunRelaxed", RE::DEFAULT_OBJECT::kActionGunRelaxed },
		{ "ActionGunAlert", RE::DEFAULT_OBJECT::kActionGunAlert },
		{ "ActionGunReady", RE::DEFAULT_OBJECT::kActionGunReady },
		{ "ActionFlipThrow", RE::DEFAULT_OBJECT::kActionFlipThrow },
		{ "ActionEnterCombat", RE::DEFAULT_OBJECT::kActionEnterCombat },
		{ "ActionExitCombat", RE::DEFAULT_OBJECT::kActionExitCombat },
		{ "ActionLimbCritical", RE::DEFAULT_OBJECT::kActionLimbCritical },
		{ "ActionEvade", RE::DEFAULT_OBJECT::kActionEvade },
		{ "ActionDodge", RE::DEFAULT_OBJECT::kActionDodge },
		{ "ActionAoEAttack", RE::DEFAULT_OBJECT::kActionAoEAttack },
		{ "ActionPanic", RE::DEFAULT_OBJECT::kActionPanic },
		{ "ActionCower", RE::DEFAULT_OBJECT::kActionCower },
		{ "ActionTunnel", RE::DEFAULT_OBJECT::kActionTunnel },
		{ "ActionHide", RE::DEFAULT_OBJECT::kActionHide },
		{ "PickupSoundGeneric", RE::DEFAULT_OBJECT::kPickupSoundGeneric },
		{ "PutdownSoundGeneric", RE::DEFAULT_OBJECT::kPutdownSoundGeneric },
		{ "PickupSoundWeapon", RE::DEFAULT_OBJECT::kPickupSoundWeapon },
		{ "PutdownSoundWeapon", RE::DEFAULT_OBJECT::kPutdownSoundWeapon },
		{ "PickupSoundArmor", RE::DEFAULT_OBJECT::kPickupSoundArmor },
		{ "PutdownSoundArmor", RE::DEFAULT_OBJECT::kPutdownSoundArmor },
		{ "PickupSoundBook", RE::DEFAULT_OBJECT::kPickupSoundBook },
		{ "PutdownSoundBook", RE::DEFAULT_OBJECT::kPutdownSoundBook },
		{ "PickupSoundIngredient", RE::DEFAULT_OBJECT::kPickupSoundIngredient },
		{ "PutdownSoundIngredient", RE::DEFAULT_OBJECT::kPutdownSoundIngredient },
		{ "HarvestSound", RE::DEFAULT_OBJECT::kHarvestSound },
		{ "HarvestFailedSound", RE::DEFAULT_OBJECT::kHarvestFailedSound },
		{ "WardBreakSound", RE::DEFAULT_OBJECT::kWardBreakSound },
		{ "WardAbsorbSound", RE::DEFAULT_OBJECT::kWardAbsorbSound },
		{ "WardDeflectSound", RE::DEFAULT_OBJECT::kWardDeflectSound },
		{ "MagicFailSound", RE::DEFAULT_OBJECT::kMagicFailSound },
		{ "ShoutFailSound", RE::DEFAULT_OBJECT::kShoutFailSound },
		{ "HeartbeatSoundFast", RE::DEFAULT_OBJECT::kHeartbeatSoundFast },
		{ "HeartbeatSoundSlow", RE::DEFAULT_OBJECT::kHeartbeatSoundSlow },
		{ "ImagespaceLowHealth", RE::DEFAULT_OBJECT::kImagespaceLowHealth },
		{ "SoulCapturedSound", RE::DEFAULT_OBJECT::kSoulCapturedSound },
		{ "NoActivationSound", RE::DEFAULT_OBJECT::kNoActivationSound },
		{ "MapMenuLoopingSound", RE::DEFAULT_OBJECT::kMapMenuLoopingSound },
		{ "DialogueVoiceCategory", RE::DEFAULT_OBJECT::kDialogueVoiceCategory },
		{ "NonDialogueVoiceCategory", RE::DEFAULT_OBJECT::kNonDialogueVoiceCategory },
		{ "SFXToFadeInDialogueCategory", RE::DEFAULT_OBJECT::kSFXToFadeInDialogueCategory },
		{ "PauseDuringMenuCategoryFade", RE::DEFAULT_OBJECT::kPauseDuringMenuCategoryFade },
		{ "PauseDuringMenuCategoryImmediate", RE::DEFAULT_OBJECT::kPauseDuringMenuCategoryImmediate },
		{ "PauseDuringLoadingMenuCategory", RE::DEFAULT_OBJECT::kPauseDuringLoadingMenuCategory },
		{ "MusicSoundCategory", RE::DEFAULT_OBJECT::kMusicSoundCategory },
		{ "StatsMuteCategory", RE::DEFAULT_OBJECT::kStatsMuteCategory },
		{ "StatsMusic", RE::DEFAULT_OBJECT::kStatsMusic },
		{ "MasterSoundCategory", RE::DEFAULT_OBJECT::kMasterSoundCategory },
		{ "DialogueOutputModel3D", RE::DEFAULT_OBJECT::kDialogueOutputModel3D },
		{ "DialogueOutputModel2D", RE::DEFAULT_OBJECT::kDialogueOutputModel2D },
		{ "PlayersOutputModel1stPerson", RE::DEFAULT_OBJECT::kPlayersOutputModel1stPerson },
		{ "PlayersOutputModel3rdPerson", RE::DEFAULT_OBJECT::kPlayersOutputModel3rdPerson },
		{ "InterfaceOutputModel", RE::DEFAULT_OBJECT::kInterfaceOutputModel },
		{ "ReverbType", RE::DEFAULT_OBJECT::kReverbType },
		{ "UnderwaterLoopSound", RE::DEFAULT_OBJECT::kUnderwaterLoopSound },
		{ "UnderwaterReverbType", RE::DEFAULT_OBJECT::kUnderwaterReverbType },
		{ "KeywordHorse", RE::DEFAULT_OBJECT::kKeywordHorse },
		{ "KeywordUndead", RE::DEFAULT_OBJECT::kKeywordUndead },
		{ "KeywordNPC", RE::DEFAULT_OBJECT::kKeywordNPC },
		{ "KeywordDummyObject", RE::DEFAULT_OBJECT::kKeywordDummyObject },
		{ "KeywordUseGeometryEmitter", RE::DEFAULT_OBJECT::kKeywordUseGeometryEmitter },
		{ "KeywordMustStop", RE::DEFAULT_OBJECT::kKeywordMustStop },
		{ "MaleFaceTextureSetHead", RE::DEFAULT_OBJECT::kMaleFaceTextureSetHead },
		{ "MaleFaceTextureSetMouth", RE::DEFAULT_OBJECT::kMaleFaceTextureSetMouth },
		{ "MaleFaceTextureSetEyes", RE::DEFAULT_OBJECT::kMaleFaceTextureSetEyes },
		{ "FemaleFaceTextureSetHead", RE::DEFAULT_OBJECT::kFemaleFaceTextureSetHead },
		{ "FemaleFaceTextureSetMouth", RE::DEFAULT_OBJECT::kFemaleFaceTextureSetMouth },
		{ "FemaleFaceTextureSetEyes", RE::DEFAULT_OBJECT::kFemaleFaceTextureSetEyes },
		{ "ImageSpaceModifierForInventoryMenu", RE::DEFAULT_OBJECT::kImageSpaceModifierForInventoryMenu },
		{ "ImageSpaceModifierForPipboyMenuInPowerArmor", RE::DEFAULT_OBJECT::kImageSpaceModifierForPipboyMenuInPowerArmor },
		{ "PackageTemplate", RE::DEFAULT_OBJECT::kPackageTemplate },
		{ "MainMenuCell", RE::DEFAULT_OBJECT::kMainMenuCell },
		{ "DefaultMovementTypeDefault", RE::DEFAULT_OBJECT::kDefaultMovementTypeDefault },
		{ "DefaultMovementTypeSwim", RE::DEFAULT_OBJECT::kDefaultMovementTypeSwim },
		{ "DefaultMovementTypeFly", RE::DEFAULT_OBJECT::kDefaultMovementTypeFly },
		{ "DefaultMovementTypeSneak", RE::DEFAULT_OBJECT::kDefaultMovementTypeSneak },
		{ "KeywordSpecialFurniture", RE::DEFAULT_OBJECT::kKeywordSpecialFurniture },
		{ "KeywordFurnitureForces1stPerson", RE::DEFAULT_OBJECT::kKeywordFurnitureForces1stPerson },
		{ "KeywordFurnitureForces3rdPerson", RE::DEFAULT_OBJECT::kKeywordFurnitureForces3rdPerson },
		{ "KeywordActivatorFurnitureNoPlayer", RE::DEFAULT_OBJECT::kKeywordActivatorFurnitureNoPlayer },
		{ "TelekinesisGrabSound", RE::DEFAULT_OBJECT::kTelekinesisGrabSound },
		{ "TelekinesisThrowSound", RE::DEFAULT_OBJECT::kTelekinesisThrowSound },
		{ "WorldMapWeather", RE::DEFAULT_OBJECT::kWorldMapWeather },
		{ "HelpManualPC", RE::DEFAULT_OBJECT::kHelpManualPC },
		{ "HelpManualXBox", RE::DEFAULT_OBJECT::kHelpManualXBox },
		{ "KeywordTypeAmmo", RE::DEFAULT_OBJECT::kKeywordTypeAmmo },
		{ "KeywordTypeArmor", RE::DEFAULT_OBJECT::kKeywordTypeArmor },
		{ "KeywordTypeBook", RE::DEFAULT_OBJECT::kKeywordTypeBook },
		{ "KeywordTypeIngredient", RE::DEFAULT_OBJECT::kKeywordTypeIngredient },
		{ "KeywordTypeKey", RE::DEFAULT_OBJECT::kKeywordTypeKey },
		{ "KeywordTypeMisc", RE::DEFAULT_OBJECT::kKeywordTypeMisc },
		{ "KeywordTypeSoulGem", RE::DEFAULT_OBJECT::kKeywordTypeSoulGem },
		{ "KeywordTypeWeapon", RE::DEFAULT_OBJECT::kKeywordTypeWeapon },
		{ "KeywordTypePotion", RE::DEFAULT_OBJECT::kKeywordTypePotion },
		{ "BaseWeaponEnchantment", RE::DEFAULT_OBJECT::kBaseWeaponEnchantment },
		{ "BaseArmorEnchantment", RE::DEFAULT_OBJECT::kBaseArmorEnchantment },
		{ "BasePotion", RE::DEFAULT_OBJECT::kBasePotion },
		{ "BasePoison", RE::DEFAULT_OBJECT::kBasePoison },
		{ "KeywordDragon", RE::DEFAULT_OBJECT::kKeywordDragon },
		{ "KeywordMovable", RE::DEFAULT_OBJECT::kKeywordMovable },
		{ "ArtObjectAbsorbEffect", RE::DEFAULT_OBJECT::kArtObjectAbsorbEffect },
		{ "WeaponMaterialList", RE::DEFAULT_OBJECT::kWeaponMaterialList },
		{ "ArmorMaterialList", RE::DEFAULT_OBJECT::kArmorMaterialList },
		{ "KeywordDisallowEnchanting", RE::DEFAULT_OBJECT::kKeywordDisallowEnchanting },
		{ "Favortravelmarkerlocation", RE::DEFAULT_OBJECT::kFavortravelmarkerlocation },
		{ "TeammateReadyWeapon", RE::DEFAULT_OBJECT::kTeammateReadyWeapon },
		{ "KeywordHoldLocation", RE::DEFAULT_OBJECT::kKeywordHoldLocation },
		{ "KeywordCivilWarOwner", RE::DEFAULT_OBJECT::kKeywordCivilWarOwner },
		{ "KeywordCivilWarNeutral", RE::DEFAULT_OBJECT::kKeywordCivilWarNeutral },
		{ "LocRefTypeCivilWarSoldier", RE::DEFAULT_OBJECT::kLocRefTypeCivilWarSoldier },
		{ "KeywordClearableLocation", RE::DEFAULT_OBJECT::kKeywordClearableLocation },
		{ "LocRefTypeResourceDestructible", RE::DEFAULT_OBJECT::kLocRefTypeResourceDestructible },
		{ "FormListHairColorList", RE::DEFAULT_OBJECT::kFormListHairColorList },
		{ "ComplexSceneObject", RE::DEFAULT_OBJECT::kComplexSceneObject },
		{ "KeywordReusableSoulGem", RE::DEFAULT_OBJECT::kKeywordReusableSoulGem },
		{ "KeywordAnimal", RE::DEFAULT_OBJECT::kKeywordAnimal },
		{ "KeywordDaedra", RE::DEFAULT_OBJECT::kKeywordDaedra },
		{ "KeywordRobot", RE::DEFAULT_OBJECT::kKeywordRobot },
		{ "KeywordNirnroot", RE::DEFAULT_OBJECT::kKeywordNirnroot },
		{ "FightersGuildFaction", RE::DEFAULT_OBJECT::kFightersGuildFaction },
		{ "MagesGuildFaction", RE::DEFAULT_OBJECT::kMagesGuildFaction },
		{ "ThievesGuildFaction", RE::DEFAULT_OBJECT::kThievesGuildFaction },
		{ "DarkBrotherhoodFaction", RE::DEFAULT_OBJECT::kDarkBrotherhoodFaction },
		{ "JarlFaction", RE::DEFAULT_OBJECT::kJarlFaction },
		{ "BunnyFaction", RE::DEFAULT_OBJECT::kBunnyFaction },
		{ "PlayerIsVampireVariable", RE::DEFAULT_OBJECT::kPlayerIsVampireVariable },
		{ "RoadMarker", RE::DEFAULT_OBJECT::kRoadMarker },
		{ "KeywordScaleActorTo10", RE::DEFAULT_OBJECT::kKeywordScaleActorTo10 },
		{ "KeywordVampire", RE::DEFAULT_OBJECT::kKeywordVampire },
		{ "KeywordForge", RE::DEFAULT_OBJECT::kKeywordForge },
		{ "KeywordCookingPot", RE::DEFAULT_OBJECT::kKeywordCookingPot },
		{ "KeywordSmelter", RE::DEFAULT_OBJECT::kKeywordSmelter },
		{ "KeywordTanningRack", RE::DEFAULT_OBJECT::kKeywordTanningRack },
		{ "HelpBasicLockpickingPC", RE::DEFAULT_OBJECT::kHelpBasicLockpickingPC },
		{ "HelpBasicLockpickingConsole", RE::DEFAULT_OBJECT::kHelpBasicLockpickingConsole },
		{ "HelpBasicForging", RE::DEFAULT_OBJECT::kHelpBasicForging },
		{ "HelpBasicCooking", RE::DEFAULT_OBJECT::kHelpBasicCooking },
		{ "HelpBasicSmelting", RE::DEFAULT_OBJECT::kHelpBasicSmelting },
		{ "HelpBasicTanning", RE::DEFAULT_OBJECT::kHelpBasicTanning },
		{ "HelpBasicObjectCreation", RE::DEFAULT_OBJECT::kHelpBasicObjectCreation },
		{ "HelpBasicEnchanting", RE::DEFAULT_OBJECT::kHelpBasicEnchanting },
		{ "HelpBasicSmithingWeapon", RE::DEFAULT_OBJECT::kHelpBasicSmithingWeapon },
		{ "HelpBasicSmithingArmor", RE::DEFAULT_OBJECT::kHelpBasicSmithingArmor },
		{ "HelpBasicAlchemy", RE::DEFAULT_OBJECT::kHelpBasicAlchemy },
		{ "HelpBarter", RE::DEFAULT_OBJECT::kHelpBarter },
		{ "HelpLevelingup", RE::DEFAULT_OBJECT::kHelpLevelingup },
		{ "HelpSkillsMenu", RE::DEFAULT_OBJECT::kHelpSkillsMenu },
		{ "HelpMapMenu", RE::DEFAULT_OBJECT::kHelpMapMenu },
		{ "HelpJournal", RE::DEFAULT_OBJECT::kHelpJournal },
		{ "HelpLowHealth", RE::DEFAULT_OBJECT::kHelpLowHealth },
		{ "HelpLowMagicka", RE::DEFAULT_OBJECT::kHelpLowMagicka },
		{ "HelpLowStamina", RE::DEFAULT_OBJECT::kHelpLowStamina },
		{ "HelpJail", RE::DEFAULT_OBJECT::kHelpJail },
		{ "HelpTeamateFavor", RE::DEFAULT_OBJECT::kHelpTeamateFavor },
		{ "HelpWeaponCharge", RE::DEFAULT_OBJECT::kHelpWeaponCharge },
		{ "HelpFavorites", RE::DEFAULT_OBJECT::kHelpFavorites },
		{ "KinectHelpFormList", RE::DEFAULT_OBJECT::kKinectHelpFormList },
		{ "ImagespaceLoadscreen", RE::DEFAULT_OBJECT::kImagespaceLoadscreen },
		{ "KeywordWeaponMaterialDaedric", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialDaedric },
		{ "KeywordWeaponMaterialDraugr", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialDraugr },
		{ "KeywordWeaponMaterialDraugrHoned", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialDraugrHoned },
		{ "KeywordWeaponMaterialDwarven", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialDwarven },
		{ "KeywordWeaponMaterialEbony", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialEbony },
		{ "KeywordWeaponMaterialElven", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialElven },
		{ "KeywordWeaponMaterialFalmer", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialFalmer },
		{ "KeywordWeaponMaterialFalmerHoned", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialFalmerHoned },
		{ "KeywordWeaponMaterialGlass", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialGlass },
		{ "KeywordWeaponMaterialImperial", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialImperial },
		{ "KeywordWeaponMaterialIron", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialIron },
		{ "KeywordWeaponMaterialOrcish", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialOrcish },
		{ "KeywordWeaponMaterialSteel", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialSteel },
		{ "KeywordWeaponMaterialWood", RE::DEFAULT_OBJECT::kKeywordWeaponMaterialWood },
		{ "KeywordWeaponTypeBoundArrow", RE::DEFAULT_OBJECT::kKeywordWeaponTypeBoundArrow },
		{ "KeywordArmorMaterialDaedric", RE::DEFAULT_OBJECT::kKeywordArmorMaterialDaedric },
		{ "KeywordArmorMaterialDragonplate", RE::DEFAULT_OBJECT::kKeywordArmorMaterialDragonplate },
		{ "KeywordArmorMaterialDragonscale", RE::DEFAULT_OBJECT::kKeywordArmorMaterialDragonscale },
		{ "KeywordArmorMaterialDragonbone", RE::DEFAULT_OBJECT::kKeywordArmorMaterialDragonbone },
		{ "KeywordArmorMaterialDwarven", RE::DEFAULT_OBJECT::kKeywordArmorMaterialDwarven },
		{ "KeywordArmorMaterialEbony", RE::DEFAULT_OBJECT::kKeywordArmorMaterialEbony },
		{ "KeywordArmorMaterialElven", RE::DEFAULT_OBJECT::kKeywordArmorMaterialElven },
		{ "KeywordArmorMaterialElvenSplinted", RE::DEFAULT_OBJECT::kKeywordArmorMaterialElvenSplinted },
		{ "KeywordArmorMaterialFullLeather", RE::DEFAULT_OBJECT::kKeywordArmorMaterialFullLeather },
		{ "KeywordArmorMaterialGlass", RE::DEFAULT_OBJECT::kKeywordArmorMaterialGlass },
		{ "KeywordArmorMaterialHide", RE::DEFAULT_OBJECT::kKeywordArmorMaterialHide },
		{ "KeywordArmorMaterialImperial", RE::DEFAULT_OBJECT::kKeywordArmorMaterialImperial },
		{ "KeywordArmorMaterialImperialHeavy", RE::DEFAULT_OBJECT::kKeywordArmorMaterialImperialHeavy },
		{ "KeywordArmorMaterialImperialReinforced", RE::DEFAULT_OBJECT::kKeywordArmorMaterialImperialReinforced },
		{ "KeywordArmorMaterialIron", RE::DEFAULT_OBJECT::kKeywordArmorMaterialIron },
		{ "KeywordArmorMaterialIronBanded", RE::DEFAULT_OBJECT::kKeywordArmorMaterialIronBanded },
		{ "KeywordArmorMaterialOrcish", RE::DEFAULT_OBJECT::kKeywordArmorMaterialOrcish },
		{ "KeywordArmorMaterialScaled", RE::DEFAULT_OBJECT::kKeywordArmorMaterialScaled },
		{ "KeywordArmorMaterialSteel", RE::DEFAULT_OBJECT::kKeywordArmorMaterialSteel },
		{ "KeywordArmorMaterialSteelPlate", RE::DEFAULT_OBJECT::kKeywordArmorMaterialSteelPlate },
		{ "KeywordArmorMaterialStormcloak", RE::DEFAULT_OBJECT::kKeywordArmorMaterialStormcloak },
		{ "KeywordArmorMaterialStudded", RE::DEFAULT_OBJECT::kKeywordArmorMaterialStudded },
		{ "KeywordGenericCraftableKeyword01", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword01 },
		{ "KeywordGenericCraftableKeyword02", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword02 },
		{ "KeywordGenericCraftableKeyword03", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword03 },
		{ "KeywordGenericCraftableKeyword04", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword04 },
		{ "KeywordGenericCraftableKeyword05", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword05 },
		{ "KeywordGenericCraftableKeyword06", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword06 },
		{ "KeywordGenericCraftableKeyword07", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword07 },
		{ "KeywordGenericCraftableKeyword08", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword08 },
		{ "KeywordGenericCraftableKeyword09", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword09 },
		{ "KeywordGenericCraftableKeyword10", RE::DEFAULT_OBJECT::kKeywordGenericCraftableKeyword10 },
		{ "KeywordnullptrMOD", RE::DEFAULT_OBJECT::kKeywordnullptrMOD },
		{ "KeywordJewelry", RE::DEFAULT_OBJECT::kKeywordJewelry },
		{ "KeywordCuirass", RE::DEFAULT_OBJECT::kKeywordCuirass },
		{ "LocalMapHidePlane", RE::DEFAULT_OBJECT::kLocalMapHidePlane },
		{ "SnowLODMaterial", RE::DEFAULT_OBJECT::kSnowLODMaterial },
		{ "SnowLODMaterialHD", RE::DEFAULT_OBJECT::kSnowLODMaterialHD },
		{ "DialogueImagespace", RE::DEFAULT_OBJECT::kDialogueImagespace },
		{ "DialogueFollowerQuest", RE::DEFAULT_OBJECT::kDialogueFollowerQuest },
		{ "PotentialFollowerFaction", RE::DEFAULT_OBJECT::kPotentialFollowerFaction },
		{ "VampireAvailablePerks", RE::DEFAULT_OBJECT::kVampireAvailablePerks },
		{ "VampireRace", RE::DEFAULT_OBJECT::kVampireRace },
		{ "VampireSpells", RE::DEFAULT_OBJECT::kVampireSpells },
		{ "KeywordMount", RE::DEFAULT_OBJECT::kKeywordMount },
		{ "VerletCape", RE::DEFAULT_OBJECT::kVerletCape },
		{ "FurnitureTestNPC", RE::DEFAULT_OBJECT::kFurnitureTestNPC },
		{ "KeywordConditionalExplosion", RE::DEFAULT_OBJECT::kKeywordConditionalExplosion },
		{ "DefaultLight1", RE::DEFAULT_OBJECT::kDefaultLight1 },
		{ "DefaultLight2", RE::DEFAULT_OBJECT::kDefaultLight2 },
		{ "DefaultLight3", RE::DEFAULT_OBJECT::kDefaultLight3 },
		{ "DefaultLight4", RE::DEFAULT_OBJECT::kDefaultLight4 },
		{ "PipboyLight", RE::DEFAULT_OBJECT::kPipboyLight },
		{ "ActionBeginLoopingActivate", RE::DEFAULT_OBJECT::kActionBeginLoopingActivate },
		{ "ActionEndLoopingActivate", RE::DEFAULT_OBJECT::kActionEndLoopingActivate },
		{ "WorkshopPlayerOwnership", RE::DEFAULT_OBJECT::kWorkshopPlayerOwnership },
		{ "QuestMarkerFollower", RE::DEFAULT_OBJECT::kQuestMarkerFollower },
		{ "QuestMarkerLocation", RE::DEFAULT_OBJECT::kQuestMarkerLocation },
		{ "QuestMarkerEnemy", RE::DEFAULT_OBJECT::kQuestMarkerEnemy },
		{ "QuestMarkerEnemyAbove", RE::DEFAULT_OBJECT::kQuestMarkerEnemyAbove },
		{ "QuestMarkerEnemyBelow", RE::DEFAULT_OBJECT::kQuestMarkerEnemyBelow },
		{ "WorkshopMiscItemKeyword", RE::DEFAULT_OBJECT::kWorkshopMiscItemKeyword },
		{ "HeavyWeaponItemKeyword", RE::DEFAULT_OBJECT::kHeavyWeaponItemKeyword },
		{ "MineItemKeyword", RE::DEFAULT_OBJECT::kMineItemKeyword },
		{ "GrenadeItemKeyword", RE::DEFAULT_OBJECT::kGrenadeItemKeyword },
		{ "ChemItemKeyword", RE::DEFAULT_OBJECT::kChemItemKeyword },
		{ "AlcoholItemKeyword", RE::DEFAULT_OBJECT::kAlcoholItemKeyword },
		{ "FoodItemKeyword", RE::DEFAULT_OBJECT::kFoodItemKeyword },
		{ "RepairKitItemKeyword", RE::DEFAULT_OBJECT::kRepairKitItemKeyword },
		{ "MedbagitemKeyword", RE::DEFAULT_OBJECT::kMedbagitemKeyword },
		{ "GlovesitemKeyword", RE::DEFAULT_OBJECT::kGlovesitemKeyword },
		{ "Helmetitemkeyword", RE::DEFAULT_OBJECT::kHelmetitemkeyword },
		{ "Clothesitemkeyword", RE::DEFAULT_OBJECT::kClothesitemkeyword }
	};

	class DefaultObjectManagerParser : public Parsers::Parser<ConfigData> {
	public:
		DefaultObjectManagerParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

	protected:
		std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override {
			if (reader.EndOfFile() || reader.Peek().empty()) {
				return std::nullopt;
			}

			ConfigData configData{};

			if (!ParseFilter(configData)) {
				return std::nullopt;
			}

			auto token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!ParseElement(configData)) {
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!ParseOperation(configData)) {
				return std::nullopt;
			}

			while (true) {
				token = reader.Peek();
				if (token == ";") {
					reader.GetToken();
					break;
				}

				token = reader.GetToken();
				if (token != ".") {
					logger::warn("Line {}, Col {}: Syntax error. Expected '.' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				if (!ParseOperation(configData)) {
					return std::nullopt;
				}
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kObjects:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[ii].OpType),
						a_configData.Operations[ii].OpData->Use, a_configData.Operations[ii].OpData->ObjectID);

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;
			}
		}

		bool ParseFilter(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID") {
				a_configData.Filter = FilterType::kFormID;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			auto filterForm = ParseForm();
			if (!filterForm.has_value()) {
				return false;
			}

			a_configData.FilterForm = filterForm.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool ParseElement(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "Objects") {
				a_configData.Element = ElementType::kObjects;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			OperationType opType;
			ConfigData::Operation::ObjectData objData;

			auto token = reader.GetToken();
			if (token == "Set") {
				opType = OperationType::kSet;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			token = reader.GetToken();
			objData.Use = std::string(token);

			token = reader.GetToken();
			if (token != ",") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			token = reader.Peek();
			if (token == "null") {
				objData.ObjectID = reader.GetToken();
			}
			else {
				std::optional<std::string> form = ParseForm();
				if (!form.has_value()) {
					return false;
				}

				objData.ObjectID = form.value();
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back({ opType, objData });

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<DefaultObjectManagerParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSDefaultObjectManager* defObjManager = filterForm->As<RE::BGSDefaultObjectManager>();
			if (!defObjManager) {
				logger::warn("'{}' is not a DefaultObjectManager.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[defObjManager];

			if (a_configData.Element == ElementType::kObjects) {
				if (!patchData.Objects.has_value()) {
					patchData.Objects = PatchData::ObjectData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kSet) {
						auto it = g_defaultObjectsMap.find(op.OpData->Use);
						if (it == g_defaultObjectsMap.end()) {
							logger::warn("Invalid Object Use Name: '{}'.", op.OpData->Use);
							continue;
						}

						if (op.OpData->ObjectID == "null") {
							patchData.Objects->SetObjectMap.insert({ it->second, nullptr });
						}
						else {
							RE::TESForm* objForm = Utils::GetFormFromString(op.OpData->ObjectID);
							if (!objForm) {
								logger::warn("Invalid Form: '{}'.", op.OpData->ObjectID);
								continue;
							}

							patchData.Objects->SetObjectMap.insert({ it->second, objForm });
						}
					}
				}
			}
		}
	}

	void PatchObject(RE::BGSDefaultObjectManager* a_defObjManager, const PatchData::ObjectData& a_objData) {
		for (auto objPair : a_objData.SetObjectMap) {
			a_defObjManager->objectArray[RE::stl::to_underlying(objPair.first)] = objPair.second;
		}
	}

	void Patch(RE::BGSDefaultObjectManager* a_defObjManager, const PatchData& a_patchData) {
		if (a_patchData.Objects.has_value()) {
			PatchObject(a_defObjManager, a_patchData.Objects.value());
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& patchData : g_patchMap) {
			Patch(patchData.first, patchData.second);
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
