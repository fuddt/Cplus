// プレビュー専用の Marp エンジン。
// Marpit は `*` / `)` で始まる箇条書きを「1クリック1項目の段階表示」にするが、
// 研修スライドは全ファイルが `*` を使うため、プレビューでは無効にして全項目を最初から表示する。
// スライド本文（Markdown）は変更しない。PPTX 出力には影響しない。
import { Marp } from '@marp-team/marp-core'

export default (constructorOptions) => {
  const marp = new Marp(constructorOptions)
  marp.markdown.core.ruler.disable('marpit_fragment')
  return marp
}
