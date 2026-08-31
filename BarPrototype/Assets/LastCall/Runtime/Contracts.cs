using System;
namespace LastCall
{
    [Serializable] public class PointDto { public string area;public float x, y, z; }
    [Serializable] public class EntryDto { public string id, name, description, spawn; }
    [Serializable] public class SaveDto { public string id, role, status, updatedAt; public int night; public float elapsed; }
    [Serializable] public class ChoiceOptionDto { public string value, label; }
    [Serializable] public class ChoiceDto { public string id, label, prompt; public ChoiceOptionDto[] options; }
    [Serializable] public class ChoiceAnswersDto { public string domain, career_stage, preferred_topic_density; }
    [Serializable] public class BootstrapDto { public int version; public string title, model, modelBase; public bool modelConfigured; public EntryDto[] roles, intents, styles; public ChoiceDto[] choices; public SaveDto[] sessions; }
    [Serializable] public class ModelConfigRequestDto { public string @base, model, key; public bool keepKey, clearKey; }
    [Serializable] public class ModelConfigResultDto { public string @base, model; public bool configured; }
    [Serializable] public class ActorDto { public string id, name, color, animation, destination, location,conversationTarget,area,posture,gesture; public float x,y,z,yaw,facingUntil,gestureAt; public int routeVersion; public int interactions; public bool interactable; public PointDto[] route; }
    [Serializable] public class EventDto { public string id, time, actor, name, text, source, level,type,target,objectTarget,generationSource,privacy,audio; public int seq; public bool hasParent; }
    [Serializable] public class CardDto { public string id, name, type, intent, text, effect, lockReason; public string[] expressions; public bool ready, unlocked; public float cooldown, cooldownRemaining; }
    [Serializable] public class LocationDto { public string id, name; public float x, z, radius, privacy; public int capacity; }
    [Serializable] public class ReflectionDto { public string title, behavior, ending; public string[] trends, events, chain; }
    [Serializable] public class ReplyDto { public string id,actor,eventId,status,error,errorCode,model;public int elapsedMs,chapter; }
    [Serializable] public class InteractionOptionDto { public string id,label,disabledReason;public bool selected,replaceable,targetRequired,enabled; }
    [Serializable] public class InteractionGroupDto { public string id,label;public InteractionOptionDto[] options; }
    [Serializable] public class InteractionDto { public string contextId,nextTitle,nextHint,nextGroup,nextActionId;public InteractionGroupDto[] groups; }
    [Serializable] public class SceneOneDto { public string phase;public bool drinkPlaced,seated;public float drinkPlacedAt,arrivalAt,phoneAt; }
    [Serializable] public class SceneTwoDto { public string phase,gamePrompt;public bool rainStopped,deckPlaced;public float drinkLevel,musicLevel;public int coasters,guests,games; }
    [Serializable] public class GazeDto { public string actor,gesture;public string[] order;public int round,pauseMs; }
    [Serializable] public class SceneThreeDto
    {
        public string phase,reader,cardName,theme,question,firstResponder,playerStance,playerMove,leaver,follower;
        public bool isJoker,highTension,jokerUsed;
        public int round,rounds;
        public float askedAt;
        public string[] responded;
        public GazeDto lastGaze;
    }
    [Serializable] public class StoryDto { public int chapter,budgetCalls,budgetTokens;public string phase;public float stageAt; }
    [Serializable] public class NightCueDto {public string id,kind,text,owner;public float duration;public bool consumed;}
    [Serializable] public class LateNightDto { public int chapter;public string phase,area,powerState,posture,ending,choice;public float powerAt;public bool doorOpen,canChocolate;public string[] companions;public NightCueDto cue;}
    [Serializable] public class StateDto
    {
        public int version, cursor, night, calls, tokens;
        public string sessionId, clock, status, mode, modeReason, role, lastTarget;
        public float elapsed;
        public bool paused, busy, cardsOffered, cardsJoined, lastCall, pastDrink;
        public ActorDto[] characters;
        public EventDto[] events;
        public CardDto[] cards;
        public LocationDto[] locations;
        public ReflectionDto reflection;
        public StoryDto story;public LateNightDto late;
        public IntroDto intro;
        public SceneOneDto scene1;
        public SceneTwoDto scene2;
        public SceneThreeDto scene3;
        public InteractionDto interaction;
        public ReplyDto[] replies;
    }
    [Serializable] public class Envelope { public string type, id, message, error; public int version, port; public bool ready; public StateDto state; }
    [Serializable] public class CommandDto
    {
        public int version=1,cursor;
        public string sessionId;
        public string id=Guid.NewGuid().ToString("N"), type, target, intent, text, card, actor, location,requestId,objectTarget,tone,movement;
        public string area;public float x,y,z,yaw;
        public bool paused,online,open;
    }
    [Serializable] public class SessionRequest
    {
        public string playerId, role, entryIntent, style, mode="new", sessionId;
        public bool online=true;
        public int seed=821;
        public string opening,entryMode,entryContext,story;
        public ChoiceAnswersDto choices;
    }
    [Serializable] public class IntroDto
    {
        public int version,checkpoint;
        public float progress;
        public string phase,entryMode,message,hint,messageSource,generationStatus,attitude,intent,playerText;
        public bool checkedMessage,phoneVisible;
    }
    [Serializable] public class PositionBatchDto
    {
        public int version=1,cursor;
        public string id=Guid.NewGuid().ToString("N"),type="positions",sessionId;
        public CommandDto[] items;
    }
}
