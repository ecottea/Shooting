// stageData.h
#pragma once
#include <vector> 

// ステージ情報をまとめた構造体
struct StageInfo {
    const char* stageId;       // ステージID（半角英数）
    const char* description;   // ステージ説明
    const char* bgmFileName;   // BGMファイル名
    void (*patternFunc)();     // 敵パターン関数ポインタ
    int bgmHandle = -1;        // BGMハンドラ（後で代入）
    unsigned int bestTime = 59999;     // 最短クリアフレーム数（後で代入）
};

// サイズは初期化リストから自動決定
extern std::vector<StageInfo> stageData;

// 各敵パターン関数の宣言（ステージ数分）
#define EV (); extern void
extern void Pattern_SampleForAI
EV Pattern1_Aimed3Way
EV Pattern2_Round
EV Pattern3_Burst
EV Pattern4_GeometryFlower
EV Pattern5_DynamicRose
EV Pattern6_LissajousCurtain
EV Pattern7_GeometryBeauty
EV Pattern8_LogSpiralPetal
EV Pattern9_FibonacciSpiral
EV Pattern10_FunToDodge
EV Pattern11_TimeLagAmbush
EV Pattern12_HighDifficultyComplex
EV Pattern13_Aimed10WayRandom
EV Pattern14_Aimed7WayCurtain
EV Pattern15_RandomDirectionMultiSpeed
EV Pattern16_PendulumTripleAim
EV Pattern17_GeometricBoss
EV Pattern18_Aimed10WayRandom
EV Pattern18_KaleidoscopeBoss
EV Pattern_Deepseek3
EV Pattern1_Lotus
EV Pattern_BeautifulSpiral
EV Pattern18_FireworkSpiral
EV Pattern1_GeometricFlower
EV Pattern2_Fun
EV Pattern3_Spectacular
EV Pattern_Wafu
EV Pattern13_Japanese
EV Pattern_Wafu_SakuraFan
EV Pattern_WafuDanmaku
EV Pattern4_Wafu
EV Pattern_SakuraFubuki
EV Pattern_WaFuu
EV Pattern_Waterfall_deepseek
EV Pattern_Waterfall_gemini
EV enemyPattern13
EV Pattern_WaterfallBoss
EV Pattern_Fall_sakana
EV Pattern_Waterfall_claude
EV Pattern_Fall_Qwen
EV Pattern_WaveParticleBoundary_deepseek
EV Pattern_WaveAndParticle_gemini
EV Pattern_Namitsubu_GPT
EV Pattern_WaveParticleBoundary_grok
EV Pattern_WaveParticleBoundary_sakana
EV Pattern_WaveParticleBoundary_claude
EV Pattern_NamiTsubuQwen
EV Pattern_Tmp();
