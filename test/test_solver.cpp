/**
 * @file test_solver.cpp
 * @brief RallyRouteSolver の単体テスト（SPIKE-RT 不要・g++ でそのまま実行可能）
 *
 * コンパイル:
 *   g++ -std=c++14 -I../app -o test_solver test_solver.cpp ../app/RallyRoute.cpp ../app/RallyRouteSolver.cpp
 *
 * 実行:
 *   ./test_solver
 */

// ---------------------------------------------------------------------------
// Log.h スタブ（SPIKE-RT なしでビルドするため）
// ---------------------------------------------------------------------------
#ifndef ETTR_APP_LOG_H_
#define ETTR_APP_LOG_H_
#include <cstdio>
#define LOGI(fmt, ...) std::printf("[INFO]  " fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) std::printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) std::printf("[WARN]  " fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) std::printf("[ERROR] " fmt, ##__VA_ARGS__)
#define LOGI_EVERY(n, fmt, ...) std::printf("[INFO]  " fmt, ##__VA_ARGS__)
#define LOGD_EVERY(n, fmt, ...) std::printf("[DEBUG] " fmt, ##__VA_ARGS__)
#endif

// ---------------------------------------------------------------------------
// ソースを直接インクルード（リンク不要）
// ---------------------------------------------------------------------------
#include "RallyRoute.cpp"
#include "RallyRouteSolver.cpp"

// ---------------------------------------------------------------------------
// テストユーティリティ
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cstring>
#include <iostream>

static int gPassCount = 0;
static int gFailCount = 0;

#define EXPECT_TRUE(cond)                                                       \
    do {                                                                        \
        if(cond) {                                                              \
            gPassCount++;                                                       \
        } else {                                                                \
            gFailCount++;                                                       \
            std::printf("  FAIL [%s:%d] %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                       \
    } while(0)

#define EXPECT_EQ(a, b)                                                         \
    do {                                                                        \
        if((a) == (b)) {                                                        \
            gPassCount++;                                                       \
        } else {                                                                \
            gFailCount++;                                                       \
            std::printf("  FAIL [%s:%d] %s == %s  (got %d vs %d)\n",           \
                        __FILE__, __LINE__, #a, #b, (int)(a), (int)(b));        \
        }                                                                       \
    } while(0)

// 経路の内容を表示するデバッグヘルパー
static void printRoute(const RallyRoute& route)
{
    std::printf("  route has %u steps:\n", static_cast<unsigned>(route.size()));
    for(std::size_t i = 0; i < route.size(); i++) {
        const RouteStep& s = route[i];
        if(s.type == RouteStepType::MOVE) {
            std::printf("    [%2zu] MOVE         → (%d,%d)\n",
                        i, s.destination.x, s.destination.y);
        } else {
            std::printf("    [%2zu] VIRTUAL_DETOUR→ (%d,%d) then back to (%d,%d)\n",
                        i, s.destination.x, s.destination.y,
                        s.returnPos.x, s.returnPos.y);
        }
    }
}

// from → to の移動が指定ゲートを通過するか判定する
// ゲートバーは 1 セルの辺のため、通過できる QR は 1 点のみ
// 水平ゲート: x = min(gx1,gx2) で y が gy-1 ↔ gy を跨ぐ移動のみ
// 垂直ゲート: y = min(gy1,gy2) で x が gx-1 ↔ gx を跨ぐ移動のみ
static bool crossesGate(QRPos from, QRPos to, const GatePos& gate)
{
    if(gate.gy1 == gate.gy2) {
        // 水平ゲート at y=gy: 通過 QR x = min(gx1,gx2) のみ
        int gy = gate.gy1;
        int qx = std::min(gate.gx1, gate.gx2);
        if(from.x != to.x || from.x != qx) return false;
        return (from.y == gy - 1 && to.y == gy)
            || (from.y == gy     && to.y == gy - 1);
    } else {
        // 垂直ゲート at x=gx: 通過 QR y = min(gy1,gy2) のみ
        int gx = gate.gx1;  // gx1 == gx2
        int qy = std::min(gate.gy1, gate.gy2);
        if(from.y != to.y || from.y != qy) return false;
        return (from.x == gx - 1 && to.x == gx)
            || (from.x == gx     && to.x == gx - 1);
    }
}

// 経路をシミュレートして、指定ゲートを少なくとも 1 回通過するか確認する
// VIRTUAL_DETOUR は「仮想 QR へ前進」「元の QR へ後退」の両方向で判定する
static bool routeCrossesGate(const RallyRoute& route,
                              const GatePos& gate, QRPos startPos)
{
    QRPos cur = startPos;
    for(std::size_t i = 0; i < route.size(); i++) {
        const RouteStep& step = route[i];
        // 前進方向の通過チェック
        if(crossesGate(cur, step.destination, gate)) return true;
        if(step.type == RouteStepType::VIRTUAL_DETOUR) {
            // 後退方向の通過チェック（仮想 QR → 実 QR）
            if(crossesGate(step.destination, step.returnPos, gate)) return true;
            cur = step.returnPos;
        } else {
            cur = step.destination;
        }
    }
    return false;
}

// 経路をシミュレートして、指定ゲートを通過した回数を返す
// VIRTUAL_DETOUR の戻り方向（仮想QR→実QR）は競技上の通過とみなさず数えない
static int routeGateCrossCount(const RallyRoute& route,
                                const GatePos& gate, QRPos startPos)
{
    int count = 0;
    QRPos cur = startPos;
    for(std::size_t i = 0; i < route.size(); i++) {
        const RouteStep& step = route[i];
        // 前進方向のみカウント（MOVE・VIRTUAL_DETOUR 共通）
        if(crossesGate(cur, step.destination, gate)) count++;
        if(step.type == RouteStepType::VIRTUAL_DETOUR) {
            // 戻り方向は数えない
            cur = step.returnPos;
        } else {
            cur = step.destination;
        }
    }
    return count;
}

// 経路の各 MOVE が実 QR のみを指しているか確認（VIRTUAL_DETOUR の destination は仮想 QR）
static bool routeQRsAreValid(const RallyRoute& route)
{
    for(std::size_t i = 0; i < route.size(); i++) {
        const RouteStep& s = route[i];
        if(s.type == RouteStepType::MOVE && s.destination.isVirtual()) {
            std::printf("  ERROR: MOVE step %zu has virtual destination (%d,%d)\n",
                        i, s.destination.x, s.destination.y);
            return false;
        }
        if(s.type == RouteStepType::VIRTUAL_DETOUR) {
            if(!s.destination.isVirtual()) {
                std::printf("  ERROR: VIRTUAL_DETOUR step %zu destination (%d,%d) is not virtual\n",
                            i, s.destination.x, s.destination.y);
                return false;
            }
            if(s.returnPos.isVirtual()) {
                std::printf("  ERROR: VIRTUAL_DETOUR step %zu returnPos (%d,%d) is virtual\n",
                            i, s.returnPos.x, s.returnPos.y);
                return false;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// テストケース
// ---------------------------------------------------------------------------

// テスト1: 例示されたゲート配置（全て外周ゲート）、1周
static void test_sampleGates_1lap()
{
    std::printf("\n=== test_sampleGates_1lap ===\n");

    GateInfo gates;
    gates.red    = {2, 5, 3, 5};  // 上端（VIRTUAL_DETOUR）
    gates.blue   = {5, 3, 5, 4};  // 右端（VIRTUAL_DETOUR）
    gates.yellow = {1, 2, 2, 2};  // 内部（MOVE）

    RallyRouteSolver::Config cfg;
    cfg.startPos = {1, 1};
    cfg.lapCount = 1;

    RallyRoute route = RallyRouteSolver::solve(gates, cfg);
    printRoute(route);

    EXPECT_TRUE(!route.isEmpty());
    EXPECT_TRUE(routeQRsAreValid(route));

    // 各ゲートを実際に通過しているか、位置シミュレーションで確認する
    EXPECT_TRUE(routeCrossesGate(route, gates.red,    cfg.startPos));
    EXPECT_TRUE(routeCrossesGate(route, gates.blue,   cfg.startPos));
    EXPECT_TRUE(routeCrossesGate(route, gates.yellow, cfg.startPos));
}

// テスト2: 3周
static void test_sampleGates_3laps()
{
    std::printf("\n=== test_sampleGates_3laps ===\n");

    GateInfo gates;
    gates.red    = {2, 5, 3, 5};
    gates.blue   = {5, 3, 5, 4};
    gates.yellow = {1, 2, 2, 2};

    RallyRouteSolver::Config cfg;
    cfg.startPos = {1, 1};
    cfg.lapCount = 3;

    RallyRoute route = RallyRouteSolver::solve(gates, cfg);
    printRoute(route);

    EXPECT_TRUE(!route.isEmpty());
    EXPECT_TRUE(routeQRsAreValid(route));

    // 各ゲートを少なくとも 3 回通過しているか確認する
    // 注: 外周ゲート (red/blue) は BFS 経路で偶発的通過は起きないため正確に 3 回
    // 内部ゲート (yellow) は BFS の移動経路で偶発的通過が生じうるため >= 3 で検証
    EXPECT_EQ(routeGateCrossCount(route, gates.red,    cfg.startPos), 3);
    EXPECT_EQ(routeGateCrossCount(route, gates.blue,   cfg.startPos), 3);
    EXPECT_TRUE(routeGateCrossCount(route, gates.yellow, cfg.startPos) >= 3);
}

// テスト3: ゲートが全て内部にある場合（MOVE のみになるべき）
static void test_interiorGatesOnly()
{
    std::printf("\n=== test_interiorGatesOnly ===\n");

    GateInfo gates;
    gates.red    = {2, 2, 2, 3};  // 垂直内部ゲート x=2, y=2..3
    gates.blue   = {3, 3, 4, 3};  // 水平内部ゲート y=3, x=3..4
    gates.yellow = {2, 3, 2, 4};  // 垂直内部ゲート x=2, y=3..4 (x=1は左端なので x=2 を使う)

    RallyRouteSolver::Config cfg;
    cfg.startPos = {1, 1};
    cfg.lapCount = 1;

    RallyRoute route = RallyRouteSolver::solve(gates, cfg);
    printRoute(route);

    EXPECT_TRUE(!route.isEmpty());
    EXPECT_TRUE(routeQRsAreValid(route));

    // 全て内部ゲートなので VIRTUAL_DETOUR は 0 のはず
    int vdCount = 0;
    for(std::size_t i = 0; i < route.size(); i++) {
        if(route[i].type == RouteStepType::VIRTUAL_DETOUR) vdCount++;
    }
    EXPECT_EQ(vdCount, 0);
}

// テスト4: 下端・左端ゲート（x=1 or y=1 への VIRTUAL_DETOUR）
static void test_bottomLeftEdgeGates()
{
    std::printf("\n=== test_bottomLeftEdgeGates ===\n");

    GateInfo gates;
    gates.red    = {2, 1, 3, 1};  // 下端ゲート y=1 → virtual y=0
    gates.blue   = {1, 3, 1, 4};  // 左端ゲート x=1 → virtual x=0
    gates.yellow = {2, 3, 3, 3};  // 内部ゲート

    RallyRouteSolver::Config cfg;
    cfg.startPos = {2, 2};
    cfg.lapCount = 1;

    RallyRoute route = RallyRouteSolver::solve(gates, cfg);
    printRoute(route);

    EXPECT_TRUE(!route.isEmpty());
    EXPECT_TRUE(routeQRsAreValid(route));

    // 各ゲートを実際に通過しているか確認
    EXPECT_TRUE(routeCrossesGate(route, gates.red,    cfg.startPos));
    EXPECT_TRUE(routeCrossesGate(route, gates.blue,   cfg.startPos));
    EXPECT_TRUE(routeCrossesGate(route, gates.yellow, cfg.startPos));
}

// テスト5: startPos がゲート approachQR と一致している場合（BFS パスが空）
static void test_startOnApproach()
{
    std::printf("\n=== test_startOnApproach ===\n");

    GateInfo gates;
    gates.red    = {2, 5, 3, 5};  // approachQR は (2,4) のみ [min(2,3)=2, gy-1=4]
    gates.blue   = {5, 3, 5, 4};
    gates.yellow = {1, 2, 2, 2};

    RallyRouteSolver::Config cfg;
    cfg.startPos = {2, 4};  // 最初のゲート approachQR と一致
    cfg.lapCount = 1;

    RallyRoute route = RallyRouteSolver::solve(gates, cfg);
    printRoute(route);

    EXPECT_TRUE(!route.isEmpty());
    EXPECT_TRUE(routeQRsAreValid(route));

    // 最初のステップが VIRTUAL_DETOUR になるはず（余分な MOVE なし）
    EXPECT_EQ(static_cast<int>(route[0].type),
              static_cast<int>(RouteStepType::VIRTUAL_DETOUR));
}

// テスト6: QRPos / RallyRoute の基本動作
static void test_primitives()
{
    std::printf("\n=== test_primitives ===\n");

    // QRPos 生成ヘルパー（波括弧のコンマがマクロ引数に誤認されないよう関数化）
    auto qr = [](int x, int y) -> QRPos { return {x, y}; };

    // QRPos: 実 / 仮想の判定
    EXPECT_TRUE(!qr(1, 1).isVirtual());
    EXPECT_TRUE(!qr(4, 4).isVirtual());
    EXPECT_TRUE( qr(0, 2).isVirtual());
    EXPECT_TRUE( qr(5, 3).isVirtual());
    EXPECT_TRUE( qr(2, 0).isVirtual());
    EXPECT_TRUE( qr(3, 5).isVirtual());

    // GatePos: 外周ゲートの判定
    GatePos topEdge  = {2, 5, 3, 5};  EXPECT_TRUE( topEdge.isOnOuterEdge());
    GatePos interior = {2, 3, 3, 3};  EXPECT_TRUE(!interior.isOnOuterEdge());
    GatePos rightEdge= {5, 2, 5, 3};  EXPECT_TRUE( rightEdge.isOnOuterEdge());
    GatePos leftEdge = {1, 2, 1, 3};  EXPECT_TRUE( leftEdge.isOnOuterEdge());

    // RallyRoute の基本操作
    RallyRoute route;
    EXPECT_TRUE(route.isEmpty());
    route.addStep(RouteStep::move({1, 2}));
    EXPECT_TRUE(!route.isEmpty());
    EXPECT_EQ(static_cast<int>(route.size()), 1);
    route.clear();
    EXPECT_EQ(static_cast<int>(route.size()), 0);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::printf("===== RallyRouteSolver unit test =====\n");

    test_primitives();
    test_sampleGates_1lap();
    test_sampleGates_3laps();
    test_interiorGatesOnly();
    test_bottomLeftEdgeGates();
    test_startOnApproach();

    std::printf("\n===== RESULT: %d passed, %d failed =====\n",
                gPassCount, gFailCount);

    return gFailCount > 0 ? 1 : 0;
}
