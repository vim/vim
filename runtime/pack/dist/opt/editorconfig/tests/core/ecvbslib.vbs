' ecvbslib.vbs: VBScript routines for use in
' editorconfig-core-vimscript and editorconfig-vim.
' Copyright (c) 2018--2019 Chris White.  All rights reserved.
' Licensed CC-BY-SA, version 3.0 or any later version, at your option.

' Remove CR and LF in a string
function nocrlf(strin)
    nocrlf = Replace(Replace(strin, vbCr, ""), vbLf, "")
end function

' === Base64 ================================================================
' from https://stackoverflow.com/a/40118072/2877364 by
' https://stackoverflow.com/users/45375/mklement0

' Base64-encodes the specified string.
' Parameter fAsUtf16LE determines how the input text is encoded at the
' byte level before Base64 encoding is applied.
' * Pass False to use UTF-8 encoding.
' * Pass True to use UTF-16 LE encoding.
Function Base64Encode(ByVal sText, ByVal fAsUtf16LE)

    ' Use an aux. XML document with a Base64-encoded element.
    ' Assigning the byte stream (array) returned by StrToBytes() to .NodeTypedValue
    ' automatically performs Base64-encoding, whose result can then be accessed
    ' as the element's text.
    With CreateObject("Msxml2.DOMDocument").CreateElement("aux")
        .DataType = "bin.base64"
        if fAsUtf16LE then
            .NodeTypedValue = StrToBytes(sText, "utf-16le", 2)
        else
            .NodeTypedValue = StrToBytes(sText, "utf-8", 3)
        end if
        Base64Encode = nocrlf(.Text)    ' No line breaks; MSXML adds them.
    End With

End Function

' Decodes the specified Base64-encoded string.
' If the decoded string's original encoding was:
' * UTF-8, pass False for fIsUtf16LE.
' * UTF-16 LE, pass True for fIsUtf16LE.
Function Base64Decode(ByVal sBase64EncodedText, ByVal fIsUtf16LE)

    Dim sTextEncoding
    if fIsUtf16LE Then sTextEncoding = "utf-16le" Else sTextEncoding = "utf-8"

    ' Use an aux. XML document with a Base64-encoded element.
    ' Assigning the encoded text to .Text makes the decoded byte array
    ' available via .nodeTypedValue, which we can pass to BytesToStr()
    With CreateObject("Msxml2.DOMDocument").CreateElement("aux")
        .DataType = "bin.base64"
        .Text = sBase64EncodedText
        Base64Decode = BytesToStr(.NodeTypedValue, sTextEncoding)
    End With

End Function

' Returns a binary representation (byte array) of the specified string in
' the specified text encoding, such as "utf-8" or "utf-16le".
' Pass the number of bytes that the encoding's BOM uses as iBomByteCount;
' pass 0 to include the BOM in the output.
function StrToBytes(ByVal sText, ByVal sTextEncoding, ByVal iBomByteCount)

    ' Create a text string with the specified encoding and then
    ' get its binary (byte array) representation.
    With CreateObject("ADODB.Stream")
        ' Create a stream with the specified text encoding...
        .Type = 2  ' adTypeText
        .Charset = sTextEncoding
        .Open
        .WriteText sText
        ' ... and convert it to a binary stream to get a byte-array
        ' representation.
        .Position = 0
        .Type = 1  ' adTypeBinary
        .Position = iBomByteCount ' skip the BOM
        StrToBytes = .Read
        .Close
    End With

end function

' Returns a string that corresponds to the specified byte array, interpreted
' with the specified text encoding, such as "utf-8" or "utf-16le".
function BytesToStr(ByVal byteArray, ByVal sTextEncoding)

    If LCase(sTextEncoding) = "utf-16le" then
        ' UTF-16 LE happens to be VBScript's internal encoding, so we can
        ' take a shortcut and use CStr() to directly convert the byte array
        ' to a string.
        BytesToStr = CStr(byteArray)
    Else ' Convert the specified text encoding to a VBScript string.
        ' Create a binary stream and copy the input byte array to it.
        With CreateObject("ADODB.Stream")
            .Type = 1 ' adTypeBinary
            .Open
            .Write byteArray
            ' Now change the type to text, set the encoding, and output the
            ' result as text.
            .Position = 0
            .Type = 2 ' adTypeText
            .CharSet = sTextEncoding
            BytesToStr = .ReadText
            .Close
        End With
    End If

end function

' === Runner ================================================================

' Run a command, copy its stdout/stderr to ours, and return its exit
' status.
' Modified from https://stackoverflow.com/a/32493083/2877364 by
' https://stackoverflow.com/users/3191599/nate-barbettini .
' See also https://www.vbsedit.com/html/4c5b06ac-dc45-4ec2-aca1-f168bab75483.asp
function RunCommandAndEcho(strCommand)
    Const WshRunning = 0
    Const WshFinished = 1
    Const WshFailed = 2

    Set WshShell = CreateObject("WScript.Shell")
    'WScript.Echo "Running >>" & strCommand & "<<..."
    Set WshShellExec = WshShell.Exec(strCommand)

    Do While WshShellExec.Status = WshRunning
        'WScript.Echo "Waiting..."
        WScript.Sleep 100
    Loop

    if not WshShellExec.StdOut.AtEndOfStream then
        WScript.StdOut.Write(WshShellExec.StdOut.ReadAll())
    end if

    if not WshShellExec.StdErr.AtEndOfStream then
        WScript.StdErr.Write(WshShellExec.StdErr.ReadAll())
    end if

    RunCommandAndEcho = WshShellExec.ExitCode
end function

' === Argument processing ===================================================

function MakeY64Args(args)

    dim b64args(100)    ' 100 = arbitrary max

    ' Make Y64-flavored base64 versions of each arg so we don't have to
    ' worry about quoting issues while executing PowerShell.

    idx=0
    For Each arg In args
        b64args(idx) = Base64Encode(nocrlf(arg), False)
        ' Y64 flavor of Base64
        b64args(idx) = replace( _
        replace( _
            replace(b64args(idx), "+", "."), _
            "/", "_" ), _
        "=", "-")
        'Wscript.Echo cstr(idx) & ": >" & arg & "< = >" & b64args(idx) & "<"
        'Wscript.Echo b64args(idx)
        idx = idx+1
    Next

    MakeY64Args = b64args
end function

Function QuoteForShell(strIn)
    QuoteForShell = """" & _
        replace(strIn, """", """""") & """"
End Function
