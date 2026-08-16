# EtRobocon2026

ET ロボコン 2026 向けの走行アプリケーションです。  
`etrobo/workspace/EtRobocon2026` 配下に配置して、シミュレータビルドと実機向け設定を同じコードベースで運用します。

## このプロジェクトでできること

- シナリオベース走行 (`ScenarioTracer`)
- PID ベースのライントレース (`LineTracer` + `Pid`)
- キャリブレーションフロー (`Calibrator`)
- タスク/周期ハンドラ連携 (`app.cfg`, `app.cpp`)
- push 前ビルドチェック (`.githooks/pre-push`)

## ディレクトリ構成

```text
EtRobocon2026/
	app/        # 走行ロジック本体（Tracer, PID, Calibrator など）
	unit/       # テスト/補助コード
	docs/       # 操作手順・規定書・教材ドキュメント
	app.cpp     # メインタスク、トレーサ配列生成、タスク制御
	app.cfg     # ASP3 タスク/周期ハンドラ定義
	.githooks/  # pre-push フック
```

## クイックスタート

### 1. 初回設定（1回だけ）

```bash
cd /path/to/etrobo/workspace/EtRobocon2026
git config core.hooksPath .githooks
chmod +x .githooks/pre-push
```

### 2. シミュレータビルド

`etrobo` ルートで実行します。

```bash
cd /path/to/etrobo
make app=EtRobocon2026 sim
```

> [!NOTE]
> 本リポジトリの pre-push フックも同じコマンドを実行し、失敗時は push を停止します。

## 実行フロー概要

1. `main_task` が初期化し、キャリブレーション周期タスクを開始
2. `Calibrator` 完了後、`LineTracer` の目標輝度を補正
3. トレーサ周期タスクで `tracerList` を順に実行
4. 各トレーサが終端条件を満たすと次へ遷移

## ETラリー設計方針

ETラリーは、経路生成と走行制御を分離して設計しています。

- 経路生成: RallyRouteSolver
- 走行制御: RallyTracer

### 座標系と前提

- ゲート座標は 5x5 グリッド（1..5）
- QR 座標は 4x4 グリッド（1..4）
- QR はゲートグリッドの交点ではなく、各セル中央に配置される
- そのため 1 本のゲートバーに対して、通過できる QR ルートは 1 本だけ
	- 逆方向を含めると 2 パターン

### 外周ゲートの扱い

- 外周ゲート（x=1/5, y=1/5）を通過するため、仮想 QR（0 または 5）を導入
- 経路上では以下の 1 ステップとして表現
	- VIRTUAL_DETOUR: 仮想 QR へ前進して通過し、即座に元の実 QR へ後退

### ソルバーの方針

- 赤 → 青 → 黄の順を 1 周として、最大 3 周まで経路を生成
- QR グリッド上の移動は上下左右の 4 方向のみ
- 最短経路探索は BFS
- 候補選択の距離評価はマンハッタン距離

### トレーサの方針

- 現在実装はデッドレコニング（エンコーダ）ベース
- QR 読み取りや QR 識別は行わない
- RallyRoute の各ステップを 旋回 → 前進（必要なら後退）で実行
- オプションで床面の黒マーカ検出（反射光）による位置補正を利用可能
	- QRデコードは行わず、黒パターンを「位置合わせイベント」として使う

### Lコース対応

- IS_LEFT_COURSE が true の場合、RallyTracer 入力を左右反転してから経路生成
	- QR の x: x' = 5 - x
	- Gate の gx: gx' = 6 - gx
	- 方位角: heading' = (180 - heading) mod 360

### tracer.ini 設定

RallyTracer の指定形式は以下です。

RallyTracer movePwm turnPwm startX startY startHeading lapCount red_gx1 red_gy1 red_gx2 red_gy2 blue_gx1 blue_gy1 blue_gx2 blue_gy2 yellow_gx1 yellow_gy1 yellow_gx2 yellow_gy2

startHeading は次の方位角定義を使います。

- 0: 東
- 90: 北
- 180: 西
- 270: 南

## 設定と拡張ポイント

- 走行シーケンス定義: `app.cpp` の `generateTracerList()`
- タスク周期/優先度: `app.cfg`
- トレース挙動: `app/LineTracer.cpp`, `app/Pid.cpp`
- シナリオ挙動: `app/ScenarioTracer.cpp`
- キャリブレーション挙動: `app/Calibrator.cpp`

### ライントレースの安定化

`tracer.ini` の `LineTracer` は、以下の順で指定します。

```text
LineTracer <distance> <brightness> <basePwm> <maxPwm> <edge> <kp> <ki> <kd> [stopColor]
```

- `maxPwm` は左右モータ出力の絶対値上限です。各出力は `[-maxPwm, +maxPwm]` に制限されます。旋回量そのものの上限ではありません。
- ライントレース開始時にPIDの積分値と微分用履歴をリセットします。
- `ki` が0以外の場合、I項単独の操作量が `maxPwm` を超えないよう積分値を制限します。
- 反射光には3サンプルの中央値フィルタを適用し、単発のセンサ外れ値を抑えます。

現在は、出力飽和中に積分を停止または巻き戻す完全なアンチワインドアップと、計測値微分を使うD項は未実装です。D項は偏差の変化から計算しています。通常のライントレースでは目標輝度が走行中に変わらないため、開始時のPIDリセットにより目標値変更に起因する急なD出力を避けています。

> [!TIP]
> デバッグ出力は `app/Log.h` のマクロ (`LOGI`, `LOGD` など) に統一すると、出力制御と可読性を維持しやすくなります。

## pre-push フック

`.githooks/pre-push` は以下を自動実行します。

- `etrobo` ルートの自動検出
- ETrobo 環境変数の読み込み
- `./make app=EtRobocon2026 sim` の実行

一時的にスキップする場合:

```bash
git push --no-verify
```

> [!WARNING]
> `--no-verify` は、ローカルでビルド成功を確認済みの場合のみに限定してください。

## 参考ドキュメント

- `docs/ETRC2026sim_manual_ver8.0(rev1.0).md`
- `docs/Build_Inst_M6.1.md`
- `docs/basics_book.md`
- `docs/modeling_book.md`
