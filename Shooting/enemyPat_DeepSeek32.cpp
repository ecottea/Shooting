// enemyPat_Tmp.cpp
// ボンバーマン弾幕：ボムグリッド・クロスチェイン
// ※ 爆弾を黒い大玉として可視化するよう修正

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -------------------------------------------------------------------
//  グリッド爆弾パターン管理用の静的関数
// -------------------------------------------------------------------
static void BombermanGrid(sEnemyShotSet* pEnemyShotSet)
{
    // グリッド最大サイズ
    static const int MAX_COL = 16;
    static const int MAX_ROW = 16;

    // ファイルスコープの static 変数（初回のみ初期化）
    static int   g_gridCols = 0, g_gridRows = 0;
    static double g_cellW = 0.0, g_cellH = 0.0;
    static int   g_bombTimer[MAX_COL][MAX_ROW];        // -1:空, 0:爆発待ち, >0:タイマー
    static sEnemyShot* g_bombShot[MAX_COL][MAX_ROW];   // 爆弾の弾オブジェクト (NULLならなし)

    if (pEnemyShotSet->count == 0) {
        // グリッド設定
        g_gridCols = pEnemyShotSet->param_i[0];
        g_gridRows = pEnemyShotSet->param_i[1];
        if (g_gridCols < 2 || g_gridCols > MAX_COL) g_gridCols = 9;
        if (g_gridRows < 2 || g_gridRows > MAX_ROW) g_gridRows = 7;
        g_cellW = 480.0 / (g_gridCols - 1);
        g_cellH = 480.0 / (g_gridRows - 1);

        for (int i = 0; i < g_gridCols; ++i) {
            for (int j = 0; j < g_gridRows; ++j) {
                g_bombTimer[i][j] = -1;
                g_bombShot[i][j] = nullptr;
            }
        }

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ---------- 爆弾の設置 ----------
    if (pEnemyShotSet->count % 30 == 0) {
        // 既存の爆弾リスト
        int bombList[MAX_COL * MAX_ROW][2];
        int bombCount = 0;
        for (int i = 0; i < g_gridCols; ++i)
            for (int j = 0; j < g_gridRows; ++j)
                if (g_bombTimer[i][j] > 0) {
                    bombList[bombCount][0] = i;
                    bombList[bombCount][1] = j;
                    bombCount++;
                }

		if (CheckSoundMem(sound_enemyShot_light) == 1)
			StopSoundMem(sound_enemyShot_light);
		PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
		int tries = 0;
		while (tries < 50) {
			int col = GetRand(g_gridCols - 1);
			int row = GetRand(g_gridRows - 1);

            double cx = col * g_cellW;
            double cy = row * g_cellH;
            double dx = cx - player.x;
            double dy = cy - player.y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist <= 30.0) {
                tries++;
                continue;   // 次の候補を試す
            }

			if (g_bombTimer[col][row] == -1) {
				g_bombTimer[col][row] = 180;
				break;
			}
			tries++;
		}
    }

    // ---------- 新しく設置された爆弾に弾オブジェクトを生成 ----------
    for (int i = 0; i < g_gridCols; ++i) {
        for (int j = 0; j < g_gridRows; ++j) {
            if (g_bombTimer[i][j] > 0 && g_bombShot[i][j] == nullptr) {
                // 黒い大玉を生成
                sEnemyShot* pBomb = new sEnemyShot;
                pBomb->x = i * g_cellW;
                pBomb->y = j * g_cellH;
                pBomb->muki = 0.0;
                pBomb->speed = 0.0;
                pBomb->kind = img_enemyShotLargeBall[7]; // 7:黒

                // 双方向リンクに追加
                pBomb->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pBomb->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pBomb;
                pEnemyShotSet->pEnemyShotHead->prev = pBomb;

                g_bombShot[i][j] = pBomb;
            }
        }
    }

    // ---------- タイマーの減算 ----------
    for (int i = 0; i < g_gridCols; ++i)
        for (int j = 0; j < g_gridRows; ++j)
            if (g_bombTimer[i][j] > 0)
                g_bombTimer[i][j]--;

    // ---------- 爆発処理（連鎖対応のキュー） ----------
    const int QMAX = g_gridCols * g_gridRows;
    std::vector<int> qx(QMAX), qy(QMAX);
    int head = 0, tail = 0;

    for (int i = 0; i < g_gridCols; ++i)
        for (int j = 0; j < g_gridRows; ++j)
            if (g_bombTimer[i][j] == 0) {
                qx[tail] = i;
                qy[tail] = j;
                tail++;
            }

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (head < tail) {
        int col = qx[head];
        int row = qy[head];
        head++;

        // 爆弾オブジェクトがあればリストから削除
        if (g_bombShot[col][row] != nullptr) {
            sEnemyShot* pBomb = g_bombShot[col][row];
            pBomb->prev->next = pBomb->next;
            pBomb->next->prev = pBomb->prev;
            delete pBomb;
            g_bombShot[col][row] = nullptr;
        }

        // 爆発の効果音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double sx = col * g_cellW;
        double sy = row * g_cellH;

        // ---- 十字方向へ弾を生成 ----
        for (int dir = 0; dir < 4; ++dir) {
            double angle = atan2((double)dy[dir], (double)dx[dir]);
            const double STEP = 20.0;

            double maxDist;
            if (dx[dir] != 0)
                maxDist = (dx[dir] > 0) ? (480.0 - sx) : sx;
            else
                maxDist = (dy[dir] > 0) ? (480.0 - sy) : sy;

            for (double d = STEP; d < maxDist; d += STEP) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = sx + dx[dir] * d;
                pShot->y = sy + dy[dir] * d;
                pShot->muki = angle;
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotLargeBall[8]; // オレンジ中玉

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // ---- 十字方向の他の爆弾を誘爆 ----
        for (int dir = 0; dir < 4; ++dir) {
            int c = col + dx[dir];
            int r = row + dy[dir];
            while (c >= 0 && c < g_gridCols && r >= 0 && r < g_gridRows) {
                if (g_bombTimer[c][r] > 0) {
                    g_bombTimer[c][r] = 0; // 即時起爆
                    qx[tail] = c;
                    qy[tail] = r;
                    tail++;
                }
                c += dx[dir];
                r += dy[dir];
            }
        }

        // 爆発済みマーク
        g_bombTimer[col][row] = -1;
    }

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        //if (pEnemyShot->count >= 600) pEnemyShot->x = 9999;

        pEnemyShot = pEnemyShot->next;
    }
}

// -------------------------------------------------------------------
//  敵本体パターン（ボンバーマン弾幕）
// -------------------------------------------------------------------
void EnemyPat_Bomberman_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // グリッド爆弾パターンを起動
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BombermanGrid;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // グリッドサイズ（交点の列数・行数）
        pSet->param_i[0] = 12;
        pSet->param_i[1] = 12;

        // ダミーヘッド
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルの ShotSet リストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵本体はゆっくり左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }
}