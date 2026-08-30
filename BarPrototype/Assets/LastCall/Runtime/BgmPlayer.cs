using UnityEngine;

namespace LastCall
{
    // Looping background music, loaded by Resources path so it stays independent of asset GUID wiring.
    // Volume is kept low so dialogue / TTS reads clearly; adjust the `volume` field (or the prefab value)
    // to make the bed louder or softer.
    [DefaultExecutionOrder(100)]
    public sealed class BgmPlayer : MonoBehaviour
    {
        private const string ClipPath = "Audio/bgm-jazz-rnb";
        [SerializeField, Range(0f, 1f)] private float volume = 0.2f;

        private AudioSource source;

        private void Awake()
        {
            var clip = Resources.Load<AudioClip>(ClipPath);
            if (clip == null)
            {
                Debug.LogWarning("BGM clip not found at Resources/" + ClipPath);
                return;
            }
            source = gameObject.AddComponent<AudioSource>();
            source.clip = clip;
            source.loop = true;
            source.playOnAwake = false;
            source.spatialBlend = 0f;
            source.volume = volume;
            source.Play();
        }

        private void OnDestroy()
        {
            if (source != null) source.Stop();
        }
    }
}
