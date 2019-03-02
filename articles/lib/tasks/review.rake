require 'fileutils'
require 'rake/clean'

BOOK = 'book'
BOOK_PDF = BOOK + '.pdf'
BOOK_EPUB = BOOK + '.epub'
CONFIG_FILE = 'config.yml'
WEBROOT = 'webroot'

def build(mode, chapter)
  sh "review-compile --target=#{mode} --footnotetext --stylesheet=style.css #{chapter} > tmp"
  mode_ext = { 'html' => 'html', 'latex' => 'tex', 'idgxml' => 'xml' }
  FileUtils.mv 'tmp', chapter.gsub(/re\z/, mode_ext[mode])
end

def build_all(mode)
  sh "review-compile --target=#{mode} --footnotetext --stylesheet=style.css"
end

task default: :html_all

desc 'build html (Usage: rake build re=target.re)'
task :html do
  if ENV['re'].nil?
    puts 'Usage: rake build re=target.re'
    exit
  end
  build('html', ENV['re'])
end

desc 'build all html'
task :html_all do
  build_all('html')
end

desc 'preproc all'
task :preproc do
  Dir.glob('*.re').each do |file|
    sh "review-preproc --replace #{file}"
  end
end

desc 'generate PDF and EPUB file'
task all: %i[pdf epub]

desc 'generate PDF file'
task pdf: BOOK_PDF

desc 'generate stagic HTML file for web'
task web: WEBROOT

desc 'generate EPUB file'
task epub: BOOK_EPUB

SRC = FileList['*.re'] + [CONFIG_FILE]

### original
#file BOOK_PDF => SRC do
#  FileUtils.rm_rf [BOOK_PDF, BOOK, BOOK + '-pdf']
#  sh "review-pdfmaker #{CONFIG_FILE}"
#end
file BOOK_PDF => SRC do
  require 'review'
  require 'review/pdfmaker'
  ReVIEW::PDFMaker.class_eval do
    ## コンパイルメッセージを減らすために、uplatexコマンドをバッチモードで起動する。
    ## エラーがあったら、バッチモードにせずに再コンパイルしてエラーメッセージを出す。
    def system_or_raise(*args)
      texcommand = @config['texcommand']   # ex: "uplatex"
      texoptions = @config['texoptions']   # ex: "-halt-on-error -file-line-error"
      latex = "#{texcommand} #{texoptions}"
      if args == ["#{latex} book.tex"]     # when latex command
        ## invoke latex command with batchmode option in order to suppress
        ## compilation message (in other words, to be quiet mode).
        latex += " -interaction=batchmode"
        ok = _run_command("#{latex} book.tex")
        ## latex command with batchmode option doesn't show any error,
        ## therefore we must invoke latex command again without batchmode option
        ## in order to show error.
        return if ok
        $stderr.puts "*\n* latex command failed; retry without batchmode option to show error.\n*"
      end
      _run_command(*args) or raise("failed to run command: #{args.join(' ')}")
    end
    def _run_command(*args)
      puts ""
      puts "[review-pdfmaker]$ #{args.join(' ')}"
      return Kernel.system(*args)
    end
    ## rake pdf && open pdf をするたびに新しいウィンドウが開いてしまうのを防ぐ。
    ## （技術解説：PDFファイルを再生成してもi-nodeが変わらないようにする。）
    #alias __orig_generate_pdf generate_pdf
    #def generate_pdf(yamlfile)
    #  pdffile = pdf_filepath()
    #  _keep_inode(pdffile) { __orig_generate_pdf(yamlfile) }
    #end
    #def _keep_inode(file, suffix="_bkup_")
    #  return yield unless File.file?(file)
    #  bkup = file + suffix
    #  File.rename(file, bkup)      # ex: book.pdf -> book.pdf_bkup_
    #  begin
    #    ret = yield                # generate book.pdf
    #    return ret unless File.file?(file)
    #    binary = File.read(file, binmode: true)  # read from book.pdf
    #    File.write(bkup, binary, binmode: true)  # write into book.pdf_bkup_
    #    File.rename(bkup, file)    # ex: book.pdf_bkup_ -> book.pdf
    #    return ret
    #  rescue
    #    File.unlink(bkup) if File.exist?(bkup)
    #  end
    #end
  end
  #
  FileUtils.rm_rf [BOOK_PDF, BOOK, BOOK + '-pdf']
  begin
    ReVIEW::PDFMaker.execute(CONFIG_FILE)
  rescue RuntimeError => ex
    if ex.message =~ /^failed to run command:/
      abort "*\n* ERROR (review-pdfmaker):\n*  #{ex.message}\n*"
    else
      raise
    end
  end
end

file BOOK_EPUB => SRC do
  FileUtils.rm_rf [BOOK_EPUB, BOOK, BOOK + '-epub']
  sh "review-epubmaker #{CONFIG_FILE}"
end

file WEBROOT => SRC do
  FileUtils.rm_rf [WEBROOT]
  sh "review-webmaker #{CONFIG_FILE}"
end

CLEAN.include([BOOK, BOOK_PDF, BOOK_EPUB, BOOK + '-pdf', BOOK + '-epub', WEBROOT, 'images/_review_math'])
