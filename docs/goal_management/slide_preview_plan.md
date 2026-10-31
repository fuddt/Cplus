# スライド・リアルタイムプレビュー計画書

この計画書は、別セッション（ローカルMacの Claude Code）に作業を引き継ぐための、自己完結した依頼書である。会話の履歴がなくても、この文書だけで背景・方針・手順が分かるようにしてある。

## 1. 背景と目的

- 対象は `docs/goal_management/slides/*.md`（現在11ファイル、各約27枚）。研修用スライドを Markdown で書いている。
- 各ファイルの冒頭は Marp 形式の front matter（`title` / `footer` / `theme` / `pptx_name`）で、スライド区切りは `---`。`pptx_name` は Marp の標準項目ではなく、出力ファイル名として使う想定の独自項目。
- 最終的には **Googleスライドで社内共有**する。ローカルで作った PPTX を取り込むと崩れる懸念があり、取り込み後に Googleスライド上で少し編集することも想定している。
- 実現したいこと: 内容がまだ未完成の段階で、変換作業を毎回せずに「スライド化したらどう見えるか」を確認する。**Markdown を編集して保存すると、プレビューがリアルタイムで更新される**こと。
- PPTX 化の方法は、まだ決まっていない（ローカルに変換スクリプトは存在しない）。

## 2. 要点: 問題の本質は変換経路

| 変換経路 | Googleスライドでの結果 |
|---|---|
| Marp 標準の PPTX 出力 | 各スライドが1枚の画像になる。見た目は一致するが文字を編集できない。今回の用途には合わない |
| Marp の `--pptx-editable` | 編集は可能だが実験的機能。LibreOffice が必要で、フォントや余白がずれやすい |
| 自作の PPTX 出力（PptxGenJS 等） | 文字・図形が実体として入るので編集できる。座標とフォントを固定すれば崩れにくい |
| Slides API で直接生成 | 変換による崩れがなくなる。ただし認証設定が必要で、規模が大きい |

方針: **Markdown を唯一の原本にし、1つの「スライドモデル」から、HTMLプレビューと PPTX の両方を同じレイアウトルールで生成する。** プレビューと最終成果物のずれを、構造的に小さくする。

## 3. 現状の観察（調査済みの事実）

- 第3回 `03_binary_manipulation_and_struct_view.md` は、1枚あたりの最大が **36行**。16:9 の1枚には収まらない可能性が高く、**はみ出し検出**がプレビューの主要な価値になる。
- コードブロックが多い（第1回で48ブロック、第3回で53ブロック）。コードの見え方が変換品質を左右する。
- 表は少ない（数行程度）。画像と Mermaid は、確認した範囲では使われていない。
- `05_what_is_dll_and_linker.md` と `06_os_basics_and_event_loop.md` は、6回構成時代の旧版。新版（`05_compiler_and_linker.md` / `06_dll_and_program_boundary.md`）に置き換わっているが残っている。プレビューの対象から外すか、一覧で区別して表示する。
- スライド教材の作成規約は `AGENTS.md` と `docs/goal_management/slide_design_guidelines.md` にある。**スライドの内容そのものは、依頼されない限り変更しない。**

## 4. 進め方

### Step 0: 変換経路の実験（最初に実施し、結果をユーザーへ報告して判断を仰ぐ）

1. `tools/slide-preview/experiments/sample3.md` を作る。第3回から代表的な3枚を、元の front matter ごと抜き出す。
   - タイトルスライド
   - 箇条書きとコードブロックを含むスライド
   - 表を含むスライド（なければ、表を含む別ファイルのスライドで代用）
2. 次の3方式で PPTX を作る。

```bash
# (a) Marp 標準（各スライドが画像になる）。Chrome または Edge が必要
npx @marp-team/marp-cli@latest tools/slide-preview/experiments/sample3.md \
  --pptx -o tools/slide-preview/experiments/out/a_marp_image.pptx

# (b) Marp の編集可能出力（実験的機能）。LibreOffice が必要: brew install --cask libreoffice
npx @marp-team/marp-cli@latest tools/slide-preview/experiments/sample3.md \
  --pptx --pptx-editable -o tools/slide-preview/experiments/out/b_marp_editable.pptx
```

3. (c) PptxGenJS で同じ3枚を作る試作スクリプト `tools/slide-preview/experiments/make_c.js` を書く。固定座標のテキストボックス、フォントは Noto Sans JP（本文）と Roboto Mono（コード）、自動縮小は使わない。
4. **ここで作業を止める。** 3つの PPTX を、ユーザーが Googleスライドに取り込んで比較する。Claude は Googleスライドを操作できない。ユーザーに確認してほしい観点は次のとおり。
   - 文字を編集できるか
   - コードブロックの背景・等幅フォントが保たれているか
   - 余白・折り返し・はみ出しが、どう崩れたか

### Step 1: プレビューアプリ（`tools/slide-preview/` に隔離）

- 構成: Node.js、ファイル監視に `chokidar`、ブラウザへの更新通知に WebSocket（`ws`）、解析に `markdown-it`。
- 機能:
  - `docs/goal_management/slides/*.md` の一覧表示と切り替え（旧版2ファイルは区別して表示）
  - 16:9 固定キャンバスでの表示
  - 全スライドのサムネイル一覧
  - Markdown 保存から約1秒での自動更新
  - **はみ出し警告**（行数と文字数からの推定。しきい値は設定ファイルで変更可能にする）
- スライドモデルは、HTML と PPTX の共通入力にする（骨子は次のとおり）。

```js
// 1) Markdown を「スライドモデル」に変換する（HTML と PPTX の共通入力）
//    front matter を読み、`---` でスライドごとに分割する
const deck = parseMarkdown(fs.readFileSync(file, 'utf8'));

// 2) プレビュー用: 同じモデルから HTML を作り、はみ出しも検出する
const html = renderHtml(deck);             // 16:9 の固定キャンバス
const warnings = detectOverflow(deck);     // 行数・文字数から溢れを推定する

// 3) 変更検知: 保存されたら再パースして全ブラウザへ通知する
chokidar.watch(slidesDir + '/*.md').on('change', (f) => {
  const d = parseMarkdown(fs.readFileSync(f, 'utf8'));
  wss.clients.forEach((c) => c.send(JSON.stringify({ file: f, html: renderHtml(d), warnings: detectOverflow(d) })));
});
```

### Step 2: PPTX 出力（Step 0 の判断結果に従う）

- 同じスライドモデルから、固定座標のテキストボックスとして PPTX を書き出す。出力ファイル名は front matter の `pptx_name` を使う。
- 崩れ対策:
  - フォントは Googleスライドに標準で入っている Noto Sans JP と Roboto Mono に固定する
  - 自動縮小（autofit）は使わない
  - 表は小さく保つ
  - 画像と Mermaid は初期対応の対象外とする（使われていないため）

### Step 3（任意）: Slides API による直接生成

Step 2 でもまだ崩れる場合のみ検討する。認証設定が必要になるため、着手前にユーザーへ確認する。

## 5. 運用ルール

- Markdown が原本。Googleスライドでの手直しは**最後の仕上げだけ**にする（再生成すると手直しが消えるため）。
- 1枚あたりの行数の上限を、`slide_design_guidelines.md` に追記する候補とする。目安は本文18行程度。ただし、既存スライドの内容を削る作業は本計画の範囲外。追記する前にユーザーへ確認する。

## 6. 制約と注意

- `tools/slide-preview/` は独立した Node プロジェクトにする（独自の `package.json`）。C++ 本体の CMake ビルドには触れない。`CLAUDE.md` の「外部依存なし」は C++ 本体の話であり、`tools/` は対象外。
- **ブランチ運用の確認が必要。** `AGENTS.md` には「作業完了時は `git push origin main`」とあるが、これはスライド教材の作成を想定した規約。ツール開発では、まずユーザーに「feature ブランチで作業して良いか、main へ直接 push するか」を確認する。確認が取れるまで main へは push しない。
- Googleスライドへの取り込みと最終的な見た目の判断は、ユーザーが行う。Claude は Googleスライドを操作できない。

## 7. 検証（完了条件）

1. `node tools/slide-preview/server.js` で起動し、ブラウザでスライドが表示される。
2. Markdown を編集して保存すると、約1秒でプレビューが更新される。
3. 36行のスライドで、はみ出し警告が出る。
4. （Step 2 実施後）書き出した PPTX を、ユーザーが Googleスライドに取り込み、プレビューとの差を確認する。
5. 取り込み後に、文字を編集できる。
