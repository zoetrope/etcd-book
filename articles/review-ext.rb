# -*- coding: utf-8 -*-

##
## Re:VIEWを拡張し、インライン命令とブロック命令を追加する
##


module ReVIEW

  ## インライン命令「@<clearpage>{}」を宣言
  Compiler.definline :clearpage         ## 改ページ
  Compiler.definline :letitgo           ## 引数をそのまま表示
  Compiler.definline :B                 ## @<strong>{}のショートカット
  Compiler.definline :hearts            ## ハートマーク
  Compiler.definline :TeX               ## TeX のロゴマーク
  Compiler.definline :LaTeX             ## LaTeX のロゴマーク

  ## ブロック命令「//textleft{ ... //}」等を宣言
  ## （ここでは第2引数が「0」なので、引数なしのブロック命令になる。
  ##   もし第2引数が「1..3」なら、//listのように必須引数が1つで
  ##   非必須引数が2という意味になる。）
  Compiler.defblock :textleft, 0        ## 左寄せ
  Compiler.defblock :textright, 0       ## 右寄せ
  Compiler.defblock :textcenter, 0      ## 中央揃え
  Compiler.defblock :abstract, 0        ## 章の概要


  ## LaTeX用の定義
  class LATEXBuilder

    ## 改ページ
    def inline_clearpage(str)
      '\clearpage'
    end

    ## 引数をそのまま表示
    ## 例：
    ##   //emlist{
    ##     @<b>{ABC}             ← 太字の「ABC」が表示される
    ##     @<letitgo>$@<b>{ABC}$ ← 「@<b>{ABC}」がそのまま表示される
    ##   //}
    def inline_letitgo(str)
      str
    end

    ## @<strong>{} のショートカット
    def inline_B(str)
      inline_strong(str)
    end

    ## ハートマーク
    def inline_hearts(str)
      '$\heartsuit$'
    end

    ## TeXのロゴマーク
    def inline_TeX(str)
      '\TeX{}'
    end

    ## LaTeXのロゴマーク
    def inline_LaTeX(str)
      '\LaTeX{}'
    end

    ## 左寄せ
    def textleft(lines)
      puts '\begin{flushleft}'
      puts lines
      puts '\end{flushleft}'
    end

    ## 右寄せ
    ## （注：Re:VIEWにはすでに //flushright{ ... //} があったので、今後はそちらを推奨）
    def textright(lines)
      puts '\begin{flushright}'
      puts lines
      puts '\end{flushright}'
    end

    ## 中央揃え
    ## （注：Re:VIEWにはすでに //centering{ ... //} があったので、今後はそちらを推奨）
    def textcenter(lines)
      puts '\begin{center}'
      puts lines
      puts '\end{center}'
    end

    ## 導入文（//lead{ ... //}）のデザインをLaTeXのスタイルファイルで
    ## 変更できるよう、マクロを使う。
    def lead(lines)
      puts '\begin{starterlead}'   # オリジナルは \begin{quotation}
      puts lines
      puts '\end{starterlead}'
    end

    ## 章 (Chapter) の概要
    ## （導入文 //lead{ ... //} と似ているが、導入文では詩や物語を
    ##   引用するのが普通らしく、概要 (abstract) とは違うみたいなので、
    ##   概要を表すブロックを用意した。）
    def abstract(lines)
      puts '\begin{starterabstract}'
      puts lines
      puts '\end{starterabstract}'
    end

    ## 引用（複数段落に対応）
    def quote(lines)
      puts '\begin{starterquote}'
      puts lines
      puts '\end{starterquote}'
    end

    ## 引用 (====[note] ... ====[/note])
    ## （ブロック構文ではないので、中に箇条書きや別のブロックを入れられる）
    def quote_begin(level, label, caption)
      puts '\begin{starterquote}'
    end
    def quote_end(level)
      puts '\end{starterquote}'
    end

    ## ノート
    def note(lines, caption=nil)
      s = compile_inline(caption || "")
      puts "\\begin{starternote}{#{s}}"
      puts lines
      puts "\\end{starternote}"
    end

    ## ノート (====[note] ... ====[/note])
    ## （ブロック構文ではないので、中に箇条書きや別のブロックを入れられる）
    def note_begin(level, label, caption)
      s = compile_inline(caption || "")
      puts "\\begin{starternote}{#{s}}"
    end
    def note_end(level)
      puts "\\end{starternote}"
    end

  end


  ## HTML（ePub）用の定義
  class HTMLBuilder

    ## 改ページはHTMLにはない
    def inline_clearpage(str)
      puts '<p></p>'
      puts '<p></p>'
    end

    ## 引数をそのまま表示
    def inline_letitgo(str)
      str
    end

    ## @<strong>{} のショートカット
    def inline_B(str)
      inline_strong(str)
    end

    ## ハートマーク
    def inline_hearts(str)
      '&hearts;'
    end

    ## TeXのロゴマーク
    def inline_TeX()
      'TeX'
    end

    ## LaTeXのロゴマーク
    def inline_LaTeX()
      'LaTeX'
    end

    ## 左寄せ
    def textleft(lines)
      puts '<div style="text-align:left">'
      puts lines
      puts '</div>'
    end

    ## 右寄せ
    def textright(lines)
      puts '<div style="text-align:right">'
      puts lines
      puts '</div>'
    end

    ## 中央揃え
    def textcenter(lines)
      puts '<div style="text-align:center">'
      puts lines
      puts '</div>'
    end

    ## 章 (Chapter) の概要
    def abstract(lines)
      puts '<div class="abstract">'
      puts lines
      puts '</div>'
    end

    ## 引用（複数段落に対応）
    def blockquote(lines)
      puts '<blockquote class="blockquote">'
      puts lines
      puts '</blockquote>'
    end

    ## 引用 (====[note] ... ====[/note])
    ## （ブロック構文ではないので、中に別のブロックや箇条書きを入れられる）
    def quote_begin(level, label, caption)
      puts '<blockquote class="blockquote">'
    end
    def quote_end(level)
      puts '</blockquote>'
    end

    ## ノート
    def note(lines, caption=nil)
      s = compile_inline(caption || "")
      puts "<div class=\"note\">"
      puts "<h5>#{s}</h5>" if s.present?
      puts lines
      puts "</div>"
    end

    ## ノート (====[note] ... ====[/note])
    ## （ブロック構文ではないので、中に別のブロックや箇条書きを入れられる）
    def note_begin(level, label, caption)
      s = compile_inline(caption || "")
      puts "<div class=\"note\">"
      puts "<h5>#{s}</h5>" if s.present?
    end
    def note_end(level)
      puts "</div>"
    end

  end


end


##
## Re:VIEW のソースコードを実行時に修正する（モンキーパッチ）
##
require_relative './lib/hooks/monkeypatch'
