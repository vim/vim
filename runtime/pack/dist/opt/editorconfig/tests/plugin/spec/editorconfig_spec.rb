require 'vimrunner'

def create_vim(*initial_commands)
  vim = Vimrunner.start
  initial_commands.each do |cmd|
    vim.command cmd
  end
  vim.add_plugin(File.expand_path('../../../..', __FILE__), 'plugin/editorconfig.vim')
  return vim
end

# The base path of the testing files
BASE_PATH = File.expand_path('../plugin_tests/test_files/', __FILE__)

# file_name is the file name that should be open by Vim
# expected_values is a Hash that contains all the Vim options we need to test
def test_editorconfig(vim, file_name, expected_values)
  vim.edit(File.join(BASE_PATH, file_name))

  expected_values.each do |key, val|
    vimval = vim.echo("&l:#{key}")
    expect(vimval).to eq(val), "key #{key} had value #{vimval}, but I expected #{val}"
  end

  vim.command 'bd!'
end

def test_instance(vim)
  describe 'plugin/editorconfig.vim' do
    after(:all) do
      vim.kill
    end

    describe '#all' do
      it '3_space.py' do
        test_editorconfig vim, '3_space.txt',
          expandtab: '1',
          shiftwidth: '3',
          tabstop: '3'
      end
    end

    it '4_space.py' do
      test_editorconfig vim, '4_space.py',
        expandtab: '1',
        shiftwidth: '4',
        tabstop: '8'
    end

    it 'space.txt' do
      test_editorconfig vim, 'space.txt',
        expandtab: '1',
        shiftwidth: vim.echo('&l:tabstop')
    end

    it 'tab.txt' do
      test_editorconfig vim, 'tab.txt',
        expandtab: '0'
    end

    it '4_tab.txt' do
      test_editorconfig vim, '4_tab.txt',
        expandtab: '0',
        shiftwidth: '4',
        tabstop: '4'
    end

    it '4_tab_width_of_8' do
      test_editorconfig vim, '4_tab_width_of_8.txt',
        expandtab: '0',
        shiftwidth: '4',
        tabstop: '8'
    end

    it 'lf.txt' do
      test_editorconfig vim, 'lf.txt',
        fileformat: 'unix'
    end

    it 'crlf.txt' do
      test_editorconfig vim, 'crlf.txt',
        fileformat: 'dos'
    end

    it 'cr.txt' do
      test_editorconfig vim, 'cr.txt',
        fileformat: 'mac'
    end

    it 'utf-8.txt' do
      test_editorconfig vim, 'utf-8.txt',
        fileencoding: 'utf-8',
        bomb: '0'
    end

    it 'utf-8-bom.txt' do
      test_editorconfig vim, 'utf-8-bom.txt',
        fileencoding: 'utf-8',
        bomb: '1'
    end

    it 'utf-16be.txt' do
      test_editorconfig vim, 'utf-16be.txt',
        fileencoding: 'utf-16'
    end

    it 'utf-16le.txt' do
      test_editorconfig vim, 'utf-16le.txt',
        fileencoding: 'utf-16le'
    end

    it 'latin1.txt' do
      test_editorconfig vim, 'latin1.txt',
        fileencoding: 'latin1'
    end

    # insert_final_newline by PreserveNoEOL tests are omitted, since they are not supported
    if vim.echo("exists('+fixendofline')") == '1'
      it 'with_newline.txt' do
        test_editorconfig vim, 'with_newline.txt',
          fixendofline: '1'
      end

      it 'without_newline.txt' do
        test_editorconfig vim, 'without_newline.txt',
          fixendofline: '0'
      end
    end
  end
end

# Test the vim core
(lambda do
  puts 'Testing default'
  vim = create_vim
  test_instance vim
end).call

# Test the vim core with an express setting
(lambda do
  puts 'Testing with express vim_core mode'
  vim = create_vim("let g:EditorConfig_core_mode='vim_core'")
  test_instance vim
end).call

# Test with external-core mode, but no external core defined
(lambda do
  puts 'Testing with fallback to vim_core mode'
  vim = create_vim("let g:EditorConfig_core_mode='external_command'")
  test_instance vim
end).call

# Test with an external core, if desired
extcore = ENV['EDITORCONFIG_VIM_EXTERNAL_CORE']
if extcore
  puts "Testing with external_command #{extcore}"
  vim = create_vim(
    "let g:EditorConfig_core_mode='external_command'",
    "let g:EditorConfig_exec_path='#{extcore}'",
  )
  test_instance vim
end

# Test the vim core with latin1 encoding
(lambda do
  puts 'Testing with express vim_core mode'
  vim = create_vim("set encoding=latin1")
  test_instance vim
end).call
