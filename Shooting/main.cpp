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

constexpr int GAME_W = 640;
constexpr int GAME_H = 480;

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ウィンドウモードの初期設定
    SetGraphMode(GAME_W, GAME_H, 32);
    ChangeWindowMode(TRUE);
    SetWindowSize(GAME_W, GAME_H);              // 初期サイズ
    loadWindowSettings();                       // 前回の設定で上書き
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);  // 第2引数TRUEでバックバッファもリサイズに追従

    if (DxLib_Init() == -1) return -1;

    // 仮想画面を作成（この画面に全ゲーム描画を行う）
    int gameScreen = MakeScreen(GAME_W, GAME_H, TRUE);
    SetDrawScreen(gameScreen);

    // スプラッシュ画像の表示
    //int splashHandle = LoadGraph("assets/images/splash.png");
    //if (splashHandle != -1) {
    //    // 描画先を実際の画面（バックバッファ）に切り替え
    //    SetDrawScreen(DX_SCREEN_BACK);
    //    DrawGraph(0, 0, splashHandle, TRUE);   // または DrawExtendGraph
    //    ScreenFlip();                           // 画面に反映
    //    Sleep(1000); // これがないと一瞬しか映らない。もはやスプラッシュ画像は必要ない。
    //    // 描画先を仮想画面に戻す
    //    SetDrawScreen(gameScreen);
    //}
    //else {
    //    MessageBox(NULL, "splash.png が見つかりません", "エラー", MB_OK);
    //}

    fileOpen();
    imgSoundLoad();
    setColor();
    currentBGMHandle = bgm_menu;
    PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
    loadCursorPos();
    
    playerShotHead.prev = &playerShotHead;
    playerShotHead.next = &playerShotHead;
    enemyShotSetHead.prev = &enemyShotSetHead;
    enemyShotSetHead.next = &enemyShotSetHead;

    while (!ProcessMessage() && !ClearDrawScreen()) {
        int frameStart = GetNowCount();

        // キー入力取得（リプレイ再生中はリプレイデータを使う）
        if (StateManager::GetState() == Joutai::Replay) {
            // リプレイ中でもQキーを取得する（物理キーボード）
            if (CheckHitKey(KEY_INPUT_Q)) key[KEY_INPUT_Q] = 1;

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

            // Qキーで即メニューに戻る
            if (key[KEY_INPUT_Q] == 1) {
                StateManager::ChangeState(Joutai::Menu);
            }
        }
        else if (StateManager::GetState() == Joutai::Win || StateManager::GetState() == Joutai::Lose) {
            // ベストタイム更新とリプレイ保存（Win時のみ）
            if (StateManager::GetState() == Joutai::Win && !replayActive) {
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
                        key[KEY_INPUT_NUMPAD4] = 0;
                        key[KEY_INPUT_NUMPAD6] = 0;
                        key[KEY_INPUT_NUMPAD8] = 0;
                        key[KEY_INPUT_NUMPAD5] = 0;
                        key[KEY_INPUT_V] = 1;
                        key[KEY_INPUT_C] = 0;
                        if (!StateManager::ChangeState(Joutai::Replay)) {
                            // リプレイファイルがない場合はメニューへ
                            StateManager::ChangeState(Joutai::Menu);
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
        int elapsed = GetNowCount() - frameStart;
        if (elapsed < 17) WaitTimer(17 - elapsed);
    }

    iniGame();
    fileClose();

    DxLib_End();
    return 0;
}