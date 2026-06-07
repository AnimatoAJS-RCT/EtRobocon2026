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

## 設定と拡張ポイント

- 走行シーケンス定義: `app.cpp` の `generateTracerList()`
- タスク周期/優先度: `app.cfg`
- トレース挙動: `app/LineTracer.cpp`, `app/Pid.cpp`
- シナリオ挙動: `app/ScenarioTracer.cpp`
- キャリブレーション挙動: `app/Calibrator.cpp`

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
