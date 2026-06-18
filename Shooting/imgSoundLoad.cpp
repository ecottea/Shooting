#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "stageData.h"
#include <unordered_map>
#include <string>


int colorWhite;
int colorGray;
int colorGreenBlue;

// ---------- 画像ID管理 ----------
std::vector<ImageData> imageData;

// ---------- グローバル変数（IDを保持） ----------
int img_player;
int img_playerShot;
int img_enemy[2];
int img_enemyShotSmallBall[8];
int img_enemyShotMediumBall[8];
int img_enemyShotLargeBall[8];
int img_enemyShotBullet[8];
int img_enemyShotScale[8];
int img_enemyShotDiamond[8];

int sound_menuCursor;
int sound_enemyShot_light;
int sound_enemyShot_medium;
int sound_enemyShot_heavy;
int sound_enemyShot_extreme;
int sound_enemyShot_noize;
int sound_enemyCharge;
int sound_enemyDestroyed;
int sound_playerShotHit_default;
int sound_playerShotHit_bossLowHP;
int sound_playerDestroyed;

int bgm_menu;
int currentBGMHandle;

// ファイル名（例："Torpedo_Hymn2"）→ BGMハンドル のキャッシュ
static std::unordered_map<std::string, int> bgmCache;


// ---------- 単一画像読み込み ----------
static int LoadImage(const char* filename, double mag, bool no_rotate, double radius) {
    int h = LoadGraph(filename);
    if (h == -1) return -1;

    int id = static_cast<int>(imageData.size());
    imageData.resize(id + 1);
    imageData[id].handle = h;
    imageData[id].mag = mag;
    imageData[id].no_rotate = no_rotate;
    imageData[id].radius = radius;
    return id;
}

// ---------- 分割画像読み込み（idBuf にID配列を返す） ----------
static int LoadDivImages(const char* filename, int allNum, int xNum, int yNum,
    int xSize, int ySize, int* idBuf,
    double mag, bool no_rotate, double radius) {
    // 一時的にハンドル配列を確保
    int* handles = new int[allNum];
    int ret = LoadDivGraph(filename, allNum, xNum, yNum, xSize, ySize, handles);
    if (ret == -1) { delete[] handles; return -1; }

    for (int i = 0; i < allNum; ++i) {
        if (handles[i] == -1) {
            // 失敗：既に確保したIDとハンドルを解放
            for (int j = 0; j < i; ++j) {
                DeleteGraph(imageData[idBuf[j]].handle);
                imageData[idBuf[j]].handle = -1;  // ロールバック（IDは戻さない簡易実装）
            }
            delete[] handles;
            return -1;
        }
        int id = static_cast<int>(imageData.size());
        imageData.resize(id + 1);
        if (id == -1) {
            for (int j = 0; j < i; ++j) {
                DeleteGraph(imageData[idBuf[j]].handle);
                imageData[idBuf[j]].handle = -1;
            }
            DeleteGraph(handles[i]); // 今回の分
            delete[] handles;
            return -1;
        }
        idBuf[i] = id;
        imageData[id].handle = handles[i];
        imageData[id].mag = mag;
        imageData[id].no_rotate = no_rotate;
        imageData[id].radius = radius;
    }
    delete[] handles;
    return 0; // 成功
}

// ---------- カラー弾8種読み込み（白→白、黒→色付き） ----------
static bool LoadColoredShotsEx(const char* monoFileName, int width, int height,
    int* idBuf, double mag, bool no_rotate, double radius) {
    int srcSoft = LoadSoftImage(monoFileName);
    if (srcSoft == -1) return false;

    int srcW, srcH;
    GetSoftImageSize(srcSoft, &srcW, &srcH);
    if (srcW != width || srcH != height) {
        DeleteSoftImage(srcSoft);
        return false;
    }

    const struct { int r, g, b; } factors[8] = {
        {255,0,0},
        {255,255,0},
        {0,255,0},
        {0,255,255},
        {64,64,255},
        {255,0,255},
        {192,192,192},
        {64,64,64}
    };

    for (int i = 0; i < 8; ++i) {
        int dstSoft = MakeARGB8ColorSoftImage(width, height);
        if (dstSoft == -1) goto fail;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int sa, sr, sg, sb;
                GetPixelSoftImage(srcSoft, x, y, &sr, &sg, &sb, &sa);
                int lum = sr;  // モノクロ画像なので sr = sg = sb を利用
                int r = factors[i].r + ((255 - factors[i].r) * lum) / 255;
                int g = factors[i].g + ((255 - factors[i].g) * lum) / 255;
                int b = factors[i].b + ((255 - factors[i].b) * lum) / 255;
                DrawPixelSoftImage(dstSoft, x, y, r, g, b, sa);
            }
        }

        int h = CreateGraphFromSoftImage(dstSoft);
        DeleteSoftImage(dstSoft);
        if (h == -1) goto fail;

        int id = static_cast<int>(imageData.size());
        imageData.resize(id + 1);
        if (id == -1) { DeleteGraph(h); goto fail; }
        idBuf[i] = id;
        imageData[id].handle = h;
        imageData[id].mag = mag;
        imageData[id].no_rotate = no_rotate;
        imageData[id].radius = radius;
    }

    DeleteSoftImage(srcSoft);
    return true;

fail:
    // 失敗したら既に確保したものを解放
    for (int j = 0; j < 8; ++j) {
        if (imageData[idBuf[j]].handle != -1) {
            DeleteGraph(imageData[idBuf[j]].handle);
            imageData[idBuf[j]].handle = -1;
        }
    }
    DeleteSoftImage(srcSoft);
    return false;
}

void imgSoundLoad()
{
    // 画像読み込み
    img_player = LoadImage("assets/images/player.png", 1.0, true, 0.0);

    img_playerShot = LoadImage("assets/images/playerShot.png", 1.0, true, 0.0);

    LoadDivImages("assets/images/enemy.png", 2, 2, 1, 207, 194, img_enemy, 1.0, true, 0.0);

    LoadColoredShotsEx("assets/images/enemyShotSmallBall.png", 30, 30, img_enemyShotSmallBall, 0.3, true, 2.5);
    LoadColoredShotsEx("assets/images/enemyShotMediumBall.png", 32, 32, img_enemyShotMediumBall, 0.65, true, 7.0);
    LoadColoredShotsEx("assets/images/enemyShotLargeBall.png", 70, 70, img_enemyShotLargeBall, 1.0, true, 20.0);
    LoadColoredShotsEx("assets/images/enemyShotBullet.png", 42, 14, img_enemyShotBullet, 0.5, false, 2.0);
    LoadColoredShotsEx("assets/images/enemyShotScale.png", 32, 24, img_enemyShotScale, 0.5, false, 3.0);
    LoadColoredShotsEx("assets/images/enemyShotDiamond.png", 32, 16, img_enemyShotDiamond, 0.5, false, 2.5);

    // 効果音読み込み
    sound_menuCursor = LoadSoundMem("assets/sounds/menuCursor.wav");
    sound_playerShotHit_default = LoadSoundMem("assets/sounds/playerShotHit_default.wav");
    sound_playerShotHit_bossLowHP = LoadSoundMem("assets/sounds/playerShotHit_bossLowHP.wav");
    sound_playerDestroyed = LoadSoundMem("assets/sounds/playerDestroyed.wav");
    sound_enemyShot_light = LoadSoundMem("assets/sounds/enemyShot_light.wav");
    sound_enemyShot_medium = LoadSoundMem("assets/sounds/enemyShot_medium.wav");
    sound_enemyShot_heavy = LoadSoundMem("assets/sounds/enemyShot_heavy.wav");
    sound_enemyShot_extreme = LoadSoundMem("assets/sounds/enemyShot_extreme.wav");
    sound_enemyShot_noize = LoadSoundMem("assets/sounds/enemyShot_noize.wav");
    sound_enemyCharge = LoadSoundMem("assets/sounds/enemyCharge.wav");
    sound_enemyDestroyed = LoadSoundMem("assets/sounds/enemyDestroyed.wav");

    // BGM 読み込み
    bgm_menu = LoadSoundMem("assets/bgm/Caramel_CPU.ogg");
}

// 遅延ロード用の関数
void loadStageBGM(int stageIndex)
{
    if (stageIndex < 0 || stageIndex >= (int)stageData.size()) return;

    // すでにロード済みなら何もしない
    if (stageData[stageIndex].bgmHandle != -1) return;

    const char* fileName = stageData[stageIndex].bgmFileName;
    std::string key(fileName);

    // キャッシュを検索
    auto it = bgmCache.find(key);
    if (it != bgmCache.end()) {
        // キャッシュにヒット → ハンドルをコピーするだけでロード不要
        stageData[stageIndex].bgmHandle = it->second;
        return;
    }

    // 未キャッシュ → 新規ロード
    char fullPath[256];
    sprintf_s(fullPath, sizeof(fullPath), "assets/bgm/%s.ogg", fileName);

    int handle = LoadSoundMem(fullPath);
    if (handle != -1) {
        // キャッシュに登録
        bgmCache[key] = handle;
        stageData[stageIndex].bgmHandle = handle;
    }
}