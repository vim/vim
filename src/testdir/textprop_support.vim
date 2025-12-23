vim9script

var py_path = expand('<sfile>:p:h') .. '/textprop.py'
execute 'py3file ' .. py_path

export class BufferState
    var handle: number

    def CheckBufferContent(msg: string = ''): number
        var args = {'handle': this.handle, 'msg': msg}
        py3eval('tp_check_buffer_content(handle, msg)', args)
        return 0
    enddef

    def DeleteText(lnum: number, col: number, count: number): number
        var args = {
            'handle': this.handle, 'lnum': lnum, 'col': col, 'count': count}
        py3eval('tp_delete_text(handle, lnum, col, count)', args)
        return 0
    enddef

    def InsertText(lnum: number, col: number, text: string): number
        var args = {
            'handle': this.handle, 'lnum': lnum, 'col': col, 'text': text}
        py3eval('tp_insert_text(handle, lnum, col, text)', args)
        return 0
    enddef

    def RemovePropertyFromLine(lnum: number, type_name: string): number
        var args = {
            'handle': this.handle, 'lnum': lnum, 'type_name': type_name}
        py3eval(
            'tp_remove_property_from_line(handle, lnum, type_name)', args)
        return 0
    enddef

    def Apply(): number
        var args = {'handle': this.handle}
        py3eval('tp_apply_buffer(handle)', args)
    enddef

    def DeletePropertyTypes(): number
        var args = {'handle': this.handle}
        py3eval('tp_delete_property_types(handle)', args)
        return 0
    enddef

    def ExpectedPropList(lnum: number): list<dict<any>>
        var args = {
            'handle': this.handle, 'lnum': lnum}
        return py3eval(
            'tp_expected_prop_list(handle, lnum)', args)
    enddef

    def ExpectedPropForType(lnum: number, type_name: string): dict<any>
        var args = {
            'handle': this.handle, 'lnum': lnum, 'type_name': type_name}
        return py3eval(
            'tp_expected_prop_for_type(handle, lnum, type_name)', args)
    enddef

    def Dump(): number
        var args = {'handle': this.handle}
        py3eval('tp_dump_state(handle)', args)
        return 0
    enddef

endclass

export def LoadBufferSpec(
        definition: list<string>, apply: bool = true): BufferState

    var args: dict<any> = {'spec': definition, 'apply': apply}
    var handle = py3eval('tp_load_buffer_spec(spec, apply)', args)
    return BufferState.new(handle)
enddef

export def CheckBufferContent(handle: number)
    var args = {'handle': handle}
    py3eval('tp_check_buffer_content(handle)', args)
enddef

export def DeleteText(
        handle: number, lnum: number, col: number, count: number)
    var args = {'handle': handle, 'lnum': lnum, 'col': col, 'count': count}
    py3eval('tp_delete_text(handle, lnum, col, count)', args)
enddef

export def FormatErrors()
    var args = {'errors': v:errors}
    py3eval('tp_format_errors(errors)', args)
enddef

export def DumpBuffer(handle: number)
    var args = {'handle': handle}
    py3eval('tp_dump_state(handle)', args)
enddef

export def ApplyBuffer(handle: number)
    var args = {'handle': handle}
    py3eval('tp_apply_buffer(handle)', args)
enddef
