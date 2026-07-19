#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"

void EnemyPat_Unavoidable_ChatGPT()
{
    // 再発動を防ぐための静的フラグ
    static bool pauseDone = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        pauseDone = false;
    }

    if (count == 120 && !pauseDone) {
        pauseDone = true;

        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int pauseImg = LoadGraph("assets/images/ChatGPT33.png");
        if (pauseImg == -1) return;  // 読み込み失敗時は何もせず抜ける

        int imgW, imgH;
        GetGraphSize(pauseImg, &imgW, &imgH);
        int scrW, scrH;
        GetScreenState(&scrW, &scrH, NULL);
        int drawX = (scrW - imgW) / 2;
        int drawY = (scrH - imgH) / 2;

        // ★ 現在の描画先を退避し、バックバッファに切り替え
        int prevDrawScreen = GetDrawScreen();
        SetDrawScreen(DX_SCREEN_BACK);

        while (true) {
            if (ProcessMessage() != 0) break;
            if (CheckHitKey(KEY_INPUT_Q)) break;

            DrawGraph(drawX, drawY, pauseImg, TRUE);
            ScreenFlip();                          // 実画面に反映
            WaitTimer(17);
        }

        // ★ 元の描画先に戻す（gameScreen）
        SetDrawScreen(prevDrawScreen);

        DeleteGraph(pauseImg);
    }
}