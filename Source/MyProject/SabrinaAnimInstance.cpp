// Fill out your copyright notice in the Description page of Project Settings.

#include "SabrinaAnimInstance.h"
#include "SabrinaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void USabrinaAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CacheOwner();
}

void USabrinaAnimInstance::CacheOwner()
{
	SabrinaCharacter = Cast<ASabrinaCharacter>(TryGetPawnOwner());
	if (SabrinaCharacter)
	{
		MovementComponent = SabrinaCharacter->GetCharacterMovement();
	}
}

void USabrinaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!SabrinaCharacter || !MovementComponent)
	{
		CacheOwner();
	}

	if (!SabrinaCharacter || !MovementComponent)
	{
		return;
	}

	const FVector Velocity = SabrinaCharacter->GetVelocity();

	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;
	Speed = HorizontalVelocity.Size();

	bIsInAir = MovementComponent->IsFalling();
	bIsAccelerating = MovementComponent->GetCurrentAcceleration().SizeSquared() > 0.0f;

	// Direction вручну: кут між напрямком руху та напрямком погляду персонажа
	Direction = 0.0f;
	if (!HorizontalVelocity.IsNearlyZero())
	{
		const FRotator ActorRotation = SabrinaCharacter->GetActorRotation();
		const FVector ForwardVector = FRotationMatrix(ActorRotation).GetScaledAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(ActorRotation).GetScaledAxis(EAxis::Y);
		const FVector NormalizedVelocity = HorizontalVelocity.GetSafeNormal();

		const float ForwardDot = FVector::DotProduct(NormalizedVelocity, ForwardVector);
		const float RightDot = FVector::DotProduct(NormalizedVelocity, RightVector);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}
}
