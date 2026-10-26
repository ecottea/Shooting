#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// マスゲーム弾幕「一糸乱れぬ大行進」
// 演者は小玉で表現し、隊列の四隅には目印として白い中玉を置く。
// count / pEnemyShotSet->count / pEnemyShot->count の加算と、画面外弾の削除はメインルーチンに任せる。

static void PlayMarchSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

static void AddMarchPerformer(sEnemyShotSet* pEnemyShotSet, int row, int col)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = pEnemyShotSet->x;
    pEnemyShot->y = pEnemyShotSet->y;
    pEnemyShot->muki = 0.0;
    pEnemyShot->speed = 0.0;
    
    const bool isMarker =
        (row == 0 || row == 16) && (col == 0 || col == 16);

    if (isMarker) {
        // 隊列の四隅を中玉で示し、格子の形が見やすいようにする。
        pEnemyShot->kind = img_enemyShotMediumBall[6];
    }
    else {
        // 小玉を演者に見立て、行ごとにシアンと青を切り替える。
        // 中央の行・列は白にして、隊列の軸を分かりやすくする。
        int color = (row % 2 == 0) ? 3 : 4;
        if (row == 8 || col == 8) color = 6;
        pEnemyShot->kind = img_enemyShotSmallBall[color];
    }

    pEnemyShot->param_i[0] = 0; // 0:隊列演技中、1:プレイヤー方向へ発射済み
    pEnemyShot->param_i[1] = row;
    pEnemyShot->param_i[2] = col;
    pEnemyShot->param_i[3] = isMarker ? 1 : 0;

    // 隊列内の相対位置と、出現地点を保存する。
    pEnemyShot->param_d[0] = (col - 8) * 24.0;
    pEnemyShot->param_d[1] = (row - 8) * 20.0;
    pEnemyShot->param_d[2] = pEnemyShotSet->x;
    pEnemyShot->param_d[3] = pEnemyShotSet->y;
    pEnemyShot->margin = 240;

    // リンクリスト末尾へ追加する。
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void MarchFormation(sEnemyShotSet* pEnemyShotSet)
{
    // 新しい弾幕セットの初回だけ、9列×5行の隊列を作る。
    if (pEnemyShotSet->count == 0) {
        PlayMarchSound(sound_enemyCharge);
        for (int row = 0; row < 17; ++row) {
            for (int col = 0; col < 17; ++col) {
                AddMarchPerformer(pEnemyShotSet, row, col);
            }
        }
    }

    const int timer = pEnemyShotSet->count;

    if (timer == 112) PlayMarchSound(sound_enemyShot_medium);
    if (timer == 270) PlayMarchSound(sound_enemyShot_extreme);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 発射後は、メインルーチンが画面外で削除するまで直進させる。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
            continue;
        }

        const int row = pShot->param_i[1];
        const int col = pShot->param_i[2];
        const double localX = pShot->param_d[0];
        const double localY = pShot->param_d[1];
        const double startX = pShot->param_d[2];
        const double startY = pShot->param_d[3];
        const double baseX = startX + localX;
        const double baseY = 174.0+100 + localY;

        if (timer < 270) {
            if (timer < 48) {
                // ボスの位置から各自の定位置へ滑らかに移動し、格子を組み上げる。
                double t = (timer + 1.0) / 48.0;
                if (t > 1.0) t = 1.0;
                const double eased = t * t * (3.0 - 2.0 * t);
                pShot->x = startX + (baseX - startX) * eased;
                pShot->y = startY + (baseY - startY) * eased;
            }
            else if (timer < 112) {
                // 全員が呼吸を合わせ、格子を保ったまま左右へ一斉移動する。
                const double phase = (timer - 48) * 2.0 * DX_PI / 64.0;
                pShot->x = baseX + 58.0 * sin(phase);
                pShot->y = baseY + 3.0 * sin(phase * 2.0);
            }
            else if (timer < 176) {
                // 行ごとに位相をずらし、波が隊列を横切るように動かす。
                const double phase = (timer - 112) * 2.0 * DX_PI / 64.0;
                pShot->x = baseX + 22.0 * sin(phase + row * DX_PI / 2.0);
                pShot->y = baseY + 8.0 * sin(phase + col * DX_PI / 4.0);
            }
            else if (timer < 224) {
                // 偶数行と奇数行が逆方向へ交互に前進・後退する。
                const double phase = (timer - 176) * 2.0 * DX_PI / 48.0;
                const double sign = (row % 2 == 0) ? 1.0 : -1.0;
                pShot->x = baseX + sign * 27.0 * sin(phase);
                pShot->y = baseY + sign * 7.0 * (1.0 - cos(phase));
            }
            else {
                // 最後は列の間隔を広げ、左右に開く扇状の陣形へ展開する。
                double open = (timer - 224 + 1.0) / 46.0;
                if (open > 1.0) open = 1.0;
                const double scaleX = 1.0 + 0.75 * open;
                const double scaleY = 1.0 + 0.20 * open;
                pShot->x = startX + localX * scaleX;
                pShot->y = 174.0+100 + localY * scaleY + fabs(localX) * 0.20 * open;
            }
        }
        else {
            // 展開を終えた瞬間、各演者が現在のプレイヤー位置を狙って一斉発射。
            // 各弾の発射済みフラグで、速度・方向の設定は一度だけ行う。
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = (pShot->param_i[3] == 1) ? 2.5 : 2.8;
            pShot->param_i[0] = 1;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体の弾幕パターン
void EnemyPat_MassGame_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200; // 変更しない
    }
    else {
        // ボスは中央付近をゆっくり往復し、演技の中心を印象付ける。
        enemy.x = 240.0 + 42.0 * sin((count - 1) * 2.0 * DX_PI / 240.0);
        enemy.y = 45.0 + 4.0 * sin((count - 1) * 2.0 * DX_PI / 320.0);
    }

    // 約5.5秒ごとに新しい演技を開始する。前の演技の弾はメイン側の管理に任せる。
    if (count % 330 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = MarchFormation;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
