// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "DemoGameViewportClient.generated.h"

/**
 * 
 */
UCLASS()
class DEMO_API UDemoGameViewportClient : public UGameViewportClient
{
	GENERATED_UCLASS_BODY()
	
public:

	// ~ begin UGameViewportClient interface
	virtual void Tick(float DeltaTime) override;
	// ~ end UGameViewportClient

protected:
	/** Cached ptr to our player controller that we should get the view from */
	TWeakObjectPtr<class APlayerController> CachedPlayerController;
};
