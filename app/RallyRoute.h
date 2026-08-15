/**
 * @file RallyRoute.h
 * @brief ETラリーの経路表現クラス
 *
 * コースの前提:
 *   - 5×5のゲートポジション座標系 (1,1)〜(5,5)
 *   - 4×4のQRコードグリッド。実QR座標は (1,1)〜(4,4)
 *   - 外周(座標が 0 または 5)は仮想QR領域
 *   - ゲートは隣接する2つのゲートポジションで定義される
 *   - 外周ゲートを通過するには仮想QRへ移動し即座に戻る "仮想迂回" が必要
 */

#ifndef RALLY_ROUTE_H
#define RALLY_ROUTE_H

#include <cstddef>
#include <vector>

// ---------------------------------------------------------------------------
// QRコード座標
// ---------------------------------------------------------------------------

/**
 * @brief QRコードの格子座標
 *
 * x, y ともに 1〜4 が実在のQRコード。
 * 0 または 5 は外周の仮想QRコードを示す。
 */
struct QRPos {
    int x;  ///< 1-4: 実QR, 0 or 5: 仮想QR
    int y;  ///< 1-4: 実QR, 0 or 5: 仮想QR

    /** @brief 仮想QR（グリッド外）かどうかを返す */
    bool isVirtual() const {
        return x < 1 || x > 4 || y < 1 || y > 4;
    }

    bool operator==(const QRPos& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const QRPos& other) const {
        return !(*this == other);
    }
};

// ---------------------------------------------------------------------------
// ゲート情報
// ---------------------------------------------------------------------------

/**
 * @brief ゲートの位置（5×5ゲートポジション座標系）
 *
 * 隣接する2点 (gx1,gy1)-(gx2,gy2) でゲートを定義する。
 * 2点は水平方向か垂直方向に1マスだけ隣接していなければならない。
 */
struct GatePos {
    int gx1;  ///< 端点1 x座標 (1-5)
    int gy1;  ///< 端点1 y座標 (1-5)
    int gx2;  ///< 端点2 x座標 (1-5)
    int gy2;  ///< 端点2 y座標 (1-5)

    /**
     * @brief ゲートが外周にあるかどうかを返す
     *
     * 両端点のいずれかが座標 1 または 5 の外縁に接している場合に真。
     * 外周ゲートを通過するには仮想迂回が必要になる。
     */
    bool isOnOuterEdge() const {
        return (gx1 == 1 && gx2 == 1) || (gx1 == 5 && gx2 == 5)
            || (gy1 == 1 && gy2 == 1) || (gy1 == 5 && gy2 == 5);
    }
};

/**
 * @brief 3色ゲートの集合
 *
 * red → blue → yellow の順に通過すると1周。
 * プログラム起動時に固定値として設定する（後でキャリブレーション対応予定）。
 *
 * 使用例:
 * @code
 * GateInfo gates;
 * gates.red    = { 2, 5, 3, 5 };   // (2,5)-(3,5)
 * gates.blue   = { 5, 3, 5, 4 };   // (5,3)-(5,4)
 * gates.yellow = { 1, 2, 2, 2 };   // (1,2)-(2,2)
 * @endcode
 */
struct GateInfo {
    GatePos red;
    GatePos blue;
    GatePos yellow;
};

// ---------------------------------------------------------------------------
// 経路ステップ
// ---------------------------------------------------------------------------

/**
 * @brief 経路ステップの種別
 */
enum class RouteStepType {
    /**
     * 通常移動: 指定した実QR座標へ直線的に移動する。
     * その移動の途中でゲートを通過する場合もある。
     */
    MOVE,

    /**
     * 仮想迂回: 仮想QR座標へ移動してゲートを通過し、
     * 直後に出発地点（実QR）まで後退する。
     * 外周ゲートを通過する場合にのみ使用する。
     */
    VIRTUAL_DETOUR,
};

/**
 * @brief 経路の1ステップ
 *
 * ステップ種別に応じた座標情報を保持する。
 *
 * - MOVE:
 *     destination のQRへ移動する。
 *
 * - VIRTUAL_DETOUR:
 *     destination（仮想QR）へ移動し、通過後すぐ returnPos（実QR）へ戻る。
 *     destination.isVirtual() == true であることが前提。
 */
struct RouteStep {
    RouteStepType type;
    QRPos destination;  ///< 移動先QR座標
    QRPos returnPos;    ///< VIRTUAL_DETOUR のとき: 後退先の実QR座標

    /**
     * @brief 通常移動ステップを生成する
     * @param dest 移動先の実QR座標
     */
    static RouteStep move(QRPos dest) {
        RouteStep step;
        step.type = RouteStepType::MOVE;
        step.destination = dest;
        step.returnPos = dest;
        return step;
    }

    /**
     * @brief 仮想迂回ステップを生成する
     * @param virtualDest 仮想QR座標（外周方向の移動先）
     * @param realBase    後退先となる実QR座標
     */
    static RouteStep virtualDetour(QRPos virtualDest, QRPos realBase) {
        RouteStep step;
        step.type = RouteStepType::VIRTUAL_DETOUR;
        step.destination = virtualDest;
        step.returnPos = realBase;
        return step;
    }
};

// ---------------------------------------------------------------------------
// 経路クラス
// ---------------------------------------------------------------------------

/**
 * @brief ETラリーの走行経路
 *
 * RouteStep のシーケンスとして経路を表現する。
 * ソルバー（RallyRouteSolver）が生成し、トレーサー（RallyTracer）が消費する。
 *
 * 使用例:
 * @code
 * RallyRoute route;
 * route.addStep(RouteStep::move({ 2, 3 }));
 * route.addStep(RouteStep::move({ 3, 3 }));
 * route.addStep(RouteStep::virtualDetour({ 3, 5 }, { 3, 4 }));
 * route.addStep(RouteStep::move({ 2, 3 }));
 * @endcode
 */
class RallyRoute {
public:
    RallyRoute() = default;

    /** @brief ステップを末尾に追加する */
    void addStep(const RouteStep& step);

    /** @brief 全ステップを削除する */
    void clear();

    /** @brief ステップ数を返す */
    std::size_t size() const { return mSteps.size(); }

    /** @brief 経路が空かどうかを返す */
    bool isEmpty() const { return mSteps.empty(); }

    /** @brief i番目のステップを返す（範囲チェックなし） */
    const RouteStep& operator[](std::size_t index) const { return mSteps[index]; }

    /** @brief 全ステップへの読み取り専用参照を返す */
    const std::vector<RouteStep>& steps() const { return mSteps; }

private:
    std::vector<RouteStep> mSteps;
};

#endif  // RALLY_ROUTE_H
