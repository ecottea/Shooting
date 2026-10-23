#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 北斗七星「七星巡航陣」
// 赤い大玉7発で星を、青い小玉で星を結ぶ線を表現する。
// 星の配置はゆっくり回転し、各星から順番に狙い弾を発射する。
// 一定時間後は星を結ぶ青い弾が外側へ飛び出し、七つの星から赤い弾を一斉発射する。

namespace {
    struct HokutoPoint {
        double x;
        double y;
    };

    // 星の接続順：ひしゃくの器を囲み、柄を伸ばす。
    static const int HOKUTO_EDGE_COUNT = 7;
    static const int hokutoEdges[HOKUTO_EDGE_COUNT][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
        { 2, 4 }, { 4, 5 }, { 5, 6 }
    };

    // 星のローカル座標。素材に頼らず、点と線の配置で北斗七星を表現する。
    static const HokutoPoint hokutoBasePoints[7] = {
        { -52.0, -20.0 },
        { -20.0, -20.0 },
        { -20.0,  12.0 },
        { -52.0,  12.0 },
        {  12.0,  25.0 },
        {  42.0,  43.0 },
        {  72.0,  61.0 }
    };

    // 弾の用途（sEnemyShot::param_i[0]）。
    enum HokutoShotType {
        HOKUTO_STAR = 0,       // 回転する星（赤い大玉）
        HOKUTO_LINE = 1,       // 星を結ぶ線の一部（青い小玉）
        HOKUTO_LINE_RELEASED = 2, // 外側へ飛び出した青い小玉
        HOKUTO_SEQUENTIAL = 3, // 各星から順番に撃つ狙い弾
        HOKUTO_VOLLEY = 4      // 七つの星からの一斉攻撃
    };

    static void HokutoAppendShot(sEnemyShotSet* pSet, double x, double y,
        double angle, double speed, int image, int type,
        int index = 0, int dotIndex = 0)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = angle;
        pShot->speed = speed;
        pShot->kind = image;
        pShot->param_i[0] = type;
        pShot->param_i[1] = index;
        pShot->param_i[2] = dotIndex;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    static sEnemyShotSet* HokutoCreateShotSet(double x, double y,
        sEnemyShotSet::PatternFunc func)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = x;
        pSet->y = y;
        pSet->patternFunc = func;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
        return pSet;
    }

    static void HokutoGetPoints(double centerX, double centerY, double angle,
        HokutoPoint points[7])
    {
        const double c = cos(angle);
        const double s = sin(angle);
        for (int i = 0; i < 7; ++i) {
            points[i].x = centerX + hokutoBasePoints[i].x * c - hokutoBasePoints[i].y * s;
            points[i].y = centerY + hokutoBasePoints[i].x * s + hokutoBasePoints[i].y * c;
        }
    }

    static void HokutoCreateLineDots(sEnemyShotSet* pSet)
    {
        for (int edge = 0; edge < HOKUTO_EDGE_COUNT; ++edge) {
            // 星そのものと重ならないよう、線分上の内側に青い小玉を並べる。
            for (int dot = 1; dot <= 7; ++dot) {
                HokutoAppendShot(pSet, 0.0, 0.0, 0.0, 0.0,
                    img_enemyShotSmallBall[4], HOKUTO_LINE, edge, dot);
            }
        }
    }

    static void HokutoPlaySound(int soundHandle)
    {
        if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    static void HokutoConstellation(sEnemyShotSet* pSet)
    {
        const int t = pSet->count; // カウント自体はメインルーチンが更新する。
        const int phase = t % 270;
        const double centerX = enemy.x;
        const double centerY = enemy.y + 130.0;

        // 150フレーム目に向けて回転が加速し、線の放出後も配置は連続して回転する。
        // 周期ごとの回転量を補正し、周期境界で星の配置が瞬間移動しないようにする。
        const int cycle = t / 270;
        double acceleratedRotation = 0.0;
        if (phase > 100) {
            int extraFrames = phase - 100;
            if (extraFrames > 50) extraFrames = 50;
            acceleratedRotation = extraFrames * 0.014;
        }
        const double rotation = t * 0.004 + cycle * 0.7 + acceleratedRotation;

        HokutoPoint points[7];
        HokutoGetPoints(centerX, centerY, rotation, points);

        // 初回のみ星を作成する。以降は各周期の開始時に線の弾だけを補充する。
        if (pSet->param_i[15] == 0) {
            for (int i = 0; i < 7; ++i) {
                HokutoAppendShot(pSet, points[i].x, points[i].y, 0.0, 0.0,
                    img_enemyShotLargeBall[0], HOKUTO_STAR, i, 0);
            }
            HokutoCreateLineDots(pSet);
            pSet->param_i[15] = 1;
        }
        else if (phase == 0) {
            HokutoCreateLineDots(pSet);
        }

        // 線を描いていた青い弾を、線の位置から外向きに一斉発射する。
        if (phase == 150) {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] == HOKUTO_LINE) {
                    const int edge = pShot->param_i[1];
                    const int dot = pShot->param_i[2];
                    const HokutoPoint& a = points[hokutoEdges[edge][0]];
                    const HokutoPoint& b = points[hokutoEdges[edge][1]];
                    const double ratio = dot / 8.0;
                    pShot->x = a.x + (b.x - a.x) * ratio;
                    pShot->y = a.y + (b.y - a.y) * ratio;
                    pShot->muki = atan2(pShot->y - centerY, pShot->x - centerX);
                    pShot->speed = 2.8;
                    pShot->param_i[0] = HOKUTO_LINE_RELEASED;
                }
                pShot = pShot->next;
            }
        }

        // 各星が順番に、プレイヤーを狙った黄色い扇状弾を撃つ。
        if ((phase < 150 || phase > 205) && t % 24 == 1) {
            const int starIndex = (t / 24) % 7;
            const double aim = atan2(player.y - points[starIndex].y,
                player.x - points[starIndex].x);
            for (int k = -2; k <= 2; ++k) {
                HokutoAppendShot(pSet, points[starIndex].x, points[starIndex].y,
                    aim + k * 0.10, 2.8, img_enemyShotMediumBall[1],
                    HOKUTO_SEQUENTIAL);
            }
            HokutoPlaySound(sound_enemyShot_light);
        }

        // 一斉攻撃の少し前に予告音を鳴らす。
        if (phase == 164) {
            HokutoPlaySound(sound_enemyCharge);
        }

        // 七つの星それぞれから、プレイヤーを狙う赤い扇状弾を同時に放つ。
        if (phase == 180) {
            for (int i = 0; i < 7; ++i) {
                const double aim = atan2(player.y - points[i].y, player.x - points[i].x);
                for (int k = -2; k <= 2; ++k) {
                    HokutoAppendShot(pSet, points[i].x, points[i].y,
                        aim + k * 0.075, 3.5, img_enemyShotMediumBall[0],
                        HOKUTO_VOLLEY);
                }
            }
            HokutoPlaySound(sound_enemyShot_extreme);
        }

        // 弾を更新する。星と線は幾何学的な位置を保ち、発射済みの弾は直進する。
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            switch (pShot->param_i[0]) {
            case HOKUTO_STAR: {
                const int index = pShot->param_i[1];
                pShot->x = points[index].x;
                pShot->y = points[index].y;
                break;
            }
            case HOKUTO_LINE: {
                const int edge = pShot->param_i[1];
                const int dot = pShot->param_i[2];
                const HokutoPoint& a = points[hokutoEdges[edge][0]];
                const HokutoPoint& b = points[hokutoEdges[edge][1]];
                const double ratio = dot / 8.0;
                pShot->x = a.x + (b.x - a.x) * ratio;
                pShot->y = a.y + (b.y - a.y) * ratio;
                break;
            }
            case HOKUTO_LINE_RELEASED:
            case HOKUTO_SEQUENTIAL:
            case HOKUTO_VOLLEY:
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;
            default:
                break;
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体の弾幕パターン
void EnemyPat_BigDipper_ChatGPT()
{
    static int moveDirection;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 仕様により200から変更しない。
        moveDirection = 1;

        HokutoCreateShotSet(enemy.x, enemy.y + 130.0, HokutoConstellation);
    }
    else {
        // 編隊全体が画面中央付近をゆっくり移動し、北斗七星の回転を見やすくする。
        enemy.x += 0.65 * moveDirection;
        if (enemy.x >= 275.0) {
            enemy.x = 275.0;
            moveDirection = -1;
        }
        else if (enemy.x <= 205.0) {
            enemy.x = 205.0;
            moveDirection = 1;
        }
    }
}
