#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LalalandPlayerArmsComponent.generated.h"
class UCameraComponent;
class USkeletalMeshComponent;
class ALalalandGlassProp;

// Portable presentation only. No money/relationship changes, scene names or routes.
UCLASS(ClassGroup=(Lalaland),meta=(BlueprintSpawnableComponent))
class LALALANDUNREAL_API ULalalandPlayerArmsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULalalandPlayerArmsComponent();
    UFUNCTION(BlueprintCallable) bool InitializeArms(UCameraComponent* Camera);
    UFUNCTION(BlueprintCallable) bool BeginCupAction(ALalalandGlassProp* Cup,const FTransform& RestWorld,const FString& Action);
    UFUNCTION(BlueprintCallable) void CancelCupAction();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    USkeletalMeshComponent* GetArmsMesh() const { return Arms; }
    FString GetAction() const { return CurrentAction; }
    bool IsFinished() const { return bFinished; }
private:
    void UpdateCup();
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Arms;
    UPROPERTY(Transient) TObjectPtr<ALalalandGlassProp> Glass;
    FTransform Grip,Rest;
    FVector ReachMesh;
    FString CurrentAction,AssetVersion=TEXT("v024");
    float Elapsed=0,Duration=0;
    bool bFinished=true,bAttached=false;
};
