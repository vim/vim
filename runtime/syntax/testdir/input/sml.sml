(* Integer constants *)

val _ = 0;
val _ = 42;
val _ = 0x2A;
val _ = ~42;         (* single token *)
val _ = ~0x2A;       (* single token *)


(* Word constants *)

val _ = 0w0;
val _ = 0w42;
val _ = 0wx2A;
val _ = ~0w42;       (* nonfix ~ *)
val _ = ~0wx2A;      (* nonfix ~ *)


(* Real constants *)

val _ = 0.0;
val _ = 42.42;
val _ = 42E42;
val _ = 42E~42;
val _ = 42.42E42;
val _ = 42.42E~42;
val _ = ~42.42;      (* single token *)
val _ = ~42.42;      (* single token *)
val _ = ~42E42;      (* single token *)
val _ = ~42E~42;     (* single token *)
val _ = ~42.42E42;   (* single token *)
val _ = ~42.42E~42;  (* single token *)


(* Character constants *)

val _ = #"a";
val _ = #"\a";
val _ = #"\b";
val _ = #"\t";
val _ = #"\n";
val _ = #"\v";
val _ = #"\f";
val _ = #"\r";
val _ = #"\"";
val _ = #"\\";
val _ = #"\^@";
val _ = #"\^A";
val _ = #"\^K";
val _ = #"\^Z";
val _ = #"\^[";
val _ = #"\^\";
val _ = #"\^]";
val _ = #"\^^";
val _ = #"\^_";
val _ = #"\000";
val _ = #"\999";
val _ = #"\u0000";
val _ = #"\uFFFF";


(* String constants *)

val _ = "";
val _ = "abc";
val _ = "...\a...\b...\t...\n...\v...\f...\r...\"...\\...";
val _ = "...\^@...\^A...\^K...\^Z...\^[...\^\...\^]...\^^...\^_...";
val _ = "\000...\999...\u0000...\uFFFF...";
val _ = "...\
            \...";

