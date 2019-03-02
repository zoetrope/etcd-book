# -*- coding: utf-8 -*-

##
## change ReVIEW source code
##

module ReVIEW

  ## ブロック命令
  Compiler.defblock :terminal, 0..3     ## ターミナル

  Book::ListIndex.class_eval do

    ## '//terminal[][]{ ... //}' をサポートするよう拡張
    def self.item_type  # override
      #'(list|listnum)'            # original
      '(list|listnum|terminal)'
    end

    ## '//terminal' のラベル（第1引数）を省略できるよう拡張
    def self.parse(src, *args)  # override
      items = []
      seq = 1
      src.grep(%r{\A//#{item_type}}) do |line|
        type_name = $1                                           #+
        if id = line.slice(/\[(.*?)\]/, 1)
          if type_name == 'terminal'                             #+
            items.push item_class.new(id, seq) unless id.empty?  #+
            seq += 1                           unless id.empty?  #+
          else                                                   #+
            items.push item_class.new(id, seq)
            seq += 1
            ReVIEW.logger.warn "warning: no ID of #{item_type} in #{line}" if id.empty?
          end                                                    #+
        end
      end
      new(items, *args)
    end

  end


  Book::Compilable.class_eval do

    def content   # override
      ## //list[?] や //terminal[?] の '?' をランダム文字列に置き換える。
      ## こうすると、重複しないラベルをいちいち指定しなくても、ソースコードや
      ## ターミナルにリスト番号がつく。ただし @<list>{} での参照はできない。
      unless @_done
        pat = Book::ListIndex.item_type  # == '(list|listnum|terminal)'
        @content = @content.gsub(/^\/\/#{pat}\[\?\]/) { "//#{$1}[#{_random_label()}]" }
        @_done = true
      end
      @content
    end

    def _random_label
      "_" + rand().to_s[2..10]
    end

  end


  Catalog.class_eval do

    def parts_with_chaps
      ## catalog.ymlの「CHAPS:」がnullのときエラーになるのを防ぐ
      (@yaml['CHAPS'] || []).flatten.compact
    end

  end


  class LATEXBuilder

    TERMINAL_OPTIONS = {
      :fold => true,
    }

    ## ターミナル用ブロック命令
    ## ・//cmd と似ているが、長い行を自動的に折り返す
    ## ・seqsplit.styの「\seqsplit{...}」コマンドを使っている
    def terminal(lines, id=nil, caption=nil, optionstr=nil)
      opt_fold = TERMINAL_OPTIONS[:fold]
      opt_fold = _parse_terminal_optionstr(optionstr) if optionstr.present?
      ## 折り返し用をするLaTeXコマンドを追加
      if opt_fold && !lines.empty?
        lines[0]  = "\\seqsplit{" + lines[0]
        lines[-1] = lines[-1] + "}"
      end
      #
      blank
      ## common_code_block() を上書きせずに済ますためのハック
      command = 'starterterminal'
      if !id.present? && caption.present?      # ラベルはないが、キャプションはある場合
        ## common_code_block()では、idなしでキャプションを出力できるのは
        ## 一部のブロックコマンドに限定されている（中でhard codingされている）。
        ## そのため、common_code_block()を上書きせずにidなしでキャプションを
        ## 出力するには、自前で出力するしかない。
        _with_status(:caption) do
          puts macro(command + 'caption', compile_inline(caption))
        end
        caption = ""     # 自前で出力したので、もうキャプションは出力しない
      elsif id.present? && !caption.present?   # ラベルはあるが、キャプションはない場合
        ## common_code_block()の仕様により、idがあってもcaptionが空だと、
        ## キャプション自体が出力されない（でも「リストX.X:」は出力させたい）。
        ## これを回避するために、「空じゃないけど出力時には空になる」文字列を使う。
        caption = "@<letitgo>{}"  # compile_inline(caption) をしたら空になる文字列
      end
      ##
      lang = nil
      common_code_block(id, lines, command, caption, lang) {|line, _| detab(line) + "\n" }
    end

    private

    def _parse_terminal_optionstr(optionstr)  # parse 'fold={on|off}'
      vals = {nil=>true, 'on'=>true, 'off'=>false}
      opt_fold = nil
      optionstr.split(',').each do |x|
        x = x.strip()
        if x =~ /\Afold(?:=(.*))?\z/
          vals.key?($1)  or
            raise "//terminal[][][#{x}]: expected 'on' or 'off'."
          opt_fold = vals[$1]
        else
          raise "//terminal[][][#{x}]: unknown option."
        end
      end
      return opt_fold
    end

    def _with_status(key, val=true)
      prev = @doc_status[key]
      @doc_status[key] = val
      yield
      @doc_status[key] = prev
    end

    public

    ## ・\caption{} のかわりに \reviewimagecaption{} を使うよう修正
    ## ・「scale=X」に加えて「pos=X」も受け付けるように拡張
    def image_image(id, caption, metric)
      pos, metric = _detect_image_pos(metric)  # detect 'pos=H' or 'pos=h'
      pos.nil? || pos =~ /\A[Hhtb]+\z/  or  # H: Here, h: here, t: top, b: bottom
        raise "//image[][][pos=#{pos}]: expected 'pos=H' or 'pos=h'."
      metrics = parse_metric('latex', metric)
      puts "\\begin{reviewimage}[#{pos}]%%#{id}" if pos
      puts "\\begin{reviewimage}%%#{id}"         unless pos
      metrics = "width=\\maxwidth" unless metrics.present?
      puts "\\includegraphics[#{metrics}]{#{@chapter.image(id).path}}"
      _with_status(:caption) do
        #puts macro('caption', compile_inline(caption)) if caption.present?  # original
        puts macro('reviewimagecaption', compile_inline(caption)) if caption.present?
      end
      puts macro('label', image_label(id))
      puts "\\end{reviewimage}"
    end

    private

    def _detect_image_pos(metric)  # detect 'pos=H' or 'pos=h' in metric string
      pos = nil; xs = []
      metric.split(',').each do |x|
        x = x.strip
        x =~ /\Apos=(\S*?)\z/ ? (pos = $1) : (xs << x)
      end if metric
      metric = xs.join(",") if pos
      return pos, metric
    end

  end

  class PDFMaker

    ### original: 2.4, 2.5
    #def call_hook(hookname)
    #  return if !@config['pdfmaker'].is_a?(Hash) || @config['pdfmaker'][hookname].nil?
    #  hook = File.absolute_path(@config['pdfmaker'][hookname], @basehookdir)
    #  if ENV['REVIEW_SAFE_MODE'].to_i & 1 > 0
    #    warn 'hook configuration is prohibited in safe mode. ignored.'
    #  else
    #    system_or_raise("#{hook} #{Dir.pwd} #{@basehookdir}")
    #  end
    #end
    ### /original

    def call_hook(hookname)
      if ENV['REVIEW_SAFE_MODE'].to_i & 1 > 0
        warn 'hook configuration is prohibited in safe mode. ignored.'
        return
      end
      d = @config['pdfmaker']
      return if d.nil? || !d.is_a?(Hash) || d[hookname].nil?
      ## hookname が文字列の配列なら、それらを全部実行する
      [d[hookname]].flatten.each do |hook|
        script = File.absolute_path(hook, @basehookdir)
        ## 拡張子が .rb なら、rubyコマンドで実行する（ファイルに実行属性がなくてもよい）
        if script.end_with?('.rb')
          ruby = ruby_fullpath()
          ruby = "ruby" unless File.exist?(ruby)
          system_or_raise("#{ruby} #{script} #{Dir.pwd} #{@basehookdir}")
        else
          system_or_raise("#{script} #{Dir.pwd} #{@basehookdir}")
        end
      end
    end

    private
    def ruby_fullpath
      require 'rbconfig'
      c = RbConfig::CONFIG
      return File.join(c['bindir'], c['ruby_install_name']) + c['EXEEXT'].to_s
    end

  end

end
