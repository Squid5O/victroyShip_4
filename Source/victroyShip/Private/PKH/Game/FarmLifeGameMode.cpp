// Fill out your copyright notice in the Description page of Project Settings.


#include "PKH/Game/FarmLifeGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "PKH/Http/HttpActor.h"
#include "PKH/Interface/DateUpdate.h"
#include "PKH/Interface/HourUpdate.h"
#include "PKH/NPC/NPCBase.h"
#include "PKH/UI/NPCConversationWidget.h"
#include "PKH/UI/TimerWidget.h"
#include "Serialization/EditorBulkData.h"

#define TEN_MINUTES 10
#define SIXTY_MINUTES 60
#define START_HOUR 8
#define END_HOUR 18
#define TIME_UPDATE_INTERVAL 5.0f

AFarmLifeGameMode::AFarmLifeGameMode()
{
	// Default Pawn & Controller


	// Http Actor
	static ConstructorHelpers::FClassFinder<AHttpActor> HttpActorClassRef(TEXT("/Game/PKH/Blueprint/BP_HttpActor.BP_HttpActor_C"));
	if(HttpActorClassRef.Class)
	{
		HttpActorClass = HttpActorClassRef.Class;
	}

	// UI
	static ConstructorHelpers::FClassFinder<UNPCConversationWidget> ConversationUIClassRef(TEXT("/Game/PKH/UI/WBP_NPCConversation.WBP_NPCConversation_C"));
	if (ConversationUIClassRef.Class)
	{
		ConversationUIClass = ConversationUIClassRef.Class;
	}
	static ConstructorHelpers::FClassFinder<UTimerWidget> TimerUIClassRef(TEXT("/Game/PKH/UI/WBP_Timer.WBP_Timer_C"));
	if (TimerUIClassRef.Class)
	{
		TimerUIClass = TimerUIClassRef.Class;
	}
}

void AFarmLifeGameMode::BeginPlay()
{
	Super::BeginPlay();

	if(HttpActorClass)
	{
		HttpActor = GetWorld()->SpawnActor<AHttpActor>(HttpActorClass);
	}

	StartTime();

	// UI
	TimerUI = CreateWidget<UTimerWidget>(GetWorld(), TimerUIClass);
	ensure(TimerUI);
	TimerUI->AddToViewport();
	TimerUI->UpdateTimerUI(Date, Hours, Minutes);

	ConversationUI = CreateWidget<UNPCConversationWidget>(GetWorld(), ConversationUIClass);
	ensure(ConversationUI);
	ConversationUI->AddToViewport();
	ConversationUI->SetVisibility(ESlateVisibility::Hidden);
}

#pragma region NPC conversation
void AFarmLifeGameMode::SendSpeech(const FString& FileName, const FString& FilePath, const TObjectPtr<ANPCBase>& NewNPC)
{
	CurNPC = NewNPC;
	SpeechFilePath = FilePath;

	HttpActor->SendSpeech(FileName, FilePath, CurNPC->GetNPCName());
	ConversationUI->UpdateConversationUI(CurNPC->GetNPCName(), TEXT(""));
	ConversationUI->SetVisibility(ESlateVisibility::Visible);
}

void AFarmLifeGameMode::SetLatestSpeech(const FString& SpeechText)
{
	LatestSpeech = SpeechText;
	UE_LOG(LogTemp, Warning, TEXT("ReqText Complete : [%s] %s"), *CurNPC->GetNPCName(), *LatestSpeech);

	// UI ����
	if(ConversationUI->IsVisible())
	{
		ConversationUI->UpdateConversationUI(CurNPC->GetNPCName(), LatestSpeech);
		CurNPC->LoadSpeechFileAndPlay(SpeechFilePath);
	}

	// ȣ���� ����

}

FString& AFarmLifeGameMode::GetLatestSpeech()
{
	return LatestSpeech;
}
#pragma endregion

#pragma region Talk to plant
void AFarmLifeGameMode::TalkToPlant(const FString& FileName, const FString& FilePath, const TArray<TObjectPtr<AActor>>& NewPlants)
{
	CurPlants = NewPlants;
	HttpActor->TalkToPlant(FileName, FilePath);
}

void AFarmLifeGameMode::SetTalkScore(int32 Score)
{
	TalkScore = Score;
	for(AActor* P : CurPlants)
	{
		// ���� ����

	}
}

int32 AFarmLifeGameMode::GetTalkScore()
{
	return TalkScore;
}
#pragma endregion

#pragma region Time flow
void AFarmLifeGameMode::StartTime()
{
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AFarmLifeGameMode::UpdateMinutes, TIME_UPDATE_INTERVAL, true, TIME_UPDATE_INTERVAL);
}

void AFarmLifeGameMode::StopTime()
{
	GetWorldTimerManager().ClearTimer(TimerHandle);
}

void AFarmLifeGameMode::UpdateMinutes()
{
	Minutes += TEN_MINUTES;
	if(Minutes == SIXTY_MINUTES)
	{
		UpdateHours();
	}
	TimerUI->UpdateTimerUI(Date, Hours, Minutes);
}

void AFarmLifeGameMode::UpdateHours()
{
	Minutes = 0;
	++Hours;

	// �ð� ������Ʈ �ϰ�ó��
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UHourUpdate::StaticClass(), OutActors);
	for(AActor* Actor : OutActors)
	{
		IHourUpdate* HU = Cast<IHourUpdate>(Actor);
		HU->OnHourUpdated(Hours);
	}

	if(Hours == END_HOUR)
	{
		UpdateDate();
	}
}

void AFarmLifeGameMode::UpdateDate()
{
	// �÷��̾ ��ȭ���̶�� ����


	Hours = START_HOUR;
	Minutes = 0;
	++Date;

	// ��¥ ������Ʈ �ϰ�ó��
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UDateUpdate::StaticClass(), OutActors);
	for (AActor* Actor : OutActors)
	{
		IDateUpdate* HU = Cast<IDateUpdate>(Actor);
		HU->OnDateUpdated(Date);
	}
}
#pragma endregion

#pragma region TTS

#pragma endregion
