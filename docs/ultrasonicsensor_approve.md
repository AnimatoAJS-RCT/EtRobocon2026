# 依頼: UltrasonicAlignTracer の超音波検出ロジック刷新（2フェーズ）

あなたは ETロボコン2026 の走行体ソフト（SPIKE-RT / TOPPERS ASP3 / C++）の改修を担当します。
対象リポジトリ: `~/etrobo/spike-rt/sdk/workspace/EtRobocon2026`
主対象ファイル: `app/UltrasonicAlignTracer.cpp` / `app/UltrasonicAlignTracer.h`
関連: `app/UltrasonicDistanceLoggerTracer.cpp/.h`, `app.cpp`, `tracer.ini`, `docs/UltrasonicAlignTracer.md`

## 0. 現状の問題

ET相撲用の「力士ボトル」（黒ラベルのPETボトル）を超音波センサ（PORT_F）で探索しているが、
**約300mm より遠いとほぼ全て -1 が返り、検出できない**。

## 1. 前提として確定している事実（調査済み。これを覆さないこと）

### 1-1. `-1` はドライバエラーではない

現行コードは `distance < 0` を「PUPドライバエラー」とコメントしているが、**これは誤り**。

- `-1` は DISTL モードが返す **「有効な距離が測れなかった／範囲内に反射体なし」** の値。
  LEGO 公式 SPIKE3 Python doc が "If the Distance Sensor cannot read a valid distance it will return -1" と明記。
  Pybricks も `data[0] < 0 || data[0] >= 2000 ? 2000 : data[0]` として負値を「無効」扱いしている。
- spike-rt の実装は次のとおりで、**Pybricks 相当のクランプがコメントアウトされており生値が素通りする**:

```c
// spike-rt: drivers/spike/pup/ultrasonicsensor.c
int32_t pup_ultrasonic_sensor_distance(pup_device_t *pdev) {
  int32_t distance;
  pbio_error_t err = pup_device_get_values(
      pdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL, &distance);
  if (err != PBIO_SUCCESS) {
    syslog(LOG_ERROR, "pup_ultrasonic_sensor_distance() failed.");
    distance = -err;          // ← ここだけが「本物のエラー」
  }
  return distance;            // ← それ以外は生値。無反射なら -1
}
```

- したがって **`-1` は「エラー」ではなく「この方位には検出範囲内に何も無い」という有効な情報**として扱う。
- ただし `-PBIO_ERROR_FAILED == -1` なので、生値の -1 と真のエラーは**現状のAPIでは区別できない**。
  区別が必要なら `pup_device_get_values()` を直接呼んで `pbio_error_t` を分離すること
  （宣言: `~/etrobo/spike-rt/drivers/spike/pup_device.h`）。

### 1-2. `SENSOR_INVALID_MM = 900` は誤り

- DISTL の RAW レンジはデバイス申告で **0…2500 (mm)**。LEGO 公称レンジは 50–2000mm ±20mm。
- **900〜2000mm の値は正当な測定値であり、センチネルではない。**
- 実機ログ `docs/log/robot_log_20260802_*.txt` に 952/1155/1463/1710/1975mm 等の実測値が
  **92サンプル**記録されており、現行コードはこれを全て `outOfRange` として破棄している。
- ヘッダの「999mm前後の『エコーなし』センチネル」というコメントは事実誤認なので修正すること。

### 1-3. センサの仕様（LEGO 公式 techspecs / Pybricks ソース）

| 項目 | 値 |
|---|---|
| ビーム角 | **±35°（距離により変動）** |
| 公称レンジ | 50–2000mm ±20mm / Fast sensing 50–300mm ±15mm |
| 公称サンプルレート | 100Hz（ただし DISTL の実効更新は実測で100ms オーダー） |
| 最小検知 | 約40mm未満は測定不可（値が下限に飽和する） |
| モード切替コスト | **切替後 50ms は stale データが返る**（Pybricks `lego_device_stale_data_delay()`）。タイムアウト再送は500ms |

LPF2 モード一覧（`pup_device_get_values(pdev, mode, &v)` で読める）:

| # | 名前 | 型 | 単位 | レンジ | 内容 |
|---|---|---|---|---|---|
| 0 | DISTL | int16 | mm | 0…2500 | 通常の長距離測距（現在使用中） |
| 1 | DISTS | int16 | mm | 0…320 | 短距離高速モード |
| 2 | SINGL | int16 | mm | 0…2500 | 単発測距（推定） |
| 3 | LISTN | int8 | bool | 0…1 | 他センサの超音波検出 |
| 4 | **TRAW** | int32 | **µs** | 0…14577 | **生の往復エコー時間**（14577µs×343m/s÷2 = 2.500m で DISTL と一致） |
| 5 | LIGHT | int8×4 | % | 0…100 | LED輝度（書き込み専用） |
| 6 | PING | int8 | – | 0…1 | 用途不明 |
| 7 | **ADRAW** | int16 | – | 0…1024 | **10bit ADC 生値（受信振幅と推定）** |
| 8 | CALIB | int8×7 | – | 0…255 | 用途不明 |

**TRAW / ADRAW で DISTL が捨てる弱いエコーを拾えるかは未文書。Phase 1 で実測して判断する。**

### 1-4. 物理（これがボトルを検出できない主因）

標準的な後方散乱断面積（Urick, *Principles of Underwater Sound*, Ch.9）:

- 直円筒（半径 a）: `σ = a·r/2` → 距離依存。エコーは 30·log(r) で減衰
- **二重曲面**（曲率半径 a₁,a₂）: `σ = a₁a₂/4` → **距離に依存しない固定損**
- 小平板（面積 A）: `σ = (A/λ)²`

40mm平板を0.8mで検出できる閾値を基準にすると（λ=8.58mm @40kHz）:

| ターゲット | 予測検出距離 |
|---|---|
| 40mm 平板 | 0.80 m |
| ボトルの**胴**（直円筒、半径33mm） | **0.60 m** |
| ボトルの**肩・首・キャップ・ペタロイド底**（二重曲面） | **0.27 m** |

**実測されている 0.3〜0.4m は二重曲面の予測値と一致する。**
現状、センサはボトル中央よりやや上（キャップ付近）を水平に見ており、
**二重曲面を狙っているために約13dB損している。これはソフトで回復不能。**

→ **最優先はハード修正: センサ取付高さをボトル胴の直円筒部（床から概ね100〜130mm）に下げる。
   これだけで検出距離が約2倍になる見込み。**
   Phase 1 でこれを定量確認する手順を含めること。

なお PET の薄肉は音響的にはほぼ完全反射（40kHz で透過損 約42dB）なので、
「中身が空だから見えない」わけではない。形状（曲率・ラベル・リブ）が原因。

### 1-5. 検出のバースト性はマルチパス干渉

実機ログ `robot_log_20260802_103515.txt`（静止ロガー、26秒）の生シーケンス:

```
-1×52  336,333,332,333,327,339×5   -1×54  303,292,307   -1×52  287×3,295,300,310×4   -1×23  300,312×4   -1×54
```

**ロボット静止・対象静止なのに「約5秒ブラインド → 約0.3〜1秒検出」を繰り返す（有効率 約10%）。**

- 原因はセンサ・床・対象の間の**マルチパス干渉フェージング**。
  経路差 `Δ ≈ 2h²/r`（h=センサ高80mm, r=400mm で Δ≈32mm≈3.7λ）。
  **距離が 5cm 変わるごとに1フェード周期**、λ/4 = **2.15mm の並進でフェードが反転する。**
- 空気の微流動（τ_c ≈ 1秒）がこの位相をゆっくり歩かせるため、バーストになる。
- **重要**: センサは旋回中心から約100mm前方にあるため、**5°の旋回でセンサは約9mm並進する（λ/4の4倍）**。
  つまり**ゆっくり連続旋回すること自体がマルチパス位相を確定的に掃引する**。
  逆に**静止測定はフェードの谷に居座るリスクがある。**
- **角度ディザ単体は垂直円柱に対してほぼ無効**（円柱はヨー不変）。効くのは並進成分。

### 1-6. 統計的な帰結

- 検出は **1-of-M（OR）判定にすること。多数決は絶対に使わない。**
  「エコーあり = 存在の強い証拠」「エコーなし = 不在の弱い証拠」という非対称性がある（占有格子法の標準）。
- 相関時間 τ_c ≈ 1秒 なので、**ドウェルは1秒を超えないと独立サンプルが増えない。**
  現行の `SCAN_SAMPLES=2` × `SAMPLE_TICKS=10` = 200ms は全く足りない。
  実機ログの `scan:` 行が `0/2` ×251回 vs `2/2`+`3/3` ×213回と極端なバイモーダルなのはこのため。

### 1-7. 現行の走査速度は速すぎる

実機ログから実測: 372 wheelDeg を約3.24秒 → **車体角速度 約74°/s**。
ビーム内滞在中のピング数は 10Hz 換算で約9発。
物理的な推奨は**「ターゲットの角度窓に10〜15発以上」→ 車体角速度 30〜40°/s**。

**探索範囲は ±60° に狭めてよい**（運用側で確認済み）。
±60°(=120°) を 35°/s なら片道3.4秒、往復6.9秒。
現行の ±120° / 74°/s の往復6.4秒とほぼ同じ時間予算で、**方位あたりのピング数が約2倍**になる。

## 2. Phase 1: 診断（まずこれだけを実装し、実機で計測する）

**目的**: Phase 2 の設計判断に必要な事実を実機で確定する。この段階では既存の探索ロジックを変更しない。

### 2-1. 新規 `UltrasonicProbeTracer` を作る

`app/UltrasonicProbeTracer.cpp/.h` を新規作成し、`app.cpp` の `generateTracerList()` に登録。
`tracer.ini` 記法:

```
UltrasonicProbeTracer <mode> <sample_count>
```

`<mode>`: `DISTL` | `DISTS` | `TRAW` | `ADRAW` | `ALL`

**要件**:

1. ロボットは走行・旋回しない（毎周期 `mWalker->brake()`）。
2. `pup_device_get_values()` を直接呼び、**`pbio_error_t` と読み取り値を分離してログ出力する**。
   これにより「生値の -1」と「真のドライバエラー」を初めて区別できる。
   - SpikeAPI の `spikeapi::UltrasonicSensor` は `pup_device_t*` を隠蔽している可能性が高い。
     `~/etrobo/spike-rt/drivers/include/libcpp/spike/UltrasonicSensor.h` を読み、
     必要なら `pup_ultrasonic_sensor_get_device(PORT_F)` を app 側で直接呼ぶ。
   - モード enum の正確な名前は
     `~/etrobo/spike-rt/external/libpybricks/lib/pbio/include/pbio/iodev.h`
     （または `lib/lego/device.h`）を grep して確認すること。推測で書かない。
3. `ALL` モードでは DISTL → TRAW → ADRAW を順に読む。
   **モード切替後は最低50ms 待って stale データを捨てること**（読み捨て1回でも可）。
   切替コストが実用に耐えるかを測るため、1周分の所要時間もログに出す。
4. ログ書式（1行1サンプル、後で awk/python で集計しやすい TSV）:
```
   [ULTRA_PROBE]<TAB>seq<TAB>mode<TAB>err<TAB>value<TAB>elapsed_ms
```
5. サンプル間隔は `tracer.ini` から変えられるようにする（既定100ms）。

### 2-2. 実機で取る計測（人間が実施。手順を README か docs に書くこと）

以下を**すべて BluetoothログをPC側で保存**して記録する。各条件30秒以上。

| 実験 | 条件 | 知りたいこと |
|---|---|---|
| E1 | 力士ボトルを 200/300/400/500/600/800mm に置き、**現在の取付高さ（キャップ付近）**で DISTL 静止測定 | 距離 vs 検出率のベースライン |
| E2 | **同じ距離で、センサをボトル胴中央（床から100〜130mm）に下げて** DISTL 静止測定 | 13dB仮説の検証。**最重要** |
| E3 | E2 と同条件で `ALL` モード | **DISTL が -1 のとき TRAW / ADRAW が何を返すか**。ADRAW が連続的な振幅を返すなら自前で閾値を下げられる |
| E4 | 300mm と 500mm で DISTS モード | 短距離モードの方が歩留まりが良いか |
| E5 | 400mm に固定し、ロボットを **前後に5mm ずつ、計±20mm** 動かしながら DISTL | マルチパスのフェード周期が実在するか（λ/4=2.15mm で反転するはず） |
| E6 | ±60° を **35°/s の等速で連続旋回**しながら DISTL（1往復） vs **8°ステップ静止測定**（2秒ドウェル） | 「ゆっくり連続旋回」と「止めて測る」のどちらが検出率が高いか。**現行方針の妥当性判定** |
| E7 | 平板（LEGOプレート40mm角など）を同じ距離に置いて DISTL | ボトル固有の損失量を切り分ける対照実験 |

**E2 と E6 の結果が Phase 2 の設計を決めるので、これを最優先で取ること。**

### 2-3. Phase 1 の完了条件

- 上記ログから、以下を `docs/UltrasonicSensorCharacterization.md` にまとめる:
  - 距離 vs 検出率のグラフ／表（取付高さ2条件）
  - `-1` が pbio エラー由来か生値由来かの結論
  - TRAW / ADRAW が弱エコー検出に使えるかの結論
  - 連続旋回 vs 静止測定 の検出率比較
  - 推奨するセンサ取付高さ

**ここで一度止めて、結果を人間に報告すること。Phase 2 は結果を見てから着手する。**

## 3. Phase 2: 本修正（Phase 1 の結果を反映して実施）

Phase 1 の結果に依存しない部分は先に実装してよい。

### 3-1. 意味論の修正（必須・依存なし）

`app/UltrasonicAlignTracer.h/.cpp`:

- `mMeasurementErrorCount` → `mNoEchoCount` にリネーム。
  コメントの「PUPドライバエラー」を「DISTL の無反射値」に修正。
- ヘッダ冒頭コメントの「999mm前後の『エコーなし』センチネル」を削除・修正。
- `SENSOR_INVALID_MM = 900` を **`SENSOR_MAX_VALID_MM = 2500`**（DISTL の RAW 上限）に変更。
- `readDistanceMm()` を「エコーの有無」と「競技上の採用範囲」の**2段判定**に分離:

```cpp
// 戻り値: 生値。*hasEcho: 物理的にエコーが返ったか。*inRange: 候補として採用する範囲か
int UltrasonicAlignTracer::readDistanceMm(bool* hasEcho, bool* inRange) const
{
    int d = mUltrasonicSensor->getDistance();
    *hasEcho = (d >= MIN_VALID_MM && d < SENSOR_MAX_VALID_MM);
    *inRange = *hasEcho && d <= mMaxDistanceMm;
    return d;
}
```

- **`runApproachMeasure()` の「ブラインドパルス」を削除または厳格に制限する。**
  現行:
```cpp
  if(mMeasurementErrorCount * 2 >= mSampleAttempts) { startPulse(predicted - PULSE_KEEP_MM); }
```
  これは「センサが『何も無い』と言っているのに前進する」動作。`-1` の意味が変わった以上、危険。
  残すなら「**直近100mm以内の走行中に有効測定が1回以上あった場合のみ**」に限定すること。

- **`CLUSTER_ERROR_BRIDGE_WDEG` によるブリッジを見直す。**
  `-1` は「そこに何も無い」なので、無条件にブリッジすると別々の物体を1つに繋いでしまう。
  残す場合は「**ブリッジの前後で距離が `CLUSTER_GAP_MM` 以内に一致すること**」を条件に追加。

### 3-2. 探索アルゴリズムを「方位占有ヒストグラム」に置き換える（必須）

現行のオンライン単一クラスタ（`feedCluster`/`closeCluster`）は、検出率10〜50%の
確率的なセンサに対して脆すぎる。以下に置き換える。

- **固定分解能の方位ビン配列**を持つ。ビン幅 2°（車体角）、範囲 ±90° → 90ビン程度。
  `int mBinHitCount[N]; int mBinDistMm[N][K];`（K=8程度のリングバッファ、またはビンごとの中央値をオンライン更新）
- **往路・復路・複数パスのヒットを同じヒストグラムに累積する**（現行はパスごとにリセットしている）。
- 候補判定は **1-of-M（OR）**: `mBinHitCount[i] >= 1` のビンを「反射あり」とする。
  **多数決・最小ヒット数の閾値は設けない。**
- 「反射あり」ビンの**連続した塊**を1つの対象とみなし、塊の中心角を狙う。
  塊が複数あれば、**塊内の距離の中央値が最小のもの**を選ぶ。
- **距離の代表値は `min` ではなく `median` にする**（現行 `mClusterMinMm` は外れ値に弱い）。
- 現行 `closeCluster()` の「両端の重複値を刈り込んで中心を取る」補正は**残す**。
  センサ更新(約100ms)より速く読むと同一値が尾を引く問題は依然あるため。

### 3-3. 走査パラメータの変更（必須）

- `tracer.ini` を `UltrasonicAlignTracer 60 2000 130` に変更（半角120°→60°、最大距離1000→2000）。
- **走査中の車体角速度を 30〜40°/s に制御する。**
  現行 `driveTurnTo()` は誤差比例PWMで速度制御になっていない。
  走査フェーズ専用に、エンコーダ差分から角速度を求めて目標角速度に追従する P 制御を追加するか、
  最低限 `SCAN_TURN_PWM` を実測で 35°/s になる値に固定する（Phase 1 E6 で実測した値を使う）。
- `SCAN_SAMPLE_INTERVAL_TICKS` は 3(30ms) のままでよいが、
  **「読み取り回数 ≠ ピング回数」であることをコメントに明記**する
  （DISTL の実効更新は約100ms。同一値を3回読んでいる）。
- 候補確認のドウェルは **1.5〜2.0秒（15〜20サンプル）に延長し、1-of-M 判定**にする。
  現行の `SCAN_SAMPLES = 2`（200ms）では相関時間 τ_c≈1秒 に対して独立サンプルが1個しか取れない。
  ただしこれは**候補ビンに対してのみ**行い、全方位には行わない（時間予算のため）。

### 3-4. 探索戦略（Phase 1 E6 の結果で分岐）

**E6 で「ゆっくり連続旋回」が優位だった場合**（物理的にはこちらが有力）:
- 粗探索・確認とも連続旋回に統一し、静止ドウェルは廃止または最小化する。
- 理由: センサは旋回中心の約100mm前方にあり、5°の旋回で約9mm並進する。
  これはマルチパスのフェード周期（λ/4 = 2.15mm）を確定的に掃引するので、
  静止して谷に居座るより有利。この根拠をコード内コメントに残すこと。

**E6 で「静止測定」が優位だった場合**:
- 静止ドウェル中に **前後 ±5mm の並進ディザ**を入れる（2.05 wheelDeg/mm なので ±10 wheelDeg）。
  角度ディザではなく**並進ディザ**であることが重要（垂直円柱はヨー不変なので角度ディザは効かない）。

### 3-5. 接近戦略

- 検出可能圏が約400〜600mm と判明した場合、**`tracer.ini` の運用として
  ScenarioTracer / LineTracer で土俵の手前まで寄せてから UltrasonicAlignTracer を起動する**構成を
  `docs/UltrasonicAlignTracer.md` に推奨手順として明記する。
  円柱エコーは 30·log(r) で増えるので、0.8m→0.4m で +9dB。
- `CREEP_FIRST_MM` / `MAX_CREEP_ATTEMPTS` を Phase 1 の実測検出圏に合わせて調整。

### 3-6. デバッグログの是正（ついでに必ず直すこと）

`2fdec3b` で以下が毎周期出力に変更されており、ライントレース走行時に Bluetooth キューを溢れさせる。
超音波調査用の一時設定と思われるので元に戻すこと。

- `app/Pid.cpp`: `LOGD_EVERY(1, "[PID]...")` → `LOGD_EVERY(200, ...)`
- `unit/LineMonitor.cpp`: `LOGD_EVERY(1, "[LINE_MON]...")` → `LOGD_EVERY(100, ...)`

### 3-7. デッドコードの整理

- `UltrasonicAlignTracer::startPrecisionScan()` は定義のみで呼び出し元が無い。
  `finishScan()` の `else if(mPrecisionScan)` 分岐も `mTargetVerifyScan` が先に評価されるため到達不能。
  ヒストグラム化で不要になるはずなので削除する。
- `CLUSTER_MIN_WIDTH_WDEG` / `CLUSTER_NEAR_BAND_MM` は宣言のみ・参照なし。削除する。

### 3-8. ドキュメント更新

- `docs/UltrasonicAlignTracer.md` の `Walker::runStraight()` は
  `Walker::runWithEncoderCorrection()` の誤り（`b5e6a15` での改名に追随漏れ）。修正すること。
- `-1` の意味、有効レンジ、ヒストグラム方式、推奨センサ高さを反映する。

## 4. やってはいけないこと

- `-1` を「ドライバエラー」として扱うロジックを新たに追加しない。
- 検出判定に**多数決**を使わない（1-of-M の OR にする）。
- 距離が取れない状態で「予測距離」だけを根拠に長距離を前進させない。
- LPF2 モードを毎周期切り替えない（1回あたり最低50msの stale データ破棄が必要）。
- 定数やモード enum 名を推測で書かない。必ず以下のローカルソースを読んで確認する:
  - `~/etrobo/spike-rt/drivers/include/libcpp/spike/UltrasonicSensor.h`
  - `~/etrobo/spike-rt/drivers/include/spike/pup/ultrasonicsensor.h`
  - `~/etrobo/spike-rt/drivers/spike/pup/ultrasonicsensor.c`
  - `~/etrobo/spike-rt/drivers/spike/pup_device.h`
  - `~/etrobo/spike-rt/external/libpybricks/lib/pbio/include/pbio/iodev.h`

## 5. ビルドと検証

```bash
cd ~/etrobo
./make app=EtRobocon2026 sim          # pre-push フックと同じコマンド
```

- 実機ビルド・転送後、`docs/log/` に Bluetooth ログを保存し、
  Phase 1 の各実験について検出率を集計して報告する。
- Phase 2 完了後は、Phase 1 と同じ距離条件で再測定し、**検出率の改善量を数値で示す**こと。

## 6. 進め方

1. **Phase 1 を実装 → 一度止めて人間に報告**（実機計測は人間が行う）
2. 計測結果を受けて Phase 2 の設計を確定 → 実装
3. 各フェーズごとに feature ブランチを切り、コミットを分ける

Phase 1 の実装に入る前に、不明点があれば質問すること。