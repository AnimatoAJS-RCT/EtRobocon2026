etrobo環境のコマンドについては以下を参照
./docs/etrobo.wiki/sim_command_reference.md

# コンパイルコマンド
## シミュレーター
pushd ~/etrobo/
make app=EtRobocon2026 sim
popd

# 運用ルール
## ブランチ名
|ブランチ|役割|ルール|
|-|-|-|
|main|常に動く状態を保つ本流|直接コミット・直接push禁止。PR経由でのみ更新|
|feature/名前-機能名|機能開発用（1人1作業1ブランチ）|mainから作成し、終わったらPRでmainへ|
|fix/名前-バグ名|バグ修正用|同上|
