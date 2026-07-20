// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ライフゲーム弾幕パターン関数
// ============================================================
static void Shot_GameOfLife(sEnemyShotSet* pSet)
{
    // 24x24のグリッド (画面480x480を20px単位で区切る)
    static int grid[24][24] = { 0 };
    static int next_grid[24][24] = { 0 };

    // 初期化処理 (count == 0 の時に1度だけ実行)
    if (pSet->count == 0) {
        for (int i = 0; i < 24; i++) for (int j = 0; j < 24; j++) grid[i][j] = next_grid[i][j] = 0;

        // 初期配置: 面白い変化を起こす「Rペントミノ」と「グライダー」を配置
        // Rペントミノ (中央付近)
        grid[15][11] = 1; grid[15][12] = 1;
        grid[16][10] = 1; grid[16][11] = 1;
        grid[17][11] = 1;

        // グライダー 1 (左上から右下へ)
        grid[5][6] = 1; grid[6][7] = 1; grid[7][5] = 1; grid[7][6] = 1; grid[7][7] = 1;

        // グライダー 2 (右上から左下へ)
        grid[5][18] = 1; grid[6][17] = 1; grid[7][19] = 1; grid[7][18] = 1; grid[7][17] = 1;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 既存の弾の移動処理 (メインルーチンでやらない場合の補完、またはサンプルに準拠)
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->count >= 60 && pShot->speed == 0.0) pShot->margin = -9999;

        pShot = pShot->next;
    }

    // 15フレームごとに世代交代を計算 (速すぎるとカオスになりすぎるため間引く)
    if (pSet->count % 60 == 0) {

        // --------------------------------------------------------
        // 1. プレイヤー介入ギミック: 自弾がセルに当たると強制「死亡」
        // --------------------------------------------------------
        sPlayerShot* pPShot = playerShotHead.next;
        while (pPShot != &playerShotHead) {
            int gx = (int)(pPShot->x / 20.0);
            int gy = (int)(pPShot->y / 20.0);

            if (gx >= 0 && gx < 24 && gy >= 0 && gy < 24) {
                if (grid[gy][gx] == 1) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 強制死亡: 8方向に爆発弾を生成
                    for (int i = 0; i < 8; i++) {
                        sEnemyShot* shot = new sEnemyShot;
                        shot->x = gx * 20.0 + 10.0;
                        shot->y = gy * 20.0 + 10.0;
                        shot->muki = i * (DX_PI / 4.0);
                        shot->speed = 4.0;
                        shot->kind = img_enemyShotSmallBall[0]; // 0:赤
                        shot->prev = pSet->pEnemyShotHead->prev;
                        shot->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = shot;
                        pSet->pEnemyShotHead->prev = shot;
                    }
                    grid[gy][gx] = 0; // 盤面からも消す
                }
            }
            pPShot = pPShot->next;
        }

        // --------------------------------------------------------
        // 2. ライフゲームの次世代を計算
        // --------------------------------------------------------
        int alive_count = 0;
        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 24; x++) {
                int neighbors = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        // 画面端での折り返しはせず、範囲内のみカウント
                        if (nx >= 0 && nx < 24 && ny >= 0 && ny < 24) {
                            neighbors += grid[ny][nx];
                        }
                    }
                }

                if (grid[y][x] == 1) {
                    if (neighbors == 2 || neighbors == 3) {
                        next_grid[y][x] = 1; // 生存
                    }
                    else {
                        next_grid[y][x] = 0; // 死亡 (過疎または過密)
                    }
                }
                else {
                    if (neighbors == 3) {
                        next_grid[y][x] = 1; // 誕生
                    }
                    else {
                        next_grid[y][x] = 0;
                    }
                }

                if (next_grid[y][x] == 1) alive_count++;
            }
        }

        // --------------------------------------------------------
        // 3. 状態変化に応じて弾を生成し、グリッドを更新
        // --------------------------------------------------------
        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 24; x++) {
                double cx = x * 20.0 + 10.0; // セルの中心X
                double cy = y * 20.0 + 10.0; // セルの中心Y

                if (grid[y][x] == 0 && next_grid[y][x] == 1) {
                    // 【誕生】: 黄色の中玉をランダム方向に中速発射
                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = cx;
                    shot->y = cy;
                    shot->muki = GetRand(360) / 180.0 * DX_PI; // GetRand(360)は0〜360を返す
                    shot->speed = 3.0 + GetRand(200) / 100.0;  // 3.0 〜 5.0
                    shot->kind = img_enemyShotMediumBall[1];   // 1:黄

                    shot->prev = pSet->pEnemyShotHead->prev;
                    shot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = shot;
                    pSet->pEnemyShotHead->prev = shot;
                }
                else if (grid[y][x] == 1 && next_grid[y][x] == 0) {
                    // 【死亡】: 赤色の小玉を8方向に高速拡散
                    for (int i = 0; i < 8; i++) {
                        sEnemyShot* shot = new sEnemyShot;
                        shot->x = cx;
                        shot->y = cy;
                        shot->muki = i * (DX_PI / 4.0);
                        shot->speed = 4.0;
                        shot->kind = img_enemyShotSmallBall[0]; // 0:赤

                        shot->prev = pSet->pEnemyShotHead->prev;
                        shot->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = shot;
                        pSet->pEnemyShotHead->prev = shot;
                    }
                }
                else if (grid[y][x] == 1 && next_grid[y][x] == 1) {
                    // 【生存】: シアンの鱗弾を自機狙いで低速発射
                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = cx;
                    shot->y = cy;
                    shot->muki = atan2(player.y - cy, player.x - cx);
                    shot->speed = 1.5;
                    shot->kind = img_enemyShotScale[3];         // 3:シアン

                    shot->prev = pSet->pEnemyShotHead->prev;
                    shot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = shot;
                    pSet->pEnemyShotHead->prev = shot;
                }

                // 次世代の状態を現在のグリッドに反映
                grid[y][x] = next_grid[y][x];

                if (grid[y][x]) {
                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = cx;
                    shot->y = cy;
                    shot->muki = 0.0;
                    shot->speed = 0.0;
                    shot->kind = img_enemyShotMediumBall[6];

                    shot->prev = pSet->pEnemyShotHead->prev;
                    shot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = shot;
                    pSet->pEnemyShotHead->prev = shot;
                }
            }
        }

        // --------------------------------------------------------
        // 4. 盤面の沈静化防止 (生きているセルが少なすぎたら新規投入)
        // --------------------------------------------------------
        if (alive_count < 15) {
            int gx = GetRand(20) + 2;
            int gy = GetRand(10) + 2;
            // 新しいグライダーをランダムな位置に生成
            grid[gy][gx + 1] = 1;
            grid[gy + 1][gx + 2] = 1;
            grid[gy + 2][gx] = 1; grid[gy + 2][gx + 1] = 1; grid[gy + 2][gx + 2] = 1;
        }
    }
}


// ============================================================
// 敵本体のパターン (ライフゲーム弾幕)
// ============================================================
void EnemyPat_LifeGame_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 複雑なパターンなのでHPを少し多めに
        muki = 1;

        // ライフゲーム管理用のショットセットを1つだけ作成
        sEnemyShotSet* pGoLSet = new sEnemyShotSet;
        pGoLSet->count = 0;
        pGoLSet->patternFunc = Shot_GameOfLife;
        pGoLSet->x = enemy.x;
        pGoLSet->y = enemy.y;
        pGoLSet->muki = 0;
        pGoLSet->kind = 0;

        // リストのヘッド初期化
        pGoLSet->pEnemyShotHead = new sEnemyShot;
        pGoLSet->pEnemyShotHead->prev = pGoLSet->pEnemyShotHead;
        pGoLSet->pEnemyShotHead->next = pGoLSet->pEnemyShotHead;

        // グローバルリストに追加
        pGoLSet->prev = enemyShotSetHead.prev;
        pGoLSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pGoLSet;
        enemyShotSetHead.prev = pGoLSet;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // ショットセットの基準位置も同期 (必要に応じて)
        //if (pGoLSet != nullptr) {
        //    pGoLSet->x = enemy.x;
        //    pGoLSet->y = enemy.y;
        //}
    }
}