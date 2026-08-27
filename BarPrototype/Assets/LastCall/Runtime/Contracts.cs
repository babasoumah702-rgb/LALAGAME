using System;
namespace LastCall
{
    [Serializable] public class PointDto { public float x, z; }
    [Serializable] public class EntryDto { public string id, name, description, spawn; }
    [Serializable] public class SaveDto { public string id, role, status, updatedAt; public int night; public float elapsed; }
    [Serializable] public class BootstrapDto { public int version; public string title, model; public bool modelConfigured; public EntryDto[] roles, intents, styles; public SaveDto[] sessions; }
    [Serializable] public class ActorDto { public string id, name, color, animation, destination, location; public float x, z, yaw; public int routeVersion; public bool interactable; public PointDto[] route; }
    [Serializable] public class EventDto { public string id, time, actor, name, text, source, level; public int seq; public bool hasParent; }
    [Serializable] public class CardDto { public string id, name, type, intent, text, effect; public string[] expressions; public bool ready; public float cooldown; }
    [Serializable] public class LocationDto { public string id, name; public float x, z, radius, privacy; public int capacity; }
    [Serializable] public class ReflectionDto { public string title, behavior, ending; public string[] trends, events, chain; }
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
    }
    [Serializable] public class Envelope { public string type, id, message, error; public int version, port; public bool ready; public StateDto state; }
    [Serializable] public class CommandDto
    {
        public int version=1,cursor;
        public string sessionId;
        public string id=Guid.NewGuid().ToString("N"), type, target, intent, text, card, actor, location;
        public float x,z,yaw;
        public bool paused,online;
    }
    [Serializable] public class SessionRequest
    {
        public string playerId, role, entryIntent, style, mode="new", sessionId;
        public bool online=true;
        public int seed=821;
    }
    [Serializable] public class PositionBatchDto
    {
        public int version=1,cursor;
        public string id=Guid.NewGuid().ToString("N"),type="positions",sessionId;
        public CommandDto[] items;
    }
}
