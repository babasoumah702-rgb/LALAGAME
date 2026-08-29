Shader "LastCall/WorldText"
{
    Properties { _MainTex("Font",2D)="white"{} _Color("Tint",Color)=(1,1,1,1) }
    SubShader
    {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" "RenderPipeline"="UniversalPipeline" }
        Pass
        {
            Blend SrcAlpha OneMinusSrcAlpha
            ZWrite Off
            ZTest LEqual
            Cull Off
            HLSLPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
            TEXTURE2D(_MainTex);SAMPLER(sampler_MainTex);
            CBUFFER_START(UnityPerMaterial)
            float4 _Color;
            CBUFFER_END
            struct V {float4 positionOS:POSITION;float2 uv:TEXCOORD0;float4 color:COLOR;};
            struct F {float4 positionCS:SV_POSITION;float2 uv:TEXCOORD0;float4 color:COLOR;};
            F vert(V v){F o;o.positionCS=TransformObjectToHClip(v.positionOS.xyz);o.uv=v.uv;o.color=v.color*_Color;return o;}
            half4 frag(F i):SV_Target{return half4(i.color.rgb,i.color.a*SAMPLE_TEXTURE2D(_MainTex,sampler_MainTex,i.uv).a);}
            ENDHLSL
        }
    }
}
