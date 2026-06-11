#pragma once
#include <vector>
#include <cassert>
#include <cstddef>

// ============================================================
//  メモリプール CRTP ベース  (C++17: inline static)
//
//  使い方: struct Foo : PoolAllocator<Foo, 256> { Foo* next; ... };
//  制約  : 派生クラスは "Derived* next;" メンバを持つこと
//          (フリーリストの連結に使用)
//
//  【設計メモ】
//  pool_ を std::array<Derived, N> で宣言すると、
//  PoolAllocator<sPlayerShot, 64> を基底クラスとして指定した瞬間
//  (sPlayerShot がまだ不完全型の段階) に実体化が走りエラーになる。
//  ポインタ宣言なら不完全型でも合法。実配列の確保は initPool() 内
//  (= 初回 new 時) に遅延するため、その時点で Derived は完全型。
// ============================================================
template<typename Derived, int N>
struct PoolAllocator {
protected:
    static constexpr int POOL_SIZE = N;

private:
    // ポインタ宣言 → 不完全型でも合法 (std::array<Derived,N> は不可)
    inline static Derived* pool_ = nullptr;
    inline static Derived* freeList_ = nullptr;
    inline static bool initialized_ = false;

    static void initPool() {
        // Derived が完全型になった後に呼ばれるので new Derived[N] は安全
        // operator new[] は未オーバーロードなので ::operator new[] が使われ再帰しない
        pool_ = new Derived[N];
        for (int i = 0; i < N - 1; ++i)
            pool_[i].next = &pool_[i + 1];
        pool_[N - 1].next = nullptr;
        freeList_ = pool_;
        initialized_ = true;
    }

public:
    void* operator new(std::size_t) {
        if (!initialized_) initPool();
        assert(freeList_ && "pool exhausted"); // 枯渇時はここでクラッシュ
        Derived* p = freeList_;
        freeList_ = freeList_->next;          // 空きチェーンから取り出す
        return p;
    }

    void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        auto* p = static_cast<Derived*>(ptr);
        p->next = freeList_;                 // 空きチェーンに戻す
        freeList_ = p;
    }
};

// ============================================================
//  共有構造体
// ============================================================
struct sCursor {
    int x = 0, y = 0;
};

struct sPlayer {
    double x = 0.0, y = 0.0;
};

// プール管理付き弾構造体: PoolAllocator を継承するだけで
// new/delete が自動的にプールを使うようになる
struct sPlayerShot : PoolAllocator<sPlayerShot, 64> {
    double       x = 0.0, y = 0.0;
    sPlayerShot* prev = nullptr;
    sPlayerShot* next = nullptr;  // PoolAllocator のフリーリストと兼用
};

struct sEnemy {
    double x = 0.0, y = 0.0;
    int    hp = 0, maxHp = 0;
};

struct sEnemyShot : PoolAllocator<sEnemyShot, 4096> {
    double      x = 0.0, y = 0.0, muki = 0.0, speed = 0.0;
    int         kind = 0;
    sEnemyShot* prev = nullptr;
    sEnemyShot* next = nullptr;  // PoolAllocator のフリーリストと兼用
};

struct sEnemyShotSet : PoolAllocator<sEnemyShotSet, 1024> {
    // 関数ポインタ型に名前を付けて宣言を読みやすくする
    using PatternFunc = void(*)(sEnemyShotSet*);

    double         x = 0.0, y = 0.0, muki = 0.0;
    PatternFunc    patternFunc = nullptr;
    int            count = 0;
    int            kind = 0;
    sEnemyShot* pEnemyShotHead = nullptr;
    sEnemyShotSet* prev = nullptr;
    sEnemyShotSet* next = nullptr;  // PoolAllocator のフリーリストと兼用
};

// ============================================================
//  列挙型  (enum class でスコープ付き・型安全に)
//
//  変更前: if (joutaiFlag == Joutai::Game)
//  変更後: if (joutaiFlag == Joutai::Game)
// ============================================================
enum class Joutai {
    Menu,
    Win,
    Lose,
    Game,
    Pause,
};

// ============================================================
//  グローバル変数（extern 宣言のみ）
// ============================================================
extern int count;
extern int key[256];

extern int colorWhite;
extern int colorGray;
extern int colorGreenBlue;

// ---------- 画像管理用の構造体とグローバル変数 ----------
struct ImageData {
    int    handle = -1;
    double mag = 1.0;
    bool   no_rotate = false;
    double radius = 0.0;
};

extern std::vector<ImageData> imageData;

// 画像ID
extern int img_player;
extern int img_playerShot;
extern int img_enemy[2];
extern int img_enemyShotSmallBall[8];
extern int img_enemyShotMediumBall[8];
extern int img_enemyShotLargeBall[8];
extern int img_enemyShotBullet[8];
extern int img_enemyShotScale[8];
extern int img_enemyShotDiamond[8];

extern int sound_menuCursor;
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyShot_heavy;
extern int sound_enemyDestroyed;
extern int sound_playerShotHit_default;
extern int sound_playerShotHit_bossLowHP;
extern int sound_playerDestroyed;

extern int bgm_menu;
extern int currentBGMHandle;

extern Joutai joutaiFlag;

extern int stageNum;

extern sCursor       cursor;
extern sPlayer       player;
extern sPlayerShot   playerShotHead;
extern sEnemy        enemy;
extern sEnemyShotSet enemyShotSetHead;
