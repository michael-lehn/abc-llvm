; @f takes %T by value, modifies a local copy and passes the copy by value to
; itself in tail position.  @f only reads %p, so memory(argmem: read) holds.
; The call cannot modify main's %s (byval): expected exit code 5.
%T = type { i64, i32 }

define internal i64 @f(ptr byval(%T) align 8 %p) memory(argmem: read) {
entry:
  %local = alloca %T, align 8
  %v = load %T, ptr %p, align 8
  store %T %v, ptr %local, align 8
  %n.addr = getelementptr inbounds %T, ptr %local, i32 0, i32 1
  %n = load i32, ptr %n.addr, align 8
  %done = icmp eq i32 %n, 0
  br i1 %done, label %exit, label %recurse

exit:
  ret i64 0

recurse:
  store i32 0, ptr %n.addr, align 8
  %r = call i64 @f(ptr byval(%T) align 8 %local)
  ret i64 %r
}

define i32 @main() {
entry:
  %s = alloca %T, align 8
  store %T { i64 1, i32 5 }, ptr %s, align 8
  %r = call i64 @f(ptr byval(%T) align 8 %s)
  %n.addr = getelementptr inbounds %T, ptr %s, i32 0, i32 1
  %n = load volatile i32, ptr %n.addr, align 8
  ret i32 %n    ; 5 = correct, 0 = main's object was modified
}
