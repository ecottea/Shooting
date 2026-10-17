// tasAutoMaker.cpp
// 全自動TASリプレイ作成（DFS探索）
//
// 仕様:
//   - 9方向（左上・上・右上・左・停止・右・左下・下・右下）を1フレームずつ試す
//   - ショットキーは常にON、低速キーは常にOFF
//   - 敵は1体のみで、enemy.x に近づく方向を優先的に探索
//   - 被弾したらその候補を破棄
//   - クリア（Win）したら探索成功
//   - 最大フレーム数はデフォルト1500

#include "tasAutoMaker.h"
#include "tasController.h"   // iniGameForTas()
#include "DxLib.h"
#include "gv.h"               // player, enemy, key, count など
#include "stageData.h"
#include "initial.h"
#include "replay.h"           // replayKeyHistory, packReplayKey, unpackReplayKey, REPLAY_KEY_*
#include "stateManager.h"     // StateManager, Joutai
#include "gameScreen.h"       // updateStars()
#include "enemy.h"
#include "enemyShot.h"
#include "player.h"
#include "playerShot.h"
#include "imgSoundLoad.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>   // exit() 用

// 自動TASモードの有効/無効を切り替える
bool g_isAutoTasMode;

namespace {

    //-----------------------------------------------
    // 探索用の定数・グローバル
    //-----------------------------------------------

    // 探索最大フレーム数（デフォルト）
    const int kDefaultMaxDepth = 60 * 60;

    // 探索局面数（DFSノード数）
    static int s_dfsNodeCount = 0;

    // 9方向の候補を作成する
    uint8_t makeDirInput(bool left, bool up, bool right, bool down)
    {
        int k[256] = { 0 };

        if (left)  k[KEY_INPUT_NUMPAD4] = 1;
        if (up)    k[KEY_INPUT_NUMPAD8] = 1;
        if (right) k[KEY_INPUT_NUMPAD6] = 1;
        if (down)  k[KEY_INPUT_NUMPAD5] = 1;

        // ショット常時ON、低速常時OFF
        k[KEY_INPUT_V] = 1;
        k[KEY_INPUT_C] = 0;

        return packReplayKey(k);
    }

    // 現在の自機に最も近い敵は1体のみ → そのX座標を返す
    float getTargetEnemyX()
    {
        // 敵は1体しかいない想定
        return (float)enemy.x;
    }

    // 被弾したか？
    bool wasPlayerHit()
    {
        return StateManager::currentState == Joutai::Lose;
    }

    // ステージクリアしたか？
    bool isCleared()
    {
        return StateManager::currentState == Joutai::Win;
    }

    // 探索中に出る効果音を止める
    void stopSearchSounds()
    {
        StopSoundMem(sound_enemyShot_noize);
        StopSoundMem(sound_enemyShot_light);
        StopSoundMem(sound_enemyShot_medium);
        StopSoundMem(sound_enemyShot_heavy);
        StopSoundMem(sound_enemyShot_extreme);
        StopSoundMem(sound_enemyCharge);
        StopSoundMem(sound_playerDestroyed);
        StopSoundMem(sound_playerShotHit_default);
        StopSoundMem(sound_playerShotHit_bossLowHP);
    }

    // 探索中の現在のシーンを1回だけ描画して画面を更新する
    // 描画先を保存・復元することでmain.cpp側の描画フローを壊さない
    void renderCurrentScene()
    {
        int oldScreen = GetDrawScreen();   // 現在の描画先を退避

        SetDrawScreen(DX_SCREEN_BACK);     // ウィンドウ背面バッファへ
        ClearDrawScreen();

        // ゲーム本体と同じ描画処理
        backGround();
        playerDisp();
        playerShotDisp();
        enemyDisp();
        enemyShotDisp();
        drawSidePanel();
        foreGround();

        // 探索局面数を表示（左下に黄文字）
        DrawFormatString(10, 460, GetColor(255, 255, 0), "探索局面数: %d", s_dfsNodeCount);

        ScreenFlip();                      // 実際に表示
        ProcessMessage();                  // ウィンドウ応答性を保つ

        SetDrawScreen(oldScreen);          // 描画先を元に戻す
    }

    // Qキーが押されたらプログラムを強制終了する
    void pollEmergencyStop()
    {
        ProcessMessage();   // キー入力を更新

        if (CheckHitKey(KEY_INPUT_Q)) {
            DxLib_End();    // DxLibの後始末
            exit(0);        // プログラム終了
        }
    }

    //-----------------------------------------------
    // ゲーム状態の復元（親フレームまで）
    //-----------------------------------------------

    // 最初から targetFrame フレームまで、キー履歴を再適用して
    // ゲーム状態を復元する（描画なし・被弾処理なし）
    void restoreToFrame(int targetFrame)
    {
        iniGameForTas();

        key[KEY_INPUT_NUMPAD4] = 0;
        key[KEY_INPUT_NUMPAD6] = 0;
        key[KEY_INPUT_NUMPAD8] = 0;
        key[KEY_INPUT_NUMPAD5] = 0;
        key[KEY_INPUT_V] = 1;  // ショットは常にON
        key[KEY_INPUT_C] = 0;

        // 0 から targetFrame-1 までキー履歴を再適用
        for (int f = 0; f < targetFrame; ++f) {
            count++;
            uint8_t input = (f < (int)replayKeyHistory.size())
                ? replayKeyHistory[f]
                : 0;
            unpackReplayKey(input, key);

            // ゲームロジック実行（被弾処理なし）
            stageData[stageNum].patternFunc();
            enemyControl();
            enemyShotControl();
            enemyShotCalc();
            playerControl();
            playerShotControl();
            playerShotCalc();
            playerShotHit();
            // enemyShotHit();  // ← 復元中は実行しない
            // enemyHit();      // ← 復元中は実行しない
        }

        // うるさいので音を止める
        stopSearchSounds();
    }

    //-----------------------------------------------
    // 1フレーム分のシミュレーション（被弾処理あり）
    //-----------------------------------------------

    // 候補入力を現在の key に反映する
    void applySearchInput(uint8_t input)
    {
        key[KEY_INPUT_NUMPAD4] = (input & REPLAY_KEY_LEFT) ? 1 : 0;
        key[KEY_INPUT_NUMPAD6] = (input & REPLAY_KEY_RIGHT) ? 1 : 0;
        key[KEY_INPUT_NUMPAD8] = (input & REPLAY_KEY_UP) ? 1 : 0;
        key[KEY_INPUT_NUMPAD5] = (input & REPLAY_KEY_DOWN) ? 1 : 0;

        // ショット常時ON、低速常時OFF
        key[KEY_INPUT_V] = 1;
        key[KEY_INPUT_C] = 0;
    }

    // 1フレームだけゲームロジックを実行（被弾判定を含む）
    void simulateOneFrame(uint8_t input)
    {
        count++;
        applySearchInput(input);

        stageData[stageNum].patternFunc();
        enemyControl();
        enemyShotControl();
        enemyShotCalc();
        playerControl();
        playerShotControl();
        playerShotCalc();
        playerShotHit();
        enemyShotHit();   // 自機への被弾判定
        enemyHit();       // 敵本体との接触判定

        stopSearchSounds();
    }

    //-----------------------------------------------
    // 方向の優先順位生成
    //-----------------------------------------------

    // 9方向の候補を「player.x が enemy.x に近づき、かつ player.y が 400 に近づく順」で返す
    std::vector<uint8_t> generateDirectionOrder()
    {
        struct Cand {
            uint8_t input;
            float   score;
        };

        // 9方向の入力を生成（ショット常時ON・低速OFF）
        Cand cand[9] = {
            { makeDirInput(true,  true,  false, false), 0.0f }, // 左上
            { makeDirInput(false, true,  false, false), 0.0f }, // 上
            { makeDirInput(false, true,  true,  false), 0.0f }, // 右上
            { makeDirInput(true,  false, false, false), 0.0f }, // 左
            { makeDirInput(false, false, false, false), 0.0f }, // 停止
            { makeDirInput(false, false, true,  false), 0.0f }, // 右
            { makeDirInput(true,  false, false, true), 0.0f }, // 左下
            { makeDirInput(false, false, false, true), 0.0f }, // 下
            { makeDirInput(false, false, true,  true), 0.0f }, // 右下
        };

        // 目標：X = enemy.x, Y = 400
        float targetX = (float)enemy.x;
        float targetY = (float)enemy.y + 230;

        if (stageData[stageNum].stageId == "DeepSeek85") {
            if (player.x < enemy.x - 60 && player.y > enemy.y) {
                targetX = (float)enemy.x - 65;
                targetY = (float)enemy.y - 60;
            }
            else if (player.x < enemy.x && player.y <= enemy.y) {
                targetX = (float)enemy.x + 60;
                targetY = (float)enemy.y - 60;
            }
            else if (player.x >= enemy.x && player.y <= enemy.y) {
                targetX = (float)enemy.x + 60;
                targetY = (float)enemy.y + 60;
            }
            else {
                targetX = (float)enemy.x - 30;
                targetY = (float)enemy.y + 100;
            }
        }
        else if (stageData[stageNum].stageId == "Gemini115") {
            targetX = (float)enemy.x;
            targetY = 420.0;
        }
        else if (stageData[stageNum].stageId == "ChatGPT115") {
            targetX = (float)enemy.x;
            targetY = (float)enemy.y + 320;
        }
        else if (stageData[stageNum].stageId == "DeepSeek137") {
            targetX = (float)enemy.x;
            targetY = (float)enemy.y + 200;
        }
        else if (stageData[stageNum].stageId == "ChatGPT137") {
            targetX = (float)enemy.x;
            targetY = (float)enemy.y + 200 + count % 100;
        }
        else if (stageData[stageNum].stageId == "Claude137") {
            targetX = (float)enemy.x;
            targetY = (float)enemy.y + 120;
        }
        else if (stageData[stageNum].stageId == "MetaAI160") {
            targetX = (float)enemy.x;
            targetY = 480 - 50;
        }

        // 現在位置との差
        const float dx = targetX - (float)player.x;
        const float dy = targetY - (float)player.y;

        const float invSqrt2 = 1.0f / 1.41421356f;  // 斜め移動の速度補正

        for (auto& c : cand) {
            float vx = 0.0f;
            float vy = 0.0f;

            if (c.input & REPLAY_KEY_LEFT)  vx -= 4.5f;
            if (c.input & REPLAY_KEY_RIGHT) vx += 4.5f;
            if (c.input & REPLAY_KEY_UP)    vy -= 4.5f;
            if (c.input & REPLAY_KEY_DOWN)  vy += 4.5f;

            // 斜め移動の場合は速度を1/√2に補正（playerControl の仕様に合わせる）
            if (vx != 0.0f && vy != 0.0f) {
                vx *= invSqrt2;
                vy *= invSqrt2;
            }

            // スコア = X軸方向の改善量 + Y軸方向の改善量
            //   dx > 0（敵が右）なら vx > 0（右移動）がプラス
            //   dy > 0（Y=400より上）なら vy > 0（下移動）がプラス
            c.score = dx * vx + dy * vy;
        }

        // 停止入力の定義
        uint8_t stopInput = makeDirInput(false, false, false, false);

        // 同じ入力が何フレーム続いているかカウント
        int sameCount = 0;
        if (!replayKeyHistory.empty()) {
            uint8_t lastInp = replayKeyHistory.back();
            for (auto it = replayKeyHistory.rbegin(); it != replayKeyHistory.rend(); ++it) {
                if (*it == lastInp) sameCount++;
                else break;
            }
        }

        // 直前の入力を取得
        uint8_t prevInput = replayKeyHistory.empty()
            ? stopInput
            : replayKeyHistory.back();

        // 目標とのX座標のズレが20以上か判定
        bool isFar = std::abs(targetX - (float)player.x) >= 20.0f;

        // ズレが閾値(20)未満で、かつ同じ入力が6フレーム以上続いている場合のみ強制停止
        bool forceStop = (!isFar && sameCount >= 6);

        // 条件に応じてソート
        std::stable_sort(cand, cand + 9,
            [prevInput, isFar, forceStop, stopInput](const Cand& a, const Cand& b) {
            if (isFar) {
                // ズレが20以上：従来通りターゲットに近づく順（スコア順）
                return a.score > b.score;
            }
            else {
                // ズレが20未満の場合の処理

                // 条件を満たした場合、例外的に「静止」を最優先
                if (forceStop) {
                    if (a.input == stopInput && b.input != stopInput) return true;
                    if (b.input == stopInput && a.input != stopInput) return false;
                }

                // 直前からのキー切り替え数が少ない順
                auto countChange = [](uint8_t in1, uint8_t in2) {
                    uint8_t diff = in1 ^ in2;
                    int count = 0;
                    while (diff != 0) {
                        count += diff & 1;
                        diff >>= 1;
                    }
                    return count;
                };

                int diffA = countChange(a.input, prevInput);
                int diffB = countChange(b.input, prevInput);

                // 切り替え数が同じ場合は、従来のスコアが高い方を優先する
                if (diffA == diffB) {
                    return a.score > b.score;
                }
                return diffA < diffB;
            }
        });

        std::vector<uint8_t> order;
        for (const auto& c : cand) {
            order.push_back(c.input);
        }
        return order;
    }

    //-----------------------------------------------
    // DFS本体
    //-----------------------------------------------

    // 残り探索フレーム数 depthRemaining だけ深さ優先探索する
    // 成功したら true を返し、replayKeyHistory に確定した入力が入る
    bool dfsAutoTas(int depthRemaining)
    {
        if (depthRemaining <= 0) {
            // 指定フレーム数まで無被弾で到達
            return true;
        }

        const int parentLen = (int)replayKeyHistory.size();

        // 親状態で方向の優先順位を計算
        std::vector<uint8_t> order = generateDirectionOrder();

        for (uint8_t input : order) {
            // 毎回、親状態からやり直す
            restoreToFrame(parentLen);

            // 1フレームだけ候補を実行（被弾判定あり）
            simulateOneFrame(input);

            // 探索局面数を1増やす
            s_dfsNodeCount++;

            static int lastDrawTime = 0;
            int now = GetNowCount();
            if (now - lastDrawTime >= 17 * 2) {   // 描画
                pollEmergencyStop();
                renderCurrentScene();
                lastDrawTime = now;
            }

            // クリアしたなら採用して探索終了
            if (isCleared()) {
                replayKeyHistory.push_back(input);
                return true;
            }

            // 被弾したらこの候補は破棄
            if (wasPlayerHit()) {
                continue;
            }

            // 被弾しなかったので仮採用して深く探索
            replayKeyHistory.push_back(input);

            if (dfsAutoTas(depthRemaining - 1)) {
                return true;
            }

            // 失敗したので破棄して次へ
            replayKeyHistory.pop_back();
        }

        return false;
    }

} // namespace

//---------------------------------------------------
// 自動TAS探索を開始する
//   maxDepth : 最大探索フレーム数（0以下の場合はデフォルト1500）
//---------------------------------------------------
void TAS_AutoSearchStart()
{
    if (!g_isAutoTasMode) return;

    // シードの乱択
    gameSeed = GetNowCount();

    // 最初からリプレイを作り直す
    replayKeyHistory.clear();

    // 探索局面数カウンタをリセット
    s_dfsNodeCount = 0;

    bool success = dfsAutoTas(kDefaultMaxDepth);

    if (success) {
        // 成立したリプレイを先頭から再生し直して最終状態にする
        restoreToFrame((int)replayKeyHistory.size());
    }
}