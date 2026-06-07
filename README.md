# EtRobocon2026

## 初回設定

このセクションには、クローン後に一度だけ必要な設定をまとめます。

### pre-push フックを有効化する

クローン後に一度だけ以下を実行してください。

```bash
git config core.hooksPath .githooks
chmod +x .githooks/pre-push
```

## pre-push フックについて

このリポジトリには Git の `pre-push` フックが `.githooks/pre-push` に含まれています。
このフックは push の前にシミュレータ向けビルドを実行し、ビルドに失敗した場合は push を中断します。

### フックの動作

フックは `etrobo` ルートから次のビルドを実行します。

```bash
make app=EtRobocon2026 sim
```

スクリプトはリポジトリの位置から上位ディレクトリをたどり、`etrobo` ルートを自動で見つけます。

### 一時的に無効化する方法

一度だけフックを無効化して push したい場合は、次のように `--no-verify` を付けてください。

```bash
git push --no-verify
```

ローカルのビルドチェックを意図的にスキップしたい場合にだけ使用してください。
