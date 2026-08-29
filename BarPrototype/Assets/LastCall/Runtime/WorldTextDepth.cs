using UnityEngine;
namespace LastCall
{
    public static class WorldTextDepth
    {
        public static void Apply(TextMesh text)
        {
            var renderer=text.GetComponent<Renderer>();
            var shader=Shader.Find("LastCall/WorldText");
            if(!renderer||!shader)return;
            if(renderer.sharedMaterial&&renderer.sharedMaterial.shader==shader)return;
            var material=new Material(shader);
            material.mainTexture=text.font?text.font.material.mainTexture:renderer.sharedMaterial.mainTexture;
            renderer.material=material;
        }
    }
}
