/* C part of c_abi_x86_64.abc */

#include <stdlib.h>
#include <stdio.h>
typedef struct { float x; } F1;
typedef struct { float x, y; } F2;
typedef struct { float x, y, z; } F3;
typedef struct { float x, y, z, w; } F4;
typedef struct { double x; } D1;
typedef struct { double x, y; } D2;
typedef struct { float x; double y; } FD;
typedef struct { double x; float y; } DF;
typedef struct { int x; } I1;
typedef struct { int x, y; } I2;
typedef struct { int x, y, z; } I3;
typedef struct { int x, y, z, w; } I4;
typedef struct { long long x; } L1;
typedef struct { long long x, y; } L2;
typedef struct { long long x, y, z; } L3;
typedef struct { int x; float y; } IF;
typedef struct { float x; int y; } FI;
typedef struct { long long x; double y; } LD;
typedef struct { double x; long long y; } DL;
typedef struct { int x; float y; double z; } IFD;
typedef struct { unsigned char x, y, z; } C3;
typedef struct { unsigned char x, y, z, w; } C4;
typedef struct { unsigned char a, b, c, d, e; } C5;
typedef struct { short x; unsigned char y; float z; } S1;
typedef struct { int *p; int x; } P1;
typedef struct { F2 a, b; } NF;
typedef struct { IF a; D1 b; } NM;
F1 c_ret_F1(void) { F1 r;
  r.x = 1.5;
  return r; }
int c_arg_F1(F1 s) { return s.x == 1.5; }
int c_arg5_F1(int a, int b, int c, int d, int e, F1 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5; }
int c_arg6_F1(int a, int b, int c, int d, int e, int f, F1 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5; }
int c_argd_F1(double a, double b, double c, double d, double e, double f, double g, F1 s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5; }
F1 c_big_F1(L3 big, int a, int b, int c, int d, int e, F1 s, int t) { F1 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_F1(int a, int b, int c, int d, int e, F1 s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5)) abort(); return r; }
F1 abc_ret_F1(void); int abc_arg_F1(F1); int abc_arg6_F1(int,int,int,int,int,int,F1,int); F1 abc_echo_F1(F1);
int cc_F1(void) { F1 s;
  s.x = 1.5;
  F1 r = abc_ret_F1(); if (!(r.x == 1.5)) { printf("F1: C reading return from ABC\n"); return 1; }
  if (!abc_arg_F1(s)) { printf("F1: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_F1(1,2,3,4,5,6,s,7)) { printf("F1: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_F1(s); if (!(r.x == 1.5)) { printf("F1: C echo through ABC\n"); return 1; }
  return 0; }
F2 c_ret_F2(void) { F2 r;
  r.x = 1.5;
  r.y = 2.5;
  return r; }
int c_arg_F2(F2 s) { return s.x == 1.5 && s.y == 2.5; }
int c_arg5_F2(int a, int b, int c, int d, int e, F2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5 && s.y == 2.5; }
int c_arg6_F2(int a, int b, int c, int d, int e, int f, F2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5 && s.y == 2.5; }
int c_argd_F2(double a, double b, double c, double d, double e, double f, double g, F2 s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5 && s.y == 2.5; }
F2 c_big_F2(L3 big, int a, int b, int c, int d, int e, F2 s, int t) { F2 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_F2(int a, int b, int c, int d, int e, F2 s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5 && s.y == 2.5)) abort(); return r; }
F2 abc_ret_F2(void); int abc_arg_F2(F2); int abc_arg6_F2(int,int,int,int,int,int,F2,int); F2 abc_echo_F2(F2);
int cc_F2(void) { F2 s;
  s.x = 1.5;
  s.y = 2.5;
  F2 r = abc_ret_F2(); if (!(r.x == 1.5 && r.y == 2.5)) { printf("F2: C reading return from ABC\n"); return 1; }
  if (!abc_arg_F2(s)) { printf("F2: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_F2(1,2,3,4,5,6,s,7)) { printf("F2: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_F2(s); if (!(r.x == 1.5 && r.y == 2.5)) { printf("F2: C echo through ABC\n"); return 1; }
  return 0; }
F3 c_ret_F3(void) { F3 r;
  r.x = 1.5;
  r.y = 2.5;
  r.z = 3.5;
  return r; }
int c_arg_F3(F3 s) { return s.x == 1.5 && s.y == 2.5 && s.z == 3.5; }
int c_arg5_F3(int a, int b, int c, int d, int e, F3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5; }
int c_arg6_F3(int a, int b, int c, int d, int e, int f, F3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5; }
int c_argd_F3(double a, double b, double c, double d, double e, double f, double g, F3 s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5; }
F3 c_big_F3(L3 big, int a, int b, int c, int d, int e, F3 s, int t) { F3 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_F3(int a, int b, int c, int d, int e, F3 s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5 && s.y == 2.5 && s.z == 3.5)) abort(); return r; }
F3 abc_ret_F3(void); int abc_arg_F3(F3); int abc_arg6_F3(int,int,int,int,int,int,F3,int); F3 abc_echo_F3(F3);
int cc_F3(void) { F3 s;
  s.x = 1.5;
  s.y = 2.5;
  s.z = 3.5;
  F3 r = abc_ret_F3(); if (!(r.x == 1.5 && r.y == 2.5 && r.z == 3.5)) { printf("F3: C reading return from ABC\n"); return 1; }
  if (!abc_arg_F3(s)) { printf("F3: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_F3(1,2,3,4,5,6,s,7)) { printf("F3: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_F3(s); if (!(r.x == 1.5 && r.y == 2.5 && r.z == 3.5)) { printf("F3: C echo through ABC\n"); return 1; }
  return 0; }
F4 c_ret_F4(void) { F4 r;
  r.x = 1.5;
  r.y = 2.5;
  r.z = 3.5;
  r.w = 4.5;
  return r; }
int c_arg_F4(F4 s) { return s.x == 1.5 && s.y == 2.5 && s.z == 3.5 && s.w == 4.5; }
int c_arg5_F4(int a, int b, int c, int d, int e, F4 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5 && s.w == 4.5; }
int c_arg6_F4(int a, int b, int c, int d, int e, int f, F4 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5 && s.w == 4.5; }
int c_argd_F4(double a, double b, double c, double d, double e, double f, double g, F4 s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5 && s.y == 2.5 && s.z == 3.5 && s.w == 4.5; }
F4 c_big_F4(L3 big, int a, int b, int c, int d, int e, F4 s, int t) { F4 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_F4(int a, int b, int c, int d, int e, F4 s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5 && s.y == 2.5 && s.z == 3.5 && s.w == 4.5)) abort(); return r; }
F4 abc_ret_F4(void); int abc_arg_F4(F4); int abc_arg6_F4(int,int,int,int,int,int,F4,int); F4 abc_echo_F4(F4);
int cc_F4(void) { F4 s;
  s.x = 1.5;
  s.y = 2.5;
  s.z = 3.5;
  s.w = 4.5;
  F4 r = abc_ret_F4(); if (!(r.x == 1.5 && r.y == 2.5 && r.z == 3.5 && r.w == 4.5)) { printf("F4: C reading return from ABC\n"); return 1; }
  if (!abc_arg_F4(s)) { printf("F4: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_F4(1,2,3,4,5,6,s,7)) { printf("F4: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_F4(s); if (!(r.x == 1.5 && r.y == 2.5 && r.z == 3.5 && r.w == 4.5)) { printf("F4: C echo through ABC\n"); return 1; }
  return 0; }
D2 c_ret_D2(void) { D2 r;
  r.x = 1.5;
  r.y = 2.5;
  return r; }
int c_arg_D2(D2 s) { return s.x == 1.5 && s.y == 2.5; }
int c_arg5_D2(int a, int b, int c, int d, int e, D2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5 && s.y == 2.5; }
int c_arg6_D2(int a, int b, int c, int d, int e, int f, D2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5 && s.y == 2.5; }
int c_argd_D2(double a, double b, double c, double d, double e, double f, double g, D2 s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5 && s.y == 2.5; }
D2 c_big_D2(L3 big, int a, int b, int c, int d, int e, D2 s, int t) { D2 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_D2(int a, int b, int c, int d, int e, D2 s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5 && s.y == 2.5)) abort(); return r; }
D2 abc_ret_D2(void); int abc_arg_D2(D2); int abc_arg6_D2(int,int,int,int,int,int,D2,int); D2 abc_echo_D2(D2);
int cc_D2(void) { D2 s;
  s.x = 1.5;
  s.y = 2.5;
  D2 r = abc_ret_D2(); if (!(r.x == 1.5 && r.y == 2.5)) { printf("D2: C reading return from ABC\n"); return 1; }
  if (!abc_arg_D2(s)) { printf("D2: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_D2(1,2,3,4,5,6,s,7)) { printf("D2: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_D2(s); if (!(r.x == 1.5 && r.y == 2.5)) { printf("D2: C echo through ABC\n"); return 1; }
  return 0; }
FD c_ret_FD(void) { FD r;
  r.x = 1.5;
  r.y = 2.5;
  return r; }
int c_arg_FD(FD s) { return s.x == 1.5 && s.y == 2.5; }
int c_arg5_FD(int a, int b, int c, int d, int e, FD s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 1.5 && s.y == 2.5; }
int c_arg6_FD(int a, int b, int c, int d, int e, int f, FD s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 1.5 && s.y == 2.5; }
int c_argd_FD(double a, double b, double c, double d, double e, double f, double g, FD s, double t) { return a==1&&g==7&&t==8&&s.x == 1.5 && s.y == 2.5; }
FD c_big_FD(L3 big, int a, int b, int c, int d, int e, FD s, int t) { FD r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_FD(int a, int b, int c, int d, int e, FD s, int t) { L3 r = {a,e,t}; if (!(s.x == 1.5 && s.y == 2.5)) abort(); return r; }
FD abc_ret_FD(void); int abc_arg_FD(FD); int abc_arg6_FD(int,int,int,int,int,int,FD,int); FD abc_echo_FD(FD);
int cc_FD(void) { FD s;
  s.x = 1.5;
  s.y = 2.5;
  FD r = abc_ret_FD(); if (!(r.x == 1.5 && r.y == 2.5)) { printf("FD: C reading return from ABC\n"); return 1; }
  if (!abc_arg_FD(s)) { printf("FD: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_FD(1,2,3,4,5,6,s,7)) { printf("FD: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_FD(s); if (!(r.x == 1.5 && r.y == 2.5)) { printf("FD: C echo through ABC\n"); return 1; }
  return 0; }
I3 c_ret_I3(void) { I3 r;
  r.x = 7;
  r.y = 8;
  r.z = 9;
  return r; }
int c_arg_I3(I3 s) { return s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg5_I3(int a, int b, int c, int d, int e, I3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg6_I3(int a, int b, int c, int d, int e, int f, I3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_argd_I3(double a, double b, double c, double d, double e, double f, double g, I3 s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 8 && s.z == 9; }
I3 c_big_I3(L3 big, int a, int b, int c, int d, int e, I3 s, int t) { I3 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_I3(int a, int b, int c, int d, int e, I3 s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 8 && s.z == 9)) abort(); return r; }
I3 abc_ret_I3(void); int abc_arg_I3(I3); int abc_arg6_I3(int,int,int,int,int,int,I3,int); I3 abc_echo_I3(I3);
int cc_I3(void) { I3 s;
  s.x = 7;
  s.y = 8;
  s.z = 9;
  I3 r = abc_ret_I3(); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("I3: C reading return from ABC\n"); return 1; }
  if (!abc_arg_I3(s)) { printf("I3: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_I3(1,2,3,4,5,6,s,7)) { printf("I3: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_I3(s); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("I3: C echo through ABC\n"); return 1; }
  return 0; }
L2 c_ret_L2(void) { L2 r;
  r.x = 7;
  r.y = 8;
  return r; }
int c_arg_L2(L2 s) { return s.x == 7 && s.y == 8; }
int c_arg5_L2(int a, int b, int c, int d, int e, L2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 8; }
int c_arg6_L2(int a, int b, int c, int d, int e, int f, L2 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 8; }
int c_argd_L2(double a, double b, double c, double d, double e, double f, double g, L2 s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 8; }
L2 c_big_L2(L3 big, int a, int b, int c, int d, int e, L2 s, int t) { L2 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_L2(int a, int b, int c, int d, int e, L2 s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 8)) abort(); return r; }
L2 abc_ret_L2(void); int abc_arg_L2(L2); int abc_arg6_L2(int,int,int,int,int,int,L2,int); L2 abc_echo_L2(L2);
int cc_L2(void) { L2 s;
  s.x = 7;
  s.y = 8;
  L2 r = abc_ret_L2(); if (!(r.x == 7 && r.y == 8)) { printf("L2: C reading return from ABC\n"); return 1; }
  if (!abc_arg_L2(s)) { printf("L2: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_L2(1,2,3,4,5,6,s,7)) { printf("L2: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_L2(s); if (!(r.x == 7 && r.y == 8)) { printf("L2: C echo through ABC\n"); return 1; }
  return 0; }
L3 c_ret_L3(void) { L3 r;
  r.x = 7;
  r.y = 8;
  r.z = 9;
  return r; }
int c_arg_L3(L3 s) { return s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg5_L3(int a, int b, int c, int d, int e, L3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg6_L3(int a, int b, int c, int d, int e, int f, L3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_argd_L3(double a, double b, double c, double d, double e, double f, double g, L3 s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 8 && s.z == 9; }
L3 c_big_L3(L3 big, int a, int b, int c, int d, int e, L3 s, int t) { L3 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_L3(int a, int b, int c, int d, int e, L3 s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 8 && s.z == 9)) abort(); return r; }
L3 abc_ret_L3(void); int abc_arg_L3(L3); int abc_arg6_L3(int,int,int,int,int,int,L3,int); L3 abc_echo_L3(L3);
int cc_L3(void) { L3 s;
  s.x = 7;
  s.y = 8;
  s.z = 9;
  L3 r = abc_ret_L3(); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("L3: C reading return from ABC\n"); return 1; }
  if (!abc_arg_L3(s)) { printf("L3: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_L3(1,2,3,4,5,6,s,7)) { printf("L3: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_L3(s); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("L3: C echo through ABC\n"); return 1; }
  return 0; }
IF c_ret_IF(void) { IF r;
  r.x = 7;
  r.y = 2.5;
  return r; }
int c_arg_IF(IF s) { return s.x == 7 && s.y == 2.5; }
int c_arg5_IF(int a, int b, int c, int d, int e, IF s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 2.5; }
int c_arg6_IF(int a, int b, int c, int d, int e, int f, IF s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 2.5; }
int c_argd_IF(double a, double b, double c, double d, double e, double f, double g, IF s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 2.5; }
IF c_big_IF(L3 big, int a, int b, int c, int d, int e, IF s, int t) { IF r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_IF(int a, int b, int c, int d, int e, IF s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 2.5)) abort(); return r; }
IF abc_ret_IF(void); int abc_arg_IF(IF); int abc_arg6_IF(int,int,int,int,int,int,IF,int); IF abc_echo_IF(IF);
int cc_IF(void) { IF s;
  s.x = 7;
  s.y = 2.5;
  IF r = abc_ret_IF(); if (!(r.x == 7 && r.y == 2.5)) { printf("IF: C reading return from ABC\n"); return 1; }
  if (!abc_arg_IF(s)) { printf("IF: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_IF(1,2,3,4,5,6,s,7)) { printf("IF: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_IF(s); if (!(r.x == 7 && r.y == 2.5)) { printf("IF: C echo through ABC\n"); return 1; }
  return 0; }
LD c_ret_LD(void) { LD r;
  r.x = 7;
  r.y = 2.5;
  return r; }
int c_arg_LD(LD s) { return s.x == 7 && s.y == 2.5; }
int c_arg5_LD(int a, int b, int c, int d, int e, LD s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 2.5; }
int c_arg6_LD(int a, int b, int c, int d, int e, int f, LD s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 2.5; }
int c_argd_LD(double a, double b, double c, double d, double e, double f, double g, LD s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 2.5; }
LD c_big_LD(L3 big, int a, int b, int c, int d, int e, LD s, int t) { LD r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_LD(int a, int b, int c, int d, int e, LD s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 2.5)) abort(); return r; }
LD abc_ret_LD(void); int abc_arg_LD(LD); int abc_arg6_LD(int,int,int,int,int,int,LD,int); LD abc_echo_LD(LD);
int cc_LD(void) { LD s;
  s.x = 7;
  s.y = 2.5;
  LD r = abc_ret_LD(); if (!(r.x == 7 && r.y == 2.5)) { printf("LD: C reading return from ABC\n"); return 1; }
  if (!abc_arg_LD(s)) { printf("LD: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_LD(1,2,3,4,5,6,s,7)) { printf("LD: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_LD(s); if (!(r.x == 7 && r.y == 2.5)) { printf("LD: C echo through ABC\n"); return 1; }
  return 0; }
C3 c_ret_C3(void) { C3 r;
  r.x = 7;
  r.y = 8;
  r.z = 9;
  return r; }
int c_arg_C3(C3 s) { return s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg5_C3(int a, int b, int c, int d, int e, C3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_arg6_C3(int a, int b, int c, int d, int e, int f, C3 s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.x == 7 && s.y == 8 && s.z == 9; }
int c_argd_C3(double a, double b, double c, double d, double e, double f, double g, C3 s, double t) { return a==1&&g==7&&t==8&&s.x == 7 && s.y == 8 && s.z == 9; }
C3 c_big_C3(L3 big, int a, int b, int c, int d, int e, C3 s, int t) { C3 r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_C3(int a, int b, int c, int d, int e, C3 s, int t) { L3 r = {a,e,t}; if (!(s.x == 7 && s.y == 8 && s.z == 9)) abort(); return r; }
C3 abc_ret_C3(void); int abc_arg_C3(C3); int abc_arg6_C3(int,int,int,int,int,int,C3,int); C3 abc_echo_C3(C3);
int cc_C3(void) { C3 s;
  s.x = 7;
  s.y = 8;
  s.z = 9;
  C3 r = abc_ret_C3(); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("C3: C reading return from ABC\n"); return 1; }
  if (!abc_arg_C3(s)) { printf("C3: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_C3(1,2,3,4,5,6,s,7)) { printf("C3: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_C3(s); if (!(r.x == 7 && r.y == 8 && r.z == 9)) { printf("C3: C echo through ABC\n"); return 1; }
  return 0; }
NM c_ret_NM(void) { NM r;
  r.a.x = 7;
  r.a.y = 2.5;
  r.b.x = 1.5;
  return r; }
int c_arg_NM(NM s) { return s.a.x == 7 && s.a.y == 2.5 && s.b.x == 1.5; }
int c_arg5_NM(int a, int b, int c, int d, int e, NM s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&t==6&&s.a.x == 7 && s.a.y == 2.5 && s.b.x == 1.5; }
int c_arg6_NM(int a, int b, int c, int d, int e, int f, NM s, int t) { return a==1&&b==2&&c==3&&d==4&&e==5&&f==6&&t==7&&s.a.x == 7 && s.a.y == 2.5 && s.b.x == 1.5; }
int c_argd_NM(double a, double b, double c, double d, double e, double f, double g, NM s, double t) { return a==1&&g==7&&t==8&&s.a.x == 7 && s.a.y == 2.5 && s.b.x == 1.5; }
NM c_big_NM(L3 big, int a, int b, int c, int d, int e, NM s, int t) { NM r = s; if (!(big.z==9&&a==1&&e==5&&t==6)) abort(); return r; }
L3 c_sret_NM(int a, int b, int c, int d, int e, NM s, int t) { L3 r = {a,e,t}; if (!(s.a.x == 7 && s.a.y == 2.5 && s.b.x == 1.5)) abort(); return r; }
NM abc_ret_NM(void); int abc_arg_NM(NM); int abc_arg6_NM(int,int,int,int,int,int,NM,int); NM abc_echo_NM(NM);
int cc_NM(void) { NM s;
  s.a.x = 7;
  s.a.y = 2.5;
  s.b.x = 1.5;
  NM r = abc_ret_NM(); if (!(r.a.x == 7 && r.a.y == 2.5 && r.b.x == 1.5)) { printf("NM: C reading return from ABC\n"); return 1; }
  if (!abc_arg_NM(s)) { printf("NM: ABC reading arg from C\n"); return 1; }
  if (!abc_arg6_NM(1,2,3,4,5,6,s,7)) { printf("NM: C calls ABC, 6 ints then struct\n"); return 1; }
  r = abc_echo_NM(s); if (!(r.a.x == 7 && r.a.y == 2.5 && r.b.x == 1.5)) { printf("NM: C echo through ABC\n"); return 1; }
  return 0; }
int cc_all(void) { int bad = 0; bad += cc_F1(); bad += cc_F2(); bad += cc_F3(); bad += cc_F4(); bad += cc_D2(); bad += cc_FD(); bad += cc_I3(); bad += cc_L2(); bad += cc_L3(); bad += cc_IF(); bad += cc_LD(); bad += cc_C3(); bad += cc_NM(); return bad; }
