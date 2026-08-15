/**
 * @file RallyRouteSolver.h
 * @brief ETラリー経路ソルバー
 *
 * ゲート情報から走行経路（RallyRoute）を生成する。
 * 赤→青→黄の順にゲートを通過する周回経路を BFS で求める。
 */

#ifndef ETTR_APP_RALLYROUTESOLVER_H_
#define ETTR_APP_RALLYROUTESOLVER_H_

#include "RallyRoute.h"

#include <vector>

class RallyRouteSolver {
public:
    struct Config {
        QRPos startPos = {1, 1};  ///< 走行開始 QR 座標
        int   lapCount = 1;       ///< 周回数 (1〜3)
    };

    /**
     * @brief 経路を生成して返す
     *
     * ゲート通過候補を列挙し、現在位置から最も近い候補を選びながら
     * red → blue → yellow の順に lapCount 周する経路を生成する。
     *
     * @param gates  ゲート位置情報
     * @param config 開始条件
     * @return 生成された経路 (候補が見つからないゲートはスキップ)
     */
    static RallyRoute solve(const GateInfo& gates, const Config& config);

private:
    /**
     * @brief ゲート通過の候補
     *
     * approachQR から exitQR への移動でゲートを通過する。
     * exitQR が仮想 QR の場合は VIRTUAL_DETOUR ステップになる。
     */
    struct GateCandidate {
        QRPos approachQR;  ///< 通過直前に立つ実 QR 座標
        QRPos exitQR;      ///< 通過後の QR 座標（仮想の場合あり）
    };

    /**
     * @brief ゲートの全通過候補を列挙する
     *
     * - 水平ゲート (gy1==gy2): Y 方向に通過。南→北 / 北→南 の両方向を検討。
     * - 垂直ゲート (gx1==gx2): X 方向に通過。西→東 / 東→西 の両方向を検討。
     * 仮想 QR からの出発は不可。仮想 QR への到達は VIRTUAL_DETOUR で対応。
     */
    static std::vector<GateCandidate> getGateCandidates(const GatePos& gate);

    /**
     * @brief 候補に対応する RouteStep を生成する
     *
     * exitQR が仮想 QR → VIRTUAL_DETOUR、実 QR → MOVE
     */
    static RouteStep makeGateStep(const GateCandidate& candidate);

    /**
     * @brief 4×4 QR グリッド上で from→to の最短経路を BFS で求める
     *
     * 4 方向（上下左右）のみ許可。グリッド外（仮想 QR）には進まない。
     * from==to の場合は {from} のみ返す。
     */
    static std::vector<QRPos> findPath(QRPos from, QRPos to);

    /**
     * @brief from→to のマンハッタン距離を返す
     *
     * 候補選択のコスト計算用。4 方向移動の最短手数と一致する。
     */
    static int pathLength(QRPos from, QRPos to);
};

#endif  // ETTR_APP_RALLYROUTESOLVER_H_
