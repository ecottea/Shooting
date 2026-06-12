#include "replay.h"
#include "DxLib.h"
#include "gv.h"
#include "stageData.h"
#include "initial.h"
#include "imgSoundLoad.h"
#include <fstream>

bool replayActive = false;
std::vector<uint8_t> replayKeyHistory;
unsigned int gameSeed = 0;

static std::vector<uint8_t> replayData;
static size_t replayFrameIndex = 0;

uint8_t packReplayKey(const int key[256]) {
    uint8_t d = 0;
    if (key[KEY_INPUT_NUMPAD4]) d |= REPLAY_KEY_LEFT;
    if (key[KEY_INPUT_NUMPAD6]) d |= REPLAY_KEY_RIGHT;
    if (key[KEY_INPUT_NUMPAD8]) d |= REPLAY_KEY_UP;
    if (key[KEY_INPUT_NUMPAD5]) d |= REPLAY_KEY_DOWN;
    if (key[KEY_INPUT_C])       d |= REPLAY_KEY_SLOW;
    if (key[KEY_INPUT_V])       d |= REPLAY_KEY_SHOT;
    return d;
}

void unpackReplayKey(uint8_t data, int key[256]) {
    key[KEY_INPUT_R] = 0;
    key[KEY_INPUT_Q] = 0;
    if (data & REPLAY_KEY_LEFT)  key[KEY_INPUT_NUMPAD4]++; else key[KEY_INPUT_NUMPAD4] = 0;
    if (data & REPLAY_KEY_RIGHT) key[KEY_INPUT_NUMPAD6]++; else key[KEY_INPUT_NUMPAD6] = 0;
    if (data & REPLAY_KEY_UP)    key[KEY_INPUT_NUMPAD8]++; else key[KEY_INPUT_NUMPAD8] = 0;
    if (data & REPLAY_KEY_DOWN)  key[KEY_INPUT_NUMPAD5]++; else key[KEY_INPUT_NUMPAD5] = 0;
    if (data & REPLAY_KEY_SLOW)  key[KEY_INPUT_C]++;       else key[KEY_INPUT_C] = 0;
    if (data & REPLAY_KEY_SHOT)  key[KEY_INPUT_V]++;       else key[KEY_INPUT_V] = 0;
}

bool saveReplay(int stageNum) {
    std::string filename = std::string("saveData/replay_") + stageData[stageNum].stageId + ".dat";
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;

    // 先頭4バイトにシード値を書き込む
    ofs.write(reinterpret_cast<const char*>(&gameSeed), sizeof(gameSeed));

    // 続けてキー入力履歴を書き込む
    ofs.write(reinterpret_cast<const char*>(replayKeyHistory.data()),
        replayKeyHistory.size() * sizeof(uint8_t));

    return true;
}

bool loadReplay(int stageNum, std::vector<uint8_t>& outData, unsigned int& outSeed) {
    std::string filename = std::string("saveData/replay_") + stageData[stageNum].stageId + ".dat";
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    // シード読み取り
    if (!ifs.read(reinterpret_cast<char*>(&outSeed), sizeof(outSeed)))
        return false;

    // 残りをキー履歴として読み取り
    outData.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return true;
}

bool startReplay(int stageNum) {
    unsigned int seed = 0;
    std::vector<uint8_t> loadedData;
    if (!loadReplay(stageNum, loadedData, seed))
        return false;

    // リプレイ時の乱数状態を復元
    gameSeed = seed;
    SRand((int)gameSeed);

    iniGame();

    replayData = std::move(loadedData);
    replayFrameIndex = 0;
    replayKeyHistory.clear();

    if (currentBGMHandle != stageData[stageNum].bgmHandle) {
        if (currentBGMHandle != -1) StopSoundMem(currentBGMHandle);
        currentBGMHandle = stageData[stageNum].bgmHandle;
        PlaySoundMem(currentBGMHandle, DX_PLAYTYPE_LOOP);
    }

    replayActive = true;
    joutaiFlag = Joutai::Replay;
    return true;
}

bool updateReplayInput() {
    if (replayFrameIndex >= replayData.size())
        return false;
    unpackReplayKey(replayData[replayFrameIndex++], key);
    return true;
}