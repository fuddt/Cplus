#!/bin/bash
# スライドのライブプレビューを起動する（macOS: Finder でダブルクリック）。
# - ブラウザが自動で開く。.md を保存すると自動で更新される。
# - 停止: このウィンドウで Ctrl+C、またはウィンドウを閉じる（preview-stop.command でも止められる）。
# - ポート変更: PORT=8081 ./preview-start.command
cd "$(dirname "$0")" || exit 1

# Finder 起動では PATH が最小限のことがあるため、Homebrew の場所を足す
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

PORT="${PORT:-8080}"
URL="http://localhost:$PORT/"

pause_and_exit() {
  echo
  read -r -p "Enter キーでウィンドウを閉じます" _
  exit "${1:-1}"
}

if ! command -v npm >/dev/null 2>&1; then
  echo "npm が見つかりません。Node.js を入れてください（brew install node）。"
  pause_and_exit 1
fi

# すでに起動中なら、二重に起動せずブラウザだけ開く
if lsof -nP -iTCP:"$PORT" -sTCP:LISTEN >/dev/null 2>&1; then
  echo "ポート $PORT はすでに使われています（起動済みの可能性があります）。ブラウザを開きます: $URL"
  open "$URL"
  echo "止めたいときは preview-stop.command を実行してください。"
  pause_and_exit 0
fi

# 初回（node_modules が無いとき）だけ依存を入れる
if [ ! -d node_modules ]; then
  echo "初回セットアップ: npm ci を実行します..."
  npm ci || { echo "npm ci に失敗しました。"; pause_and_exit 1; }
fi

# サーバーが立ち上がるのを待ってからブラウザを開く（バックグラウンド）
(
  for _ in $(seq 1 60); do
    curl -s -o /dev/null "$URL" && break
    sleep 0.5
  done
  open "$URL"
) &

echo "プレビューを起動します: $URL"
echo "停止: Ctrl+C またはこのウィンドウを閉じる"
echo
PORT="$PORT" exec npm run preview
