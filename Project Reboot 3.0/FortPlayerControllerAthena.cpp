#include "FortPlayerControllerAthena.h"
#include "FortPlayerPawn.h"
#include "FortKismetLibrary.h"

#include "SoftObjectPtr.h"
#include "globals.h"
#include "GameplayStatics.h"

void AFortPlayerControllerAthena::ServerGiveCreativeItemHook(AFortPlayerControllerAthena* Controller, FFortItemEntry CreativeItem)
{
	// Don't worry, the validate has a check if it is a creative enabled mode or not, but we need to add a volume check.

	auto CreativeItemPtr = &CreativeItem;
	auto ItemDefinition = CreativeItemPtr->GetItemDefinition();

	if (!ItemDefinition)
		return;

	bool bShouldUpdate = false;
	auto LoadedAmmo = -1; // CreativeItemPtr->GetLoadedAmmo()
	Controller->GetWorldInventory()->AddItem(ItemDefinition, &bShouldUpdate, CreativeItemPtr->GetCount(), LoadedAmmo, false);

	if (bShouldUpdate)
		Controller->GetWorldInventory()->Update(Controller);
}

void AFortPlayerControllerAthena::ServerTeleportToPlaygroundLobbyIslandHook(AFortPlayerControllerAthena* Controller)
{
	auto Pawn = Controller->GetMyFortPawn();

	if (!Pawn)
		return;

	static auto FortPlayerStartCreativeClass = FindObject<UClass>("/Script/FortniteGame.FortPlayerStartCreative");
	auto AllCreativePlayerStarts = UGameplayStatics::GetAllActorsOfClass(GetWorld(), FortPlayerStartCreativeClass);

	for (int i = 0; i < AllCreativePlayerStarts.Num(); i++)
	{
		auto CurrentPlayerStart = AllCreativePlayerStarts.at(i);

		static auto PlayerStartTagsOffset = CurrentPlayerStart->GetOffset("PlayerStartTags");
		auto bHasSpawnTag = CurrentPlayerStart->Get<FGameplayTagContainer>(PlayerStartTagsOffset).Contains("Playground.LobbyIsland.Spawn");

		if (!bHasSpawnTag)
			continue;

		Pawn->TeleportTo(CurrentPlayerStart->GetActorLocation(), Pawn->GetActorRotation());
		break;
	}

	AllCreativePlayerStarts.Free();
}

void AFortPlayerControllerAthena::ServerAcknowledgePossessionHook(APlayerController* Controller, APawn* Pawn)
{
	static auto AcknowledgedPawnOffset = Controller->GetOffset("AcknowledgedPawn");
	Controller->Get<APawn*>(AcknowledgedPawnOffset) = Pawn;

	if (Globals::bNoMCP)
		return;

	auto ControllerAsFort = Cast<AFortPlayerController>(Controller);
	auto PawnAsFort = Cast<AFortPlayerPawn>(Pawn);
	auto PlayerStateAsFort = Cast<AFortPlayerState>(Pawn->GetPlayerState());

	if (!PawnAsFort)
		return;

	static auto UpdatePlayerCustomCharacterPartsVisualizationFn = FindObject<UFunction>("/Script/FortniteGame.FortKismetLibrary.UpdatePlayerCustomCharacterPartsVisualization");

	if (!UpdatePlayerCustomCharacterPartsVisualizationFn)
	{
		auto CosmeticLoadout = ControllerAsFort->GetCosmeticLoadout();

		ApplyCID(PawnAsFort, CosmeticLoadout->GetCharacter());

		return;
	}

	if (!PlayerStateAsFort)
		return;
	
	UFortKismetLibrary::StaticClass()->ProcessEvent(UpdatePlayerCustomCharacterPartsVisualizationFn, &PlayerStateAsFort);
}

void AFortPlayerControllerAthena::ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PlayerController, __int64 ChatEntry, __int64 SenderID)
{
	using UAthenaEmojiItemDefinition = UFortItemDefinition;

	static auto EmojiComm = FindObject<UAthenaEmojiItemDefinition>("/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");
	PlayerController->ServerPlayEmoteItemHook(PlayerController, EmojiComm);
}

void AFortPlayerControllerAthena::GetPlayerViewPointHook(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation)
{
	if (auto MyFortPawn = PlayerController->GetMyFortPawn())
	{
		Location = MyFortPawn->GetActorLocation();
		Rotation = PlayerController->GetControlRotation();
		return;
	}

	return AFortPlayerControllerAthena::GetPlayerViewPointOriginal(PlayerController, Location, Rotation);
}