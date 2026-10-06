#include "MixologySequence.h"
void FMixSequence::Begin(const FString& Method,float Now){Start=Now;Shake=Method.Contains(TEXT("shake"));Stir=!Shake&&Method.Contains(TEXT("stir"));}
float FMixSequence::Duration()const{return Shake?3.6f:Stir?2.5f:1.7f;}
EMixStage FMixSequence::Stage(float Now)const{
 float T=FMath::Max(0.f,Now-Start);
 if(Shake){if(T<1.5f)return EMixStage::Mixing;if(T<1.9f)return EMixStage::Uncapping;if(T<3.2f)return EMixStage::Serving;}
 else{if(T<1.3f)return EMixStage::Serving;if(Stir&&T<2.1f)return EMixStage::Mixing;}
 return T<Duration()?EMixStage::Finishing:EMixStage::Ready;
}
float FMixSequence::StageProgress(float Now)const{
 float T=FMath::Max(0.f,Now-Start);auto S=Stage(Now);float Begin=0,Length=1;
 if(Shake){if(S==EMixStage::Mixing)Length=1.5f;else if(S==EMixStage::Uncapping){Begin=1.5f;Length=.4f;}else if(S==EMixStage::Serving){Begin=1.9f;Length=1.3f;}else{Begin=3.2f;Length=.4f;}}
 else{if(S==EMixStage::Serving)Length=1.3f;else if(S==EMixStage::Mixing){Begin=1.3f;Length=.8f;}else{Begin=Stir?2.1f:1.3f;Length=.4f;}}
 return FMath::Clamp((T-Begin)/Length,0.f,1.f);
}
float FMixSequence::FillProgress(float Now)const{float ServingStart=Shake?1.9f:0;return FMath::Clamp((Now-Start-ServingStart)/1.3f,0.f,1.f);}
FString FMixSequence::Label(float Now)const{
 switch(Stage(Now)){case EMixStage::Mixing:return Shake?TEXT("摇和材料"):TEXT("杯内轻搅");case EMixStage::Uncapping:return TEXT("移开顶盖，准备倒酒");case EMixStage::Serving:return Shake?TEXT("过滤并倒入杯中"):TEXT("直接兑入杯中");case EMixStage::Finishing:return TEXT("收回酒具，成品就绪");default:return TEXT("成品已就绪");}
}
