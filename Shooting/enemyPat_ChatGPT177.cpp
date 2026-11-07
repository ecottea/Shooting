// ============================================================
// 弾幕：三平方の定理「ピタゴラスの証明」
//
// 直角三角形の各辺に正方形を作り、短辺側の二つの正方形を
// 埋める弾を斜辺側の正方形へ移動させることで、面積の等しさを表現する。
// 使用素材：SmallBall（面積タイル）、MediumBall（図形の輪郭）
// 色：赤（短辺1）、青（短辺2）、黄（斜辺）、白（三角形・直角マーク）
// ============================================================

#include "gv.h"
#include "DxLib.h"
#include <cmath>

namespace {
    constexpr double PY_PI = 3.14159265358979323846;
    constexpr double PY_SCALE = 1.35;

    constexpr int PY_RED = 0;
    constexpr int PY_YELLOW = 1;
    constexpr int PY_BLUE = 4;
    constexpr int PY_WHITE = 6;

    constexpr int PY_ROLE_LINE = 0;
    constexpr int PY_ROLE_TILE = 1;

    // 弾を弾セットのリスト末尾に追加する。
    // count類のインクリメントはメインルーチン側に任せる。
    static sEnemyShot* PythAddShot(sEnemyShotSet* pSet,
        double x, double y, int kind, int role)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->kind = kind;
        pShot->speed = 0.0;
        pShot->param_i[0] = role;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        return pShot;
    }

    // 線分を中玉の点列で描く。
    static void PythAddLine(sEnemyShotSet* pSet,
        double x1, double y1, double x2, double y2, int color)
    {
        const double dx = x2 - x1;
        const double dy = y2 - y1;
        const double length = std::sqrt(dx * dx + dy * dy);
        int steps = static_cast<int>(length / (8.0 * PY_SCALE));
        if (steps < 1) steps = 1;

        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / steps;
            PythAddShot(pSet,
                x1 + dx * t,
                y1 + dy * t,
                img_enemyShotMediumBall[color],
                PY_ROLE_LINE);
        }
    }

    // 面積タイルとなる小玉を追加する。
    // param_d[0..3] に移動元・移動先、param_d[4] に移動時の弧の向きを保存する。
    static void PythAddTile(sEnemyShotSet* pSet,
        double srcX, double srcY, double dstX, double dstY,
        int color, int index, double arcSign)
    {
        sEnemyShot* pShot = PythAddShot(
            pSet, srcX, srcY, img_enemyShotSmallBall[color], PY_ROLE_TILE);

        pShot->param_i[1] = index;
        pShot->param_i[2] = color;
        pShot->param_d[0] = srcX;
        pShot->param_d[1] = srcY;
        pShot->param_d[2] = dstX;
        pShot->param_d[3] = dstY;
        pShot->param_d[4] = arcSign;
    }

    // 三角形と三つの正方形、および面積タイルを初期配置する。
    static void PythCreateDiagram(sEnemyShotSet* pSet)
    {
        // 直角三角形の頂点。辺の比は 3:4:5。
        // A が直角、AB が赤、AC が青、BC が黄。
        const double ox = pSet->x;
        const double oy = pSet->y;

        const double ax = ox - 70.0 * PY_SCALE;
        const double ay = oy + 255.0 * PY_SCALE;
        const double bx = ox - 70.0 * PY_SCALE;
        const double by = oy + 165.0 * PY_SCALE;
        const double cx = ox + 50.0 * PY_SCALE;
        const double cy = oy + 255.0 * PY_SCALE;

        // 三角形の三辺。
        PythAddLine(pSet, ax, ay, bx, by, PY_RED);
        PythAddLine(pSet, ax, ay, cx, cy, PY_BLUE);
        PythAddLine(pSet, bx, by, cx, cy, PY_YELLOW);

        // AB（赤）の外側に作る正方形。共有辺 AB 以外の三辺を描く。
        const double r1x = ax - 90.0 * PY_SCALE, r1y = ay;
        const double r2x = bx - 90.0 * PY_SCALE, r2y = by;
        PythAddLine(pSet, ax, ay, r1x, r1y, PY_RED);
        PythAddLine(pSet, r1x, r1y, r2x, r2y, PY_RED);
        PythAddLine(pSet, r2x, r2y, bx, by, PY_RED);

        // AC（青）の外側に作る正方形。共有辺 AC 以外の三辺を描く。
        const double b1x = cx, b1y = cy + 120.0 * PY_SCALE;
        const double b2x = ax, b2y = ay + 120.0 * PY_SCALE;
        PythAddLine(pSet, cx, cy, b1x, b1y, PY_BLUE);
        PythAddLine(pSet, b1x, b1y, b2x, b2y, PY_BLUE);
        PythAddLine(pSet, b2x, b2y, ax, ay, PY_BLUE);

        // BC（黄）の外側に作る正方形。
        // BC=(120,90)、外向きの垂直ベクトル=(90,-120)。
        const double y1x = cx + 90.0 * PY_SCALE, y1y = cy - 120.0 * PY_SCALE;
        const double y2x = bx + 90.0 * PY_SCALE, y2y = by - 120.0 * PY_SCALE;
        PythAddLine(pSet, cx, cy, y1x, y1y, PY_YELLOW);
        PythAddLine(pSet, y1x, y1y, y2x, y2y, PY_YELLOW);
        PythAddLine(pSet, y2x, y2y, bx, by, PY_YELLOW);

        // 直角マーク。
        const double rx1 = ax, ry1 = ay - 15.0 * PY_SCALE;
        const double rx2 = ax + 15.0 * PY_SCALE, ry2 = ay - 15.0 * PY_SCALE;
        const double rx3 = ax + 15.0 * PY_SCALE, ry3 = ay;
        PythAddLine(pSet, rx1, ry1, rx2, ry2, PY_WHITE);
        PythAddLine(pSet, rx2, ry2, rx3, ry3, PY_WHITE);

        // 斜辺側の正方形に並べる 10×10 の移動先セル。
        // B から BC 方向と、正方形の外向き方向へ、それぞれ等間隔に配置する。
        // 先に赤の 6×6 セルを左上側へ割り当て、残り 64 セルを青に割り当てる。
        const double hypU_x = cx - bx;
        const double hypU_y = cy - by;
        const double hypV_x = 90.0 * PY_SCALE;
        const double hypV_y = -120.0 * PY_SCALE;

        int tileIndex = 0;

        // 赤の正方形：6×6 個のタイルを、斜辺側正方形の 6×6 領域へ移す。
        for (int gy = 0; gy < 6; ++gy) {
            for (int gx = 0; gx < 6; ++gx) {
                const double srcX = bx
                    + (gy + 0.5) / 6.0 * (ax - bx)
                    + (gx + 0.5) / 6.0 * (-90.0 * PY_SCALE);
                const double srcY = by
                    + (gy + 0.5) / 6.0 * (ay - by);

                const double dstX = bx
                    + (gx + 0.5) / 10.0 * hypU_x
                    + (gy + 0.5) / 10.0 * hypV_x;
                const double dstY = by
                    + (gx + 0.5) / 10.0 * hypU_y
                    + (gy + 0.5) / 10.0 * hypV_y;

                PythAddTile(pSet, srcX, srcY, dstX, dstY,
                    PY_RED, tileIndex++, 1.0);
            }
        }

        // 青の正方形：8×8 個のタイルを、赤の領域以外の 64 セルへ移す。
        int blueIndex = 0;
        for (int ty = 0; ty < 10; ++ty) {
            for (int tx = 0; tx < 10; ++tx) {
                if (tx < 6 && ty < 6) continue;

                const int sxIndex = blueIndex % 8;
                const int syIndex = blueIndex / 8;
                const double srcX = ax
                    + (sxIndex + 0.5) / 8.0 * (cx - ax);
                const double srcY = ay
                    + (syIndex + 0.5) / 8.0 * (120.0 * PY_SCALE);

                const double dstX = bx
                    + (tx + 0.5) / 10.0 * hypU_x
                    + (ty + 0.5) / 10.0 * hypV_x;
                const double dstY = by
                    + (tx + 0.5) / 10.0 * hypU_y
                    + (ty + 0.5) / 10.0 * hypV_y;

                PythAddTile(pSet, srcX, srcY, dstX, dstY,
                    PY_BLUE, tileIndex++, -1.0);
                ++blueIndex;
            }
        }
    }

    static void PythPlaySound(int sound)
    {
        if (CheckSoundMem(sound)) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);
    }

    // 一つの図形セットを更新する。
    static void ShotPythagorean(sEnemyShotSet* pEnemyShotSet)
    {
        constexpr int TRANSFER_START = 180-60;
        constexpr int TRANSFER_END = 300;
        constexpr int BURST_START = 420;
        constexpr int OUTLINE_END = 500;
        constexpr int REMOVE_ALL = 530;

        if (pEnemyShotSet->count == 0) {
            PythCreateDiagram(pEnemyShotSet);
        }

        const int c = pEnemyShotSet->count;
        if (c == TRANSFER_START) PythPlaySound(sound_enemyShot_medium);
        if (c == BURST_START) PythPlaySound(sound_enemyShot_extreme);

        // 斜辺側の正方形の中心。
        const double centerX = pEnemyShotSet->x + 35.0 * PY_SCALE;
        const double centerY = pEnemyShotSet->y + 150.0 * PY_SCALE;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (c >= REMOVE_ALL) {
                // 画面外の弾の削除自体はメインルーチンに任せる。
                pShot->x = -1000.0;
                pShot->y = -1000.0;
                pShot = pShot->next;
                continue;
            }

            if (pShot->param_i[0] == PY_ROLE_LINE) {
                if (c >= OUTLINE_END) {
                    pShot->x = -1000.0;
                    pShot->y = -1000.0;
                }
                pShot = pShot->next;
                continue;
            }

            const double srcX = pShot->param_d[0];
            const double srcY = pShot->param_d[1];
            const double dstX = pShot->param_d[2];
            const double dstY = pShot->param_d[3];

            if (c < TRANSFER_START) {
                // 証明開始前：赤と青の正方形をタイルで埋めておく。
                pShot->x = srcX;
                pShot->y = srcY;
            }
            else if (c < TRANSFER_END) {
                // 滑らかな補間に、短い円弧状の移動を加えて面積の移し替えを見せる。
                double t = (c - TRANSFER_START)
                    / static_cast<double>(TRANSFER_END - TRANSFER_START);
                const double ease = t * t * (3.0 - 2.0 * t);
                const double dx = dstX - srcX;
                const double dy = dstY - srcY;
                const double len = std::sqrt(dx * dx + dy * dy);
                const double arc = 18.0 * PY_SCALE * pShot->param_d[4]
                    * std::sin(PY_PI * t);

                double nx = 0.0;
                double ny = 0.0;
                if (len > 0.001) {
                    nx = -dy / len;
                    ny = dx / len;
                }

                pShot->x = srcX + dx * ease + nx * arc;
                pShot->y = srcY + dy * ease + ny * arc;
            }
            else if (c < BURST_START) {
                // 移し替え完了：赤と青のタイルで黄色の正方形が埋まる。
                pShot->x = dstX;
                pShot->y = dstY;
            }
            else {
                // 証明完成：中心からの距離に比例して射出角度を回転させる。
                double dx = dstX - centerX;
                double dy = dstY - centerY;
                double len = std::sqrt(dx * dx + dy * dy);
                if (len < 0.001) {
                    const double angle = pShot->param_i[1] * 2.399963229728653;
                    dx = std::cos(angle);
                    dy = std::sin(angle);
                    len = 1.0;
                }

                const double angle = std::atan2(dy, dx) + len * 0.01;
                dx = std::cos(angle);
                dy = std::sin(angle);

                const double distance = (c - BURST_START) * 2.2 * PY_SCALE*0.7;
                pShot->x = dstX + dx * distance;
                pShot->y = dstY + dy * distance;
            }

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_PythagoreanTheorem_ChatGPT()
{
    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;
    }

    // 図形を画面内に収めたまま、ボスは上部で小さく左右に揺れる。
    enemy.x = 240.0 + 18.0 * std::sin(count * 0.006);
    enemy.y = 45.0 + 4.0 * std::sin(count * 0.013);

    // 初回のみ、プレイヤーを直角三角形の内部へ配置する。
    // 現在の player.x における斜辺と底辺の中間の高さを選ぶ。
    if (count == 1) {
        const double triLeft = enemy.x - 70.0 * PY_SCALE;
        const double triRight = enemy.x + 50.0 * PY_SCALE;
        const double baseY = enemy.y + 255.0 * PY_SCALE;
        double t = (player.x - triLeft) / (triRight - triLeft);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        const double hypotenuseY = enemy.y + (165.0 + 90.0 * t) * PY_SCALE;
        player.y = (hypotenuseY + baseY) * 0.5-95;
    }

    // 一つの証明演出が終わる間隔で、同じ弾幕を繰り返す。
    if (count % 600 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPythagorean;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y-95;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        PythPlaySound(sound_enemyCharge);
    }
}