// main.cpp

#include "DxLib.h"
#include "gv.h"
#include "initial.h"
#include "fps.h"
#include "gameScreen.h"
#include "stageData.h"
#include "menu.h"
#include "imgSoundLoad.h"
#include "player.h"
#include "playerShot.h"
#include "enemy.h"
#include "enemyShot.h"
#include "fileOpenClose.h"
#include "getHitKeyStateAll2.h"
#include "replay.h"
#include "stateManager.h"   // StateManager, Joutai
#include "recordController.h"
#include "masterpieceViewer.h"
#include "tasController.h"


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ウィンドウモードの初期設定
    SetGraphMode(GAME_W, GAME_H, 32);
    ChangeWindowMode(TRUE);
    
    // ウィンドウを非表示で初期化（黒画面を隠す）
    SetWindowVisibleFlag(FALSE);

    if (DxLib_Init() == -1) return -1;
    SetWindowSize(GAME_W, GAME_H);              // デフォルトサイズ
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);  // 第2引数TRUEでバックバッファもリサイズに追従

    // スプラッシュ画像を読み込んで表示
    int splashHandle = LoadGraph("assets/images/splash.jpg");
    if (splashHandle != -1) {
        SetDrawScreen(DX_SCREEN_BACK);   // 実画面のバックバッファに描画
        DrawGraph(0, 0, splashHandle, TRUE);
        ScreenFlip();
    }
    if (!recordingMode && !masterpieceMode) {
        loadWindowSettings();            // ウィンドウ位置・サイズを復元
    }

    // ここでウィンドウを表示する（これまでは非表示だったので一瞬でスプラッシュが現れる）
    HWND hwnd = (HWND)GetMainWindowHandle();
    if (!hwnd || !IsWindowVisible(hwnd)) SetWindowVisibleFlag(TRUE);
    
    // 起動時刻を記録（スプラッシュ最低表示時間のため）
    int startTime = GetNowCount();

    // 通常のロード処理（画像・効果音・メニューBGMなど）
    fileOpen();
    loadCursorPos();                     // カーソル位置を復元（先に読み込む）        
    imgSoundLoad();                      // 画像/効果音/メニューBGM 読込
    stageNum = cursor.page * 100 + cursor.y * 10 + cursor.x;
    
    // ---------- 最低 SPLASH_MIN_TIME 間スプラッシュを維持しつつBGMをプリロード ----------
    int SPLASH_MIN_TIME = recordingMode ? 1500 : 100;   // スプラッシュ最低表示時間[ms]
    int elapsed = GetNowCount() - startTime;
    if (elapsed < SPLASH_MIN_TIME && splashHandle != -1) {
        int waitEnd = startTime + SPLASH_MIN_TIME;
        int preloadStage = stageNum;                    // stageNum からロード開始
        int loadedCount = 0;                            // 読み込んだステージ数
        const int totalStages = (int)stageData.size();

        while (GetNowCount() < waitEnd) {
            if (ProcessMessage() == -1) break;

            // スプラッシュ描画
            SetDrawScreen(DX_SCREEN_BACK);
            DrawGraph(0, 0, splashHandle, TRUE);
            ScreenFlip();

            // 未読込のステージがあれば1つロード
            if (loadedCount < totalStages) {
                loadStageBGM(preloadStage);
                ++loadedCount;
                // 次のステージへ（循環）
                preloadStage = (preloadStage + 1) % totalStages;
            }

            WaitTimer(17);   // CPU負荷軽減 & 約60fps維持
        }
    }

    // スプラッシュ画像を破棄
    if (splashHandle != -1) DeleteGraph(splashHandle);

    if (masterpieceMode) {
        masterpieceMain();
        return 0;
    }

    // ---------- ゲーム画面の準備 ----------
    int gameScreen = MakeScreen(GAME_W, GAME_H, TRUE);
    SetDrawScreen(gameScreen);
        
    // メニューBGMを再生（スプラッシュ終了後）
    StateManager::ChangeState(Joutai::Menu);
    
    playerShotHead.prev = &playerShotHead;
    playerShotHead.next = &playerShotHead;
    enemyShotSetHead.prev = &enemyShotSetHead;
    enemyShotSetHead.next = &enemyShotSetHead;    

    // 録画モード自動化ステートマシン
    RecordController recorder;

    while (!ProcessMessage() && !ClearDrawScreen()) {
        int frameStart = GetNowCount();

        // ---- 録画モード時のキー入力 ----
        if (recordingMode) {
            recorder.Update(key);
        }
        // ---- 通常モード時のキー入力 ----
        else {
            // キー入力取得（リプレイ再生中はリプレイデータを使う）
            if (StateManager::GetState() == Joutai::Replay) {
                // リプレイ中でもQキーを取得する（物理キーボード）
                if (CheckHitKey(KEY_INPUT_Q)) key[KEY_INPUT_Q] = 1;
				// ジョイスティック用
                int pad = GetJoypadInputState(DX_INPUT_PAD1);
                key[KEY_INPUT_Q] |= (pad & PAD_INPUT_4) ? 1 : 0;

                if (!updateReplayInput()) {
                    // リプレイデータ終了 → メニューへ
                    StateManager::ChangeState(Joutai::Menu);
                    continue;
                }
            }
            else {
                getHitKeyStateAll_2(key);
                // 通常ゲーム中はキー履歴を記録
                if (StateManager::GetState() == Joutai::Game) {
                    replayKeyHistory.push_back(packReplayKey(key));
                }
            }
        }

        // M キーで無敵モードの ON/OFF 切り替え
        if (key[KEY_INPUT_M] == 1) {
            isMuteki = !isMuteki;
        }
        
        // 状態に応じて処理を分岐
        if (StateManager::GetState() == Joutai::Menu) {
            moveCursor();
            menuDraw();
            if (key[KEY_INPUT_Q] == 1) {
                saveWindowSettings();
                break;
            }
        }
        else if (StateManager::GetState() == Joutai::Game || StateManager::GetState() == Joutai::Replay) {
            stageData[stageNum].patternFunc();
            enemyControl();
            enemyShotControl();
            enemyShotCalc();
            playerControl();
            playerShotControl();
            playerShotCalc();
            playerShotHit();
            enemyShotHit();
            enemyHit();

            backGround();
            playerDisp();
            playerShotDisp();
            enemyDisp();
            enemyShotDisp();
            drawSidePanel();
            foreGround();

            // Qキーで即メニューに戻る
            if (key[KEY_INPUT_Q] == 1) {
                StateManager::ChangeState(Joutai::Menu);
            }

            // TAS用に最大countを更新
            if (g_isTasMode) {
                TAS_UpdateMaxCount(count);
            }
        }
        else if (StateManager::GetState() == Joutai::Win || StateManager::GetState() == Joutai::Lose) {
            if (replayActive || StateManager::GetState() == Joutai::Win || !TAS_OnLoseState()) {
                // ベストタイム更新とリプレイ保存（Win時のみ）
                if (StateManager::GetState() == Joutai::Win && !replayActive && !isMuteki) {
                    unsigned int clearTime = count;
                    if (clearTime < stageData[stageNum].bestTime) {
                        stageData[stageNum].bestTime = clearTime;
                        saveReplay(stageNum);
                        fileClose();  // bestTime.json 更新
                    }
                }

                backGround();
                playerDisp();
                playerShotDisp();
                enemyDisp();
                enemyShotDisp();
                drawSidePanel();
                foreGround();
                drawGameOverlay();
                count--;

                if (key[KEY_INPUT_Q] == 1) {
                    StateManager::ChangeState(Joutai::Menu);
                }
                else if (key[KEY_INPUT_V] == 1) {
                    if (!replayActive) {
                        StateManager::ChangeState(Joutai::Game);   // リトライ（通常ゲーム）
                    }
                }
                else if (key[KEY_INPUT_R] == 1) {
                    if (replayActive) {
                        StateManager::ChangeState(Joutai::Replay); // リプレイ再開
                    }
                }
                else if (key[KEY_INPUT_N] == 1) {
                    int nextStage = stageNum + 1;
                    if (nextStage < (int)stageData.size()) {
                        stageNum = nextStage;
                        cursor.page = stageNum / 100;
                        cursor.y = (stageNum % 100) / 10;
                        cursor.x = stageNum % 10;

                        if (!replayActive) {
                            StateManager::ChangeState(Joutai::Game);   // 次のステージへ（通常）
                        }
                        else {
                            if (!StateManager::ChangeState(Joutai::Replay)) {
                                // リプレイファイルがない場合はメニューへ
                                StateManager::ChangeState(Joutai::Menu);
                            }
                        }
                    }
                }
            }
        }

        fpsTimeFunction();
        count++;

        // リサイズ対応の合成処理
        SetDrawScreen(DX_SCREEN_BACK);
        ClearDrawScreen();

        // GetWindowSize ではなく GetDrawScreenSize でバックバッファの実サイズを取得
        int winW, winH;
        GetDrawScreenSize(&winW, &winH);

        double scaleX = (double)winW / GAME_W;
        double scaleY = (double)winH / GAME_H;
        double scale = (scaleX < scaleY) ? scaleX : scaleY;
        int drawW = (int)(GAME_W * scale);
        int drawH = (int)(GAME_H * scale);
        int offsetX = (winW - drawW) / 2;
        int offsetY = (winH - drawH) / 2;

        DrawExtendGraph(offsetX, offsetY, offsetX + drawW, offsetY + drawH, gameScreen, TRUE);
        ScreenFlip();

        SetDrawScreen(gameScreen);

        // FPS 調整
        int MS = 17;
        int elapsed = GetNowCount() - frameStart;
        if (elapsed < MS) WaitTimer(MS - elapsed);
    }

    iniGame();
    fileClose();

    DxLib_End();
    return 0;
}