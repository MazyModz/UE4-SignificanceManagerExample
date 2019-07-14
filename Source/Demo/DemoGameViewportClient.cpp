// Fill out your copyright notice in the Description page of Project Settings.

#include "DemoGameViewportClient.h"

#include "SignificanceManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UDemoGameViewportClient::UDemoGameViewportClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDemoGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* MyWorld = GetWorld();
	if(MyWorld != nullptr)
	{
		const bool bHasValidData = CachedPlayerController.IsValid();
		if (!bHasValidData)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(MyWorld, 0);
			CachedPlayerController = TWeakObjectPtr<class APlayerController>(PC);
		}

		if (bHasValidData)
		{
			USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(MyWorld);
			if (SignificanceManager)
			{
				FVector ViewLocation;
				FRotator ViewRotation;
				CachedPlayerController.Get()->GetPlayerViewPoint(ViewLocation, ViewRotation);

				TArray<FTransform> Viewpoints;
				Viewpoints.Emplace(ViewRotation, ViewLocation, FVector::OneVector);

				SignificanceManager->Update(Viewpoints);
			}
		}
	}
}
