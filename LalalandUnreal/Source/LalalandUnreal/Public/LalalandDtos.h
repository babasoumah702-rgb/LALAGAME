#pragma once

#include "CoreMinimal.h"
#include "LalalandDtos.generated.h"

USTRUCT(BlueprintType)
struct FLalalandEntryDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString name;
    UPROPERTY(BlueprintReadOnly) FString description;
    UPROPERTY(BlueprintReadOnly) FString spawn;
};

USTRUCT(BlueprintType)
struct FLalalandChoiceOptionDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString value;
    UPROPERTY(BlueprintReadOnly) FString label;
};

USTRUCT(BlueprintType)
struct FLalalandChoiceDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString label;
    UPROPERTY(BlueprintReadOnly) FString prompt;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandChoiceOptionDto> options;
};

USTRUCT(BlueprintType)
struct FLalalandBootstrapDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 version = 0;
    UPROPERTY(BlueprintReadOnly) FString title;
    UPROPERTY(BlueprintReadOnly) FString model;
    UPROPERTY(BlueprintReadOnly) FString modelBase;
    UPROPERTY(BlueprintReadOnly) bool modelConfigured = false;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandEntryDto> roles;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandEntryDto> intents;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandEntryDto> styles;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandChoiceDto> choices;
};

USTRUCT(BlueprintType)
struct FLalalandPointDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString area;
    UPROPERTY(BlueprintReadOnly) double x = 0;
    UPROPERTY(BlueprintReadOnly) double y = 0;
    UPROPERTY(BlueprintReadOnly) double z = 0;
};

USTRUCT(BlueprintType)
struct FLalalandActorDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString name;
    UPROPERTY(BlueprintReadOnly) FString color;
    UPROPERTY(BlueprintReadOnly) FString animation;
    UPROPERTY(BlueprintReadOnly) FString destination;
    UPROPERTY(BlueprintReadOnly) FString location;
    UPROPERTY(BlueprintReadOnly) FString conversationTarget;
    UPROPERTY(BlueprintReadOnly) FString area;
    UPROPERTY(BlueprintReadOnly) FString posture;
    UPROPERTY(BlueprintReadOnly) FString gesture;
    UPROPERTY(BlueprintReadOnly) double x = 0;
    UPROPERTY(BlueprintReadOnly) double y = 0;
    UPROPERTY(BlueprintReadOnly) double z = 0;
    UPROPERTY(BlueprintReadOnly) double yaw = 0;
    UPROPERTY(BlueprintReadOnly) double facingUntil = 0;
    UPROPERTY(BlueprintReadOnly) double gestureAt = 0;
    UPROPERTY(BlueprintReadOnly) int32 routeVersion = 0;
    UPROPERTY(BlueprintReadOnly) int32 interactions = 0;
    UPROPERTY(BlueprintReadOnly) bool interactable = false;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandPointDto> route;
};

USTRUCT(BlueprintType)
struct FLalalandEventDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString time;
    UPROPERTY(BlueprintReadOnly) FString actor;
    UPROPERTY(BlueprintReadOnly) FString name;
    UPROPERTY(BlueprintReadOnly) FString text;
    UPROPERTY(BlueprintReadOnly) FString source;
    UPROPERTY(BlueprintReadOnly) FString level;
    UPROPERTY(BlueprintReadOnly) FString type;
    UPROPERTY(BlueprintReadOnly) FString target;
    UPROPERTY(BlueprintReadOnly) FString objectTarget;
    UPROPERTY(BlueprintReadOnly) FString generationSource;
    UPROPERTY(BlueprintReadOnly) FString privacy;
    UPROPERTY(BlueprintReadOnly) FString audio;
    UPROPERTY(BlueprintReadOnly) int32 seq = 0;
};

USTRUCT(BlueprintType)
struct FLalalandInteractionOptionDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString label;
    UPROPERTY(BlueprintReadOnly) FString disabledReason;
    UPROPERTY(BlueprintReadOnly) bool selected = false;
    UPROPERTY(BlueprintReadOnly) bool replaceable = false;
    UPROPERTY(BlueprintReadOnly) bool targetRequired = false;
    UPROPERTY(BlueprintReadOnly) bool enabled = false;
};

USTRUCT(BlueprintType)
struct FLalalandInteractionGroupDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString label;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandInteractionOptionDto> options;
};

USTRUCT(BlueprintType)
struct FLalalandInteractionDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString contextId;
    UPROPERTY(BlueprintReadOnly) FString nextTitle;
    UPROPERTY(BlueprintReadOnly) FString nextHint;
    UPROPERTY(BlueprintReadOnly) FString nextGroup;
    UPROPERTY(BlueprintReadOnly) FString nextActionId;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandInteractionGroupDto> groups;
};

USTRUCT(BlueprintType)
struct FLalalandSceneOneDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString phase;
    UPROPERTY(BlueprintReadOnly) bool drinkPlaced = false;
    UPROPERTY(BlueprintReadOnly) bool seated = false;
    UPROPERTY(BlueprintReadOnly) double drinkPlacedAt = -1;
    UPROPERTY(BlueprintReadOnly) double arrivalAt = -1;
    UPROPERTY(BlueprintReadOnly) double phoneAt = -1;
};

USTRUCT(BlueprintType)
struct FLalalandIntroDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 version = 0;
    UPROPERTY(BlueprintReadOnly) int32 checkpoint = 0;
    UPROPERTY(BlueprintReadOnly) double progress = 0;
    UPROPERTY(BlueprintReadOnly) FString phase;
    UPROPERTY(BlueprintReadOnly) FString entryMode;
    UPROPERTY(BlueprintReadOnly) FString message;
    UPROPERTY(BlueprintReadOnly) FString hint;
    UPROPERTY(BlueprintReadOnly) FString messageSource;
    UPROPERTY(BlueprintReadOnly) FString generationStatus;
    UPROPERTY(BlueprintReadOnly) FString attitude;
    UPROPERTY(BlueprintReadOnly) FString intent;
    UPROPERTY(BlueprintReadOnly) FString playerText;
    UPROPERTY(BlueprintReadOnly) bool ready = false;
    UPROPERTY(BlueprintReadOnly) bool checkedMessage = false;
    UPROPERTY(BlueprintReadOnly) bool phoneVisible = false;
};

USTRUCT(BlueprintType)
struct FLalalandReplyDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString id;
    UPROPERTY(BlueprintReadOnly) FString actor;
    UPROPERTY(BlueprintReadOnly) FString eventId;
    UPROPERTY(BlueprintReadOnly) FString status;
    UPROPERTY(BlueprintReadOnly) FString error;
    UPROPERTY(BlueprintReadOnly) FString errorCode;
    UPROPERTY(BlueprintReadOnly) FString model;
    UPROPERTY(BlueprintReadOnly) int32 elapsedMs = 0;
    UPROPERTY(BlueprintReadOnly) int32 chapter = 0;
};

USTRUCT(BlueprintType)
struct FLalalandStateDto
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 version = 0;
    UPROPERTY(BlueprintReadOnly) int32 cursor = 0;
    UPROPERTY(BlueprintReadOnly) int32 night = 0;
    UPROPERTY(BlueprintReadOnly) int32 calls = 0;
    UPROPERTY(BlueprintReadOnly) int32 tokens = 0;
    UPROPERTY(BlueprintReadOnly) FString sessionId;
    UPROPERTY(BlueprintReadOnly) FString clock;
    UPROPERTY(BlueprintReadOnly) FString status;
    UPROPERTY(BlueprintReadOnly) FString mode;
    UPROPERTY(BlueprintReadOnly) FString modeReason;
    UPROPERTY(BlueprintReadOnly) FString role;
    UPROPERTY(BlueprintReadOnly) FString lastTarget;
    UPROPERTY(BlueprintReadOnly) double elapsed = 0;
    UPROPERTY(BlueprintReadOnly) bool paused = false;
    UPROPERTY(BlueprintReadOnly) bool busy = false;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandActorDto> characters;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandEventDto> events;
    UPROPERTY(BlueprintReadOnly) FLalalandIntroDto intro;
    UPROPERTY(BlueprintReadOnly) FLalalandSceneOneDto scene1;
    UPROPERTY(BlueprintReadOnly) FLalalandInteractionDto interaction;
    UPROPERTY(BlueprintReadOnly) TArray<FLalalandReplyDto> replies;
};

USTRUCT(BlueprintType)
struct FLalalandCommandDto
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 version = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 cursor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString sessionId;
    // Command ids are intentionally randomized for idempotency, so they cannot
    // participate in Unreal's deterministic member initialization check.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (IgnoreForMemberInitializationTest)) FString id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString type;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString target;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString intent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString actor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString location;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString requestId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString objectTarget;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString tone;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString movement;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString area;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double x = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double y = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double z = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double yaw = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool paused = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool online = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool open = false;

    FLalalandCommandDto()
        : id(FGuid::NewGuid().ToString(EGuidFormats::Digits))
    {
    }
};

USTRUCT(BlueprintType)
struct FLalalandSessionRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString playerId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString role = TEXT("passerby");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString entryIntent = TEXT("observe_only");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString style = TEXT("natural");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString mode = TEXT("new");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString sessionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool online = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 seed = 821;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString opening = TEXT("scene0_v1");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString story = TEXT("scene1_v1");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString entryMode;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString entryContext;
};

struct FLalalandJson
{
    static bool ParseBootstrap(const FString& Json, FLalalandBootstrapDto& Out, FString& Error);
    static bool ParseStateEnvelope(const FString& Json, FLalalandStateDto& Out, FString& Error);
    static FString WriteSession(const FLalalandSessionRequest& Request);
    static FString WriteCommand(const FLalalandCommandDto& Command);
};
