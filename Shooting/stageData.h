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
extern void EnemyPat_SampleForAI();
extern void EnemyPat_Tmp();
extern void EnemyPat_3Way();
extern void EnemyPat_Round();
extern void EnemyPat_Burst();
extern void EnemyPat_Namitsubu();
extern void EnemyPat_Smeargle();
extern void EnemyPat_Difficulty_Claude();
extern void EnemyPat_Geometry_Claude2();
extern void EnemyPat_Comet_Grok2();
extern void EnemyPat_Hakaikousen_DeepSeek2();
extern void EnemyPat_Tatsujinou_Gemini();
extern void EnemyPat_RazorLeaf_DeepSeek2();
extern void EnemyPat_Tatsujinou_Claude();
extern void EnemyPat_Geometry_DeepSeek();
extern void EnemyPat_Geometry_Gemini();
extern void EnemyPat_Geometry_ChatGPT();
extern void EnemyPat_Geometry_Grok();
extern void EnemyPat_Geometry_Sakana();
extern void EnemyPat_Geometry_Claude();
extern void EnemyPat_Geometry_Qwen();
extern void EnemyPat_Geometry_Vibe();
extern void EnemyPat_Dodge_DeepSeek();
extern void EnemyPat_Dodge_Gemini();
extern void EnemyPat_Dodge_ChatGPT();
extern void EnemyPat_Dodge_Grok();
extern void EnemyPat_Dodge_Sakana();
extern void EnemyPat_Dodge_Claude();
extern void EnemyPat_Dodge_Qwen();
extern void EnemyPat_Dodge_Vibe();
extern void EnemyPat_Beautiful_ChatGPT();
extern void EnemyPat_Beautiful_Claude();
extern void EnemyPat_Beautiful_DeepSeek();
extern void EnemyPat_Beautiful_Gemini();
extern void EnemyPat_Beautiful_Grok();
extern void EnemyPat_Beautiful_Sakana();
extern void EnemyPat_Beautiful_Qwen();
extern void EnemyPat_Beautiful_Vibe();
extern void EnemyPat_Japanese_DeepSeek();
extern void EnemyPat_Japanese_ChatGPT();
extern void EnemyPat_Japanese_Grok();
extern void EnemyPat_Japanese_Sakana();
extern void EnemyPat_Japanese_Qwen();
extern void EnemyPat_Japanese_Gemini();
extern void EnemyPat_Japanese_Claude();
extern void EnemyPat_Japanese_Vibe();
extern void EnemyPat_Waterfall_DeepSeek();
extern void EnemyPat_Waterfall_Gemini();
extern void EnemyPat_Waterfall_ChatGPT();
extern void EnemyPat_Waterfall_Grok();
extern void EnemyPat_Waterfall_Sakana();
extern void EnemyPat_Waterfall_Claude();
extern void EnemyPat_Waterfall_Qwen();
extern void EnemyPat_Waterfall_Vibe();
extern void EnemyPat_Namistubu_DeepSeek();
extern void EnemyPat_Namistubu_Gemini();
extern void EnemyPat_Namistubu_ChatGPT();
extern void EnemyPat_Namistubu_Grok();
extern void EnemyPat_Namistubu_Sakana();
extern void EnemyPat_Namistubu_Claude();
extern void EnemyPat_Namistubu_Qwen();
extern void EnemyPat_Namitsubu_Vibe();
extern void EnemyPat_Comet_DeepSeek();
extern void EnemyPat_Comet_ChatGPT();
extern void EnemyPat_Comet_Gemini();
extern void EnemyPat_Comet_Grok();
extern void EnemyPat_Comet_Sakana();
extern void EnemyPat_Comet_Claude();
extern void EnemyPat_Comet_Qwen();
extern void EnemyPat_Comet_Vibe();
extern void EnemyPat_Thunder_DeepSeek();
extern void EnemyPat_Thunder_ChatGPT();
extern void EnemyPat_Thunder_Gemini();
extern void EnemyPat_Thunder_Grok();
extern void EnemyPat_Thunder_Sakana();
extern void EnemyPat_Thunder_Claude();
extern void EnemyPat_Thunder_Qwen();
extern void EnemyPat_Thunder_Vibe();
extern void EnemyPat_Blizzard_DeepSeek();
extern void EnemyPat_Blizzard_ChatGPT();
extern void EnemyPat_Blizzard_Gemini();
extern void EnemyPat_Blizzard_Grok();
extern void EnemyPat_Blizzard_Sakana();
extern void EnemyPat_Blizzard_Claude();
extern void EnemyPat_Blizzard_Qwen();
extern void EnemyPat_Blizzard_Vibe();
extern void EnemyPat_Dragon_DeepSeek();
extern void EnemyPat_Dragon_ChatGPT();
extern void EnemyPat_Dragon_Gemini();
extern void EnemyPat_Dragon_Grok();
extern void EnemyPat_Dragon_Sakana();
extern void EnemyPat_Dragon_Claude();
extern void EnemyPat_Dragon_Qwen();
extern void EnemyPat_Dragon_Vibe();
extern void EnemyPat_Hakaikousen_DeepSeek();
extern void EnemyPat_Hakaikousen_ChatGPT();
extern void EnemyPat_Hakaikousen_Gemini();
extern void EnemyPat_Hakaikousen_Grok();
extern void EnemyPat_Hakaikousen_Sakana();
extern void EnemyPat_Hakaikousen_Claude();
extern void EnemyPat_Hakaikousen_Qwen();
extern void EnemyPat_Hakaikousen_Vibe();
extern void EnemyPat_Sun_DeepSeek();
extern void EnemyPat_Sun_ChatGPT();
extern void EnemyPat_Sun_Gemini();
extern void EnemyPat_Sun_Grok();
extern void EnemyPat_Sun_Sakana();
extern void EnemyPat_Sun_Claude();
extern void EnemyPat_Sun_Qwen();
extern void EnemyPat_Sun_Vibe();
extern void EnemyPat_RazorLeaf_DeepSeek();
extern void EnemyPat_RazorLeaf_ChatGPT();
extern void EnemyPat_RazorLeaf_Gemini();
extern void EnemyPat_RazorLeaf_Grok();
extern void EnemyPat_RazorLeaf_Sakana();
extern void EnemyPat_RazorLeaf_Claude();
extern void EnemyPat_RazorLeaf_Qwen();
extern void EnemyPat_RazorLeaf_Vibe();
extern void EnemyPat_Poison_DeepSeek();
extern void EnemyPat_Poison_ChatGPT();
extern void EnemyPat_Poison_Gemini();
extern void EnemyPat_Poison_Grok();
extern void EnemyPat_Poison_Sakana();
extern void EnemyPat_Poison_Claude();
extern void EnemyPat_Poison_Qwen();
extern void EnemyPat_Poison_Vibe();
extern void EnemyPat_WashingMachine_DeepSeek();
extern void EnemyPat_WashingMachine_ChatGPT();
extern void EnemyPat_WashingMachine_Gemini();
extern void EnemyPat_WashingMachine_Grok();
extern void EnemyPat_WashingMachine_Sakana();
extern void EnemyPat_WashingMachine_Claude();
extern void EnemyPat_WashingMachine_Qwen();
extern void EnemyPat_WashingMachine_Vibe();
