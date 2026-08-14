// recordController.cpp

#include "recordController.h"
#include "DxLib.h"
#include "gv.h"              // key[] のサイズ用、必要なら extern
#include "stateManager.h"    // StateManager::GetState()
#include "replay.h"          // updateReplayInput()
#include <cstring>           // memset


// 録画モード用グローバル変数
bool recordingMode = 0;   // (true:録画モード, false:通常モード)
int  replayLoopCount = 8;    // 連続リプレイ録画個数
bool is_tate = 0;

RecordController::RecordController()
    : m_step(RecordStep::InitMenu)
    , m_waitTimer(0)
    , m_replayCount(0)
    , m_replayEnded(false)
{
}

void RecordController::Update(int key[256]) {
    // 全キーをゼロクリア（リプレイ中以外）
    if (StateManager::GetState() != Joutai::Replay) {
        memset(key, 0, sizeof(int) * 256);
    }

    switch (m_step) {
    case RecordStep::InitMenu:
        changeStep(RecordStep::WaitBeforeR, 120); // メニュー画面で 2 秒待機
        break;

    case RecordStep::WaitBeforeR:
        if (--m_waitTimer <= 0) {
            if (StateManager::GetState() == Joutai::Menu) {
                key[KEY_INPUT_R] = 1;
                changeStep(RecordStep::PressR);
            }
            else {
                if (m_replayCount >= replayLoopCount) {
                    key[KEY_INPUT_Q] = 1;
                    changeStep(RecordStep::PressQ);
                }
                else {
                    key[KEY_INPUT_N] = 1;
                    changeStep(RecordStep::PressR);
                }
            }
        }
        break;

    case RecordStep::PressR:
        if (StateManager::GetState() == Joutai::Replay) {
            changeStep(RecordStep::WaitReplayEnd, 150); // リプレイ終了後 2.5 秒待機
        }
        else if (StateManager::GetState() == Joutai::Menu) {
            key[KEY_INPUT_Q] = 1;
            changeStep(RecordStep::PressQ);
        }
        break;

    case RecordStep::WaitReplayEnd: {
        Joutai st = StateManager::GetState();
        if (st == Joutai::Win || st == Joutai::Lose || m_replayEnded) {
            m_replayCount++;
            m_replayEnded = false;
            changeStep(RecordStep::WaitBeforeR, 150); // リプレイ終了後 2.5 秒待機
        }
        else if (st == Joutai::Menu) {
            key[KEY_INPUT_Q] = 1;
            changeStep(RecordStep::PressQ);
        }
        break;
    }
    case RecordStep::PressQ:
        m_step = RecordStep::Done;
        break;

    case RecordStep::Done:
        break;
    }

    // リプレイ中ならリプレイデータで上書き
    if (StateManager::GetState() == Joutai::Replay) {
        if (!updateReplayInput()) {
            m_replayEnded = true;
        }
    }
}

void RecordController::changeStep(RecordStep next, int waitFrames) {
    m_step = next;
    m_waitTimer = waitFrames;
}