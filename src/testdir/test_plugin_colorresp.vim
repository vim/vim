" Test for the colorresp plugin

CheckNotGui
CheckUnix

runtime plugin/colorresp.vim

func Test_colorresp()
    set t_RF=x
    set t_RB=y

    " response to t_RF, 4 digits
    let red = 0x12
    let green = 0x34
    let blue = 0x56
    let seq = printf("\<Esc>]10;rgb:%02x00/%02x00/%02x00\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrfgresp)
    " call WaitForAssert({-> assert_equal(seq, v:termrfgresp)})

    " response to t_RF, 2 digits
    let red = 0x78
    let green = 0x9a
    let blue = 0xbc
    let seq = printf("\<Esc>]10;rgb:%02x/%02x/%02x\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrfgresp)

    " response to t_RB, 4 digits, dark
    set background=light
    call test_option_not_set('background')
    let red = 0x29
    let green = 0x4a
    let blue = 0x6b
    let seq = printf("\<Esc>]11;rgb:%02x00/%02x00/%02x00\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrbgresp)
    call assert_equal('dark', &background)

    " response to t_RB, 4 digits, light
    set background=dark
    call test_option_not_set('background')
    let red = 0x81
    let green = 0x63
    let blue = 0x65
    let seq = printf("\<Esc>]11;rgb:%02x00/%02x00/%02x00\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrbgresp)
    call assert_equal('light', &background)

    " response to t_RB, 2 digits, dark
    set background=light
    call test_option_not_set('background')
    let red = 0x47
    let green = 0x59
    let blue = 0x5b
    let seq = printf("\<Esc>]11;rgb:%02x/%02x/%02x\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrbgresp)
    call assert_equal('dark', &background)

    " response to t_RB, 2 digits, light
    set background=dark
    call test_option_not_set('background')
    let red = 0x83
    let green = 0xa4
    let blue = 0xc2
    let seq = printf("\<Esc>]11;rgb:%02x/%02x/%02x\x07", red, green, blue)
    call feedkeys(seq, 'Lx!')
    call assert_equal(seq, v:termrbgresp)
    call assert_equal('light', &background)

    set t_RF= t_RB=
  endfunc
