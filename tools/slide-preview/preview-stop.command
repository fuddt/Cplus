#!/bin/bash
# スライドのライブプレビューを停止する（macOS: Finder でダブルクリック）。
# 指定ポートで待ち受けている marp だけを止める（別のプログラムは止めない）。
# - ポート変更: PORT=8081 ./preview-stop.command
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

PORT="${PORT:-8080}"
pids=$(lsof -nP -iTCP:"$PORT" -sTCP:LISTEN -t 2>/dev/null | sort -u)

if [ -z "$pids" ]; then
  echo "ポート $PORT で動いているプレビューはありません。"
else
  for pid in $pids; do
    if ps -o command= -p "$pid" | grep -q 'marp'; then
      kill "$pid" && echo "プレビューを停止しました（PID $pid）。"
    else
      echo "ポート $PORT を使っているのは marp ではないため止めません（PID $pid: $(ps -o command= -p "$pid" | cut -c1-60)）。"
    fi
  done
fi

if [ -t 0 ]; then
  echo
  read -r -p "Enter キーでウィンドウを閉じます" _
fi
