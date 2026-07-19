#pragma once
#include <vector>
#include <cstdint>

// リプレイ用キー入力ビットマスク（1バイト内の割り当て）
constexpr uint8_t REPLAY_KEY_LEFT  = 1 << 0; // NUMPAD4
constexpr uint8_t REPLAY_KEY_RIGHT = 1 << 1; // NUMPAD6
constexpr uint8_t REPLAY_KEY_UP    = 1 << 2; // NUMPAD8
constexpr uint8_t REPLAY_KEY_DOWN  = 1 << 3; // NUMPAD5
constexpr uint8_t REPLAY_KEY_SLOW  = 1 << 4; // C
constexpr uint8_t REPLAY_KEY_SHOT  = 1 << 5; // V

extern bool replayActive;
extern std::vector<uint8_t> replayKeyHistory;
extern unsigned int gameSeed;


// 現在のキー入力状態から1バイトを生成
uint8_t packReplayKey(const int key[256]);

// 1バイトからキー配列を上書き（毎フレーム先頭で呼ぶ）
void unpackReplayKey(uint8_t data, int key[256]);

// リプレイデータの保存
bool saveReplay(int stageNum);

// リプレイデータの読み込み（成功時true）
bool loadReplay(int stageNum, std::vector<uint8_t>& outData, unsigned int& outSeed);

// リプレイ再生の開始
bool startReplay(int stageNum);

// 毎フレームのリプレイ入力更新（終了時false）
bool updateReplayInput();
