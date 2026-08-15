/**
 * @file RallyRouteSolver.cpp
 * @brief ETラリー経路ソルバー実装
 */

#include "RallyRouteSolver.h"

#include "Log.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <queue>

// ---------------------------------------------------------------------------
// solve()
// ---------------------------------------------------------------------------

RallyRoute RallyRouteSolver::solve(const GateInfo& gates, const Config& config)
{
    RallyRoute route;
    QRPos currentPos = config.startPos;
    int lapCount = std::max(1, std::min(config.lapCount, 3));

    const GatePos* gateOrder[3] = {&gates.red, &gates.blue, &gates.yellow};
    const char* gateNames[3]    = {"red", "blue", "yellow"};

    for(int lap = 0; lap < lapCount; lap++) {
        for(int g = 0; g < 3; g++) {
            const GatePos& gate = *gateOrder[g];

            std::vector<GateCandidate> candidates = getGateCandidates(gate);
            if(candidates.empty()) {
                LOGE("[SOLVER] lap=%d gate=%s: no valid candidates\n", lap, gateNames[g]);
                continue;
            }

            // 現在位置から最も近いアプローチ候補を選ぶ
            int bestDist = INT_MAX;
            int bestIdx  = 0;
            for(int i = 0; i < static_cast<int>(candidates.size()); i++) {
                int dist = pathLength(currentPos, candidates[i].approachQR);
                if(dist < bestDist) {
                    bestDist = dist;
                    bestIdx  = i;
                }
            }

            const GateCandidate& best = candidates[bestIdx];

            // currentPos → approachQR の経路ステップを追加
            std::vector<QRPos> path = findPath(currentPos, best.approachQR);
            for(int i = 1; i < static_cast<int>(path.size()); i++) {
                route.addStep(RouteStep::move(path[i]));
            }

            // ゲート通過ステップを追加
            route.addStep(makeGateStep(best));

            // 現在位置を更新
            // VIRTUAL_DETOUR の場合: ロボットは approachQR に戻る
            // MOVE の場合: ロボットは exitQR に移動する
            currentPos = best.exitQR.isVirtual() ? best.approachQR : best.exitQR;

            LOGI("[SOLVER] lap=%d gate=%s: approach=(%d,%d) exit=(%d,%d) totalSteps=%u\n",
                 lap, gateNames[g],
                 best.approachQR.x, best.approachQR.y,
                 best.exitQR.x,     best.exitQR.y,
                 static_cast<unsigned>(route.size()));
        }
    }

    return route;
}

// ---------------------------------------------------------------------------
// getGateCandidates()
// ---------------------------------------------------------------------------

std::vector<RallyRouteSolver::GateCandidate>
RallyRouteSolver::getGateCandidates(const GatePos& gate)
{
    std::vector<GateCandidate> candidates;

    if(gate.gy1 == gate.gy2) {
        // ---- 水平ゲート (y 方向に通過) ----
        // ゲートバーは QR セル (qx, gy-1) の上辺 1 本。
        // ポスト (gx1,gy)-(gx2,gy) が囲む 1 セルの左下 x = min(gx1,gx2)。
        // 通過できる QR は qx の 1 点のみ（逆方向含め最大 2 候補）。
        int gy = gate.gy1;
        int qx = std::min(gate.gx1, gate.gx2);

        // 南→北通過: QR(qx, gy-1) → QR(qx, gy)  [gy=5 なら exit は仮想]
        {
            QRPos approach = {qx, gy - 1};
            QRPos exit     = {qx, gy};
            if(!approach.isVirtual()) {
                candidates.push_back({approach, exit});
            }
        }
        // 北→南通過: QR(qx, gy) → QR(qx, gy-1)  [gy=1 なら exit は仮想]
        {
            QRPos approach = {qx, gy};
            QRPos exit     = {qx, gy - 1};
            if(!approach.isVirtual() && exit.y >= 0) {
                candidates.push_back({approach, exit});
            }
        }
    } else {
        // ---- 垂直ゲート (x 方向に通過) ----
        // ゲートバーは QR セル (gx-1, qy) の右辺 1 本。
        // ポスト (gx,gy1)-(gx,gy2) が囲む 1 セルの左下 y = min(gy1,gy2)。
        // 通過できる QR は qy の 1 点のみ（逆方向含め最大 2 候補）。
        int gx = gate.gx1;  // gx1 == gx2
        int qy = std::min(gate.gy1, gate.gy2);

        // 西→東通過: QR(gx-1, qy) → QR(gx, qy)  [gx=5 なら exit は仮想]
        {
            QRPos approach = {gx - 1, qy};
            QRPos exit     = {gx, qy};
            if(!approach.isVirtual()) {
                candidates.push_back({approach, exit});
            }
        }
        // 東→西通過: QR(gx, qy) → QR(gx-1, qy)  [gx=1 なら exit は仮想]
        {
            QRPos approach = {gx, qy};
            QRPos exit     = {gx - 1, qy};
            if(!approach.isVirtual() && exit.x >= 0) {
                candidates.push_back({approach, exit});
            }
        }
    }

    return candidates;
}

// ---------------------------------------------------------------------------
// makeGateStep()
// ---------------------------------------------------------------------------

RouteStep RallyRouteSolver::makeGateStep(const GateCandidate& candidate)
{
    if(candidate.exitQR.isVirtual()) {
        // 仮想 QR へ進み、そのまま approachQR へ戻る
        return RouteStep::virtualDetour(candidate.exitQR, candidate.approachQR);
    } else {
        // 実 QR へ通常移動
        return RouteStep::move(candidate.exitQR);
    }
}

// ---------------------------------------------------------------------------
// findPath() - BFS on 4×4 QR grid (4-connectivity)
// ---------------------------------------------------------------------------

std::vector<QRPos> RallyRouteSolver::findPath(QRPos from, QRPos to)
{
    if(from == to) return {from};

    // セルインデックス: (y-1)*4 + (x-1), 範囲 0〜15
    auto cellIdx = [](const QRPos& p) -> int {
        return (p.y - 1) * 4 + (p.x - 1);
    };
    auto cellPos = [](int idx) -> QRPos {
        return {idx % 4 + 1, idx / 4 + 1};
    };

    // 斜め移動は許可せず、上下左右の 4 近傍のみを展開する
    static const int DX[4] = {-1, 1, 0, 0};
    static const int DY[4] = { 0, 0,-1, 1};

    const int GRID = 16;
    bool visited[GRID] = {};
    int  parent[GRID];
    for(int i = 0; i < GRID; i++) parent[i] = -1;

    int startIdx = cellIdx(from);
    int goalIdx  = cellIdx(to);

    visited[startIdx] = true;
    std::queue<int> q;
    q.push(startIdx);

    bool found = false;
    while(!q.empty() && !found) {
        int curIdx = q.front(); q.pop();
        QRPos cur  = cellPos(curIdx);

        for(int d = 0; d < 4; d++) {
            QRPos next = {cur.x + DX[d], cur.y + DY[d]};
            if(next.isVirtual()) continue;

            int nextIdx = cellIdx(next);
            if(visited[nextIdx]) continue;

            visited[nextIdx] = true;
            parent[nextIdx]  = curIdx;

            if(nextIdx == goalIdx) { found = true; break; }

            q.push(nextIdx);
        }
    }

    if(!found) {
        // 4×4 完全連結グリッドでは通常到達不能にならない
        LOGE("[SOLVER] findPath: cannot reach (%d,%d) from (%d,%d)\n",
             to.x, to.y, from.x, from.y);
        return {from};
    }

    // 逆順に経路を復元
    std::vector<QRPos> path;
    for(int cur = goalIdx; cur != startIdx; cur = parent[cur]) {
        path.push_back(cellPos(cur));
    }
    path.push_back(from);
    std::reverse(path.begin(), path.end());
    return path;
}

// ---------------------------------------------------------------------------
// pathLength() - マンハッタン距離
// ---------------------------------------------------------------------------

int RallyRouteSolver::pathLength(QRPos from, QRPos to)
{
    return std::abs(to.x - from.x) + std::abs(to.y - from.y);
}
