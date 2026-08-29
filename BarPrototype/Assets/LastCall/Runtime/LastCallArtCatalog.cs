using System;
using System.Linq;
using UnityEngine;

namespace LastCall
{
    [CreateAssetMenu(menuName="Last Call/Art catalog")]
    public sealed class LastCallArtCatalog : ScriptableObject
    {
        [Serializable] public class ModelItem { public string id; public GameObject prefab; }
        [Serializable] public class TextureItem { public string id; public Texture2D texture; }
        public ModelItem[] models;
        public TextureItem[] textures;
        public static LastCallArtCatalog Current;
        public GameObject Model(string id)=>models?.FirstOrDefault(x=>x.id==id)?.prefab;
        public Texture2D Texture(string id)=>textures?.FirstOrDefault(x=>x.id==id)?.texture;
    }
}
