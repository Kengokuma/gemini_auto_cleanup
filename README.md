# gemini_auto_cleanup

⚠️ 警告: 実験的かつ危険なコード

このプログラムは、AI (Gemini) の生成したコマンドを検証なしに直接 system() やファイルシステム操作のシステムコールで実行します。悪意のある、または誤ったコマンドが実行されるリスクがあります。本番環境や機密データを含むディレクトリでは絶対に実行しないでください。デモンストレーション目的でのみ使用してください。

### セットアップ

##### gemini cliのインストール

   ```bash
   npm install -g @google/gemini-cli
   ```

##### デモ用ダミーファイルの作成

   ```bash
   cd gemini_auto_cleanup
   chmod +x demo_setup.sh
   bash demo_setup.sh
   ```

### コンパイル

   ```bash
   gcc gemini_auto_cleanup.c -o gemini_auto_cleanup
   ```
### 実行

   ```bash
   ./gemini_auto_cleanup
   ```
### クリーンアップ

   ```bash
   chmod +x demo_clean.sh
   bash demo_clean.sh
   ```
