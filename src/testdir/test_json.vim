" Test for JSON functions.
scriptencoding utf-8

let s:json1 = '"str\"in\\g"'
let s:var1 = "str\"in\\g"
let s:json2 = '"\u0001\u0002\u0003\u0004\u0005\u0006\u0007"'
let s:var2 = "\x01\x02\x03\x04\x05\x06\x07"
let s:json3 = '"\b\t\n\u000b\f\r\u000e\u000f"'
let s:var3 = "\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
let s:json4 = '"\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017"'
let s:var4 = "\x10\x11\x12\x13\x14\x15\x16\x17"
let s:json5 = '"\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f"'
let s:var5 = "\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"

let s:jsonmb = '"s¢cĴgё"'
let s:varmb = "s¢cĴgё"
let s:jsonnr = '1234'
let s:varnr = 1234
let s:jsonfl = '12.34'
let s:varfl = 12.34

let s:jsonl1 = '[1,"a",3]'
let s:varl1 = [1, "a", 3]
let s:jsonl2 = '[1,["a",[],"c"],3]'
let s:jsonl2s = "  [\r1  ,  [  \"a\"  ,  [  ]  ,  \"c\"  ]  ,  3\<Tab>]\r\n"
let s:varl2 = [1, 2, 3]
let l2 = ['a', s:varl2, 'c']
let s:varl2[1] = l2
let s:varl2x = [1, ["a", [], "c"], 3]
let s:jsonl3 = '[[1,2],[1,2]]'
let l3 = [1, 2]
let s:varl3 = [l3, l3]

let s:jsond1 = '{"a":1,"b":"bee","c":[1,2]}'
let s:vard1 = {"a": 1, "b": "bee","c": [1,2]}
let s:jsond2 = '{"1":1,"2":{"a":"aa","b":{},"c":"cc"},"3":3}'
let s:jsond2s = "  { \"1\" : 1 , \"2\" :\n{ \"a\"\r: \"aa\" , \"b\" : {\<Tab>} , \"c\" : \"cc\" } , \"3\" : 3 }\r\n"
let s:vard2 = {"1": 1, "2": 2, "3": 3}
let d2 = {"a": "aa", "b": s:vard2, "c": "cc"}
let s:vard2["2"] = d2
let s:vard2x = {"1": 1, "2": {"a": "aa", "b": {}, "c": "cc"}, "3": 3}
let d3 = {"a": 1, "b": 2}
let s:vard3 = {"x": d3, "y": d3}
let s:jsond3 = '{"x":{"a":1,"b":2},"y":{"a":1,"b":2}}'

let s:jsonvals = '[true,false,,null]'
let s:varvals = [v:true, v:false, v:none, v:null]

func Test_encode()
  call assert_equal(s:json1, jsonencode(s:var1))
  call assert_equal(s:json2, jsonencode(s:var2))
  call assert_equal(s:json3, jsonencode(s:var3))
  call assert_equal(s:json4, jsonencode(s:var4))
  call assert_equal(s:json5, jsonencode(s:var5))

  if has('multi_byte')
    call assert_equal(s:jsonmb, jsonencode(s:varmb))
  endif

  call assert_equal(s:jsonnr, jsonencode(s:varnr))
  if has('float')
    call assert_equal(s:jsonfl, jsonencode(s:varfl))
  endif

  call assert_equal(s:jsonl1, jsonencode(s:varl1))
  call assert_equal(s:jsonl2, jsonencode(s:varl2))
  call assert_equal(s:jsonl3, jsonencode(s:varl3))

  call assert_equal(s:jsond1, jsonencode(s:vard1))
  call assert_equal(s:jsond2, jsonencode(s:vard2))
  call assert_equal(s:jsond3, jsonencode(s:vard3))

  call assert_equal(s:jsonvals, jsonencode(s:varvals))

  call assert_fails('echo jsonencode(function("tr"))', 'E474:')
  call assert_fails('echo jsonencode([function("tr")])', 'E474:')
endfunc

func Test_decode()
  call assert_equal(s:var1, jsondecode(s:json1))
  call assert_equal(s:var2, jsondecode(s:json2))
  call assert_equal(s:var3, jsondecode(s:json3))
  call assert_equal(s:var4, jsondecode(s:json4))
  call assert_equal(s:var5, jsondecode(s:json5))

  if has('multi_byte')
    call assert_equal(s:varmb, jsondecode(s:jsonmb))
  endif

  call assert_equal(s:varnr, jsondecode(s:jsonnr))
  if has('float')
    call assert_equal(s:varfl, jsondecode(s:jsonfl))
  endif

  call assert_equal(s:varl1, jsondecode(s:jsonl1))
  call assert_equal(s:varl2x, jsondecode(s:jsonl2))
  call assert_equal(s:varl2x, jsondecode(s:jsonl2s))
  call assert_equal(s:varl3, jsondecode(s:jsonl3))

  call assert_equal(s:vard1, jsondecode(s:jsond1))
  call assert_equal(s:vard2x, jsondecode(s:jsond2))
  call assert_equal(s:vard3, jsondecode(s:jsond3))

  call assert_equal(s:varvals, jsondecode(s:jsonvals))

  call assert_equal(v:true, jsondecode('true'))
  call assert_equal(type(v:true), type(jsondecode('true')))
  call assert_equal(v:none, jsondecode(''))
  call assert_equal(type(v:none), type(jsondecode('')))
  call assert_equal("", jsondecode('""'))

  call assert_equal({'n': 1}, jsondecode('{"n":1,}'))

  call assert_fails('call jsondecode("\"")', "E474:")
  call assert_fails('call jsondecode("blah")', "E474:")
  call assert_fails('call jsondecode("true blah")', "E474:")
  call assert_fails('call jsondecode("<foobar>")', "E474:")

  call assert_fails('call jsondecode("{")', "E474:")
  call assert_fails('call jsondecode("{foobar}")', "E474:")
  call assert_fails('call jsondecode("{\"n\",")', "E474:")
  call assert_fails('call jsondecode("{\"n\":")', "E474:")
  call assert_fails('call jsondecode("{\"n\":1")', "E474:")
  call assert_fails('call jsondecode("{\"n\":1,")', "E474:")
  call assert_fails('call jsondecode("{\"n\",1}")', "E474:")
  call assert_fails('call jsondecode("{-}")', "E474:")

  call assert_fails('call jsondecode("[foobar]")', "E474:")
  call assert_fails('call jsondecode("[")', "E474:")
  call assert_fails('call jsondecode("[1")', "E474:")
  call assert_fails('call jsondecode("[1,")', "E474:")
  call assert_fails('call jsondecode("[1 2]")', "E474:")
endfunc
