---
name: "Ultrasonic Sensor Phase 1"
description: "Implement and prepare the Phase 1 ultrasonic sensor characterization for EtRobocon2026"
argument-hint: "Optional: experiment or implementation focus"
agent: "agent"
---

ETロボコン2026 の超音波センサー検出を Phase 1（診断・実機計測準備）として対応してください。追加の焦点があれば `$ARGUMENTS` を優先し、なければ以下をすべて実施します。

最初に [承認済み仕様](../../docs/ultrasonicsensor_approve.md) と [プロジェクト規約](../../AGENTS.md) を読み、現行実装と周辺 API を最小限確認してください。作業対象は SPIKE-RT / TOPPERS ASP3 / C++ です。

## 固定制約

- 超音波センサー（PORT_F）の取付位置は大会規約上変更不可です。高さ変更、胴部高さへの再取付、またはそれを前提とした実装・推奨を行わないでください。
- 承認済み仕様内の E2（センサー高さ変更）は実施不能として文書化し、固定位置のまま比較可能な条件（対象物、距離、姿勢、床面など）を記録する代替計測に置き換えてください。
- `-1` は DISTL の生値としての無反射・無効測定であり、ドライバエラーと決めつけないでください。`pup_device_get_values()` の返却値 `pbio_error_t` を別に記録して初めて区別します。
- 有効な DISTL 生値の上限は 2500 mm です。900〜2000 mm を無効値として捨てないでください。
- ここでは既存の `UltrasonicAlignTracer` 探索アルゴリズムを変更しません。Phase 2 は人間による実機計測結果を受け取るまで着手しません。

## Phase 1 の実装

1. `app/UltrasonicProbeTracer.cpp/.h` を追加し、`app.cpp` の `generateTracerList()` と `tracer.ini` に登録してください。
2. `UltrasonicProbeTracer <mode> <sample_count> [sample_interval_ms]` を受け付けます。`mode` は `DISTL`、`DISTS`、`TRAW`、`ADRAW`、`ALL` とし、既定のサンプル間隔は 100 ms にしてください。
3. 毎周期 `mWalker->brake()` して、ロボットを走行・旋回させません。
4. 必ず次のローカルソースで API、デバイス取得方法、モード enum 名を検証してから実装してください。定数名を推測しないでください。
   - `~/etrobo/spike-rt/drivers/include/libcpp/spike/UltrasonicSensor.h`
   - `~/etrobo/spike-rt/drivers/include/spike/pup/ultrasonicsensor.h`
   - `~/etrobo/spike-rt/drivers/spike/pup/ultrasonicsensor.c`
   - `~/etrobo/spike-rt/drivers/spike/pup_device.h`
   - `~/etrobo/spike-rt/external/libpybricks/lib/pbio/include/pbio/iodev.h`
5. `pup_device_get_values()` を直接使い、エラーと値を分離して、1サンプル1行の TSV で出力してください。

   ```text
   [ULTRA_PROBE]\tseq\tmode\terr\tvalue\telapsed_ms
   ```

6. `ALL` は DISTL → TRAW → ADRAW の順に読みます。モードを切り替えるたび、最低 50 ms 待って stale データを 1 回読み捨ててから記録してください。1 周の所要時間もログ化してください。
7. 計測手順と集計項目を `docs/UltrasonicSensorCharacterization.md` に書いてください。E1、E3、E4、E5、E6、E7 は保持し、E2 は上記制約と代替計測を明記します。各条件を Bluetooth ログへ 30 秒以上保存すること、結果表のテンプレート、`-1` の由来、TRAW/ADRAW の有用性、連続旋回と静止測定の比較を含めます。

## 進め方と検証

- 編集前に、対象コードを制御する箇所、検証可能な仮説、最小の確認方法を短く示してください。
- Phase 1 の追加後、まず対象アプリの狭いビルドまたは型・構文検査を実行し、その後 `cd ~/etrobo && ./make app=EtRobocon2026 sim` を実行してください。
- 実機でしか確認できない項目は推測で完了扱いにせず、実行手順と未確定事項を報告してください。
- Phase 1 の実装が済んだら停止し、Phase 2 のヒストグラム化、速度変更、接近戦略、既存ロジック改修には進まないでください。
- ブランチやコミットの作成は、利用者から明示的に依頼された場合だけ行ってください。