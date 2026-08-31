#!/usr/bin/env python3
import argparse, json, os, re, subprocess, sys
from pathlib import Path

PRIMITIVE = {
    "void": "void",
    "bool": "bool",
    "_Bool": "bool",
    "char": "char",
    "signed char": "char",
    "unsigned char": "u8",
    "short": "short",
    "unsigned short": "ushort",
    "int": "int",
    "unsigned int": "unsigned",
    "long": "long",
    "unsigned long": "unsigned long",
    "long long": "long long",
    "unsigned long long": "unsigned long long",
    "float": "float",
    "double": "double",
}

ABC_KEYWORDS = {
    "type",
    "struct",
    "enum",
    "fn",
    "extern",
    "if",
    "else",
    "while",
    "return",
    "const",
    "readonly",
    "array",
    "of",
}


def abc_identifier(name):
    if name in ABC_KEYWORDS:
        return name + "_"
    return name


class Error(Exception):
    pass


def split_args(s):
    out = []
    cur = []
    depth = 0
    for c in s:
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        if c == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(c)
    if "".join(cur).strip():
        out.append("".join(cur).strip())
    return out


def conv_type(q):
    q = " ".join(q.strip().split())
    q = re.sub(r"\b(struct|enum)\s+", "", q)
    # arrays, possibly multidimensional
    m = re.match(r"^(.*)\[([0-9]+)\]$", q)
    if m:
        return f"array[{m.group(2)}] of {conv_type(m.group(1))}"
    # function pointer type
    m = re.match(r"^(.*?)\s*\(\s*\*\s*\)\s*\((.*)\)$", q)
    if m:
        ret = conv_type(m.group(1))
        args = []
        for i, a in enumerate(split_args(m.group(2))):
            if a == "...":
                args.append("...")
            elif a and a != "void":
                args.append(f"arg{i}: {conv_type(a)}")
        r = f"->fn({', '.join(args)})"
        if ret != "void":
            r += f": {ret}"
        return r
    if q.endswith("*"):
        return "->" + conv_type(q[:-1].strip())
    if q.startswith("const "):
        return "readonly " + conv_type(q[6:])
    if q.startswith("volatile "):
        q = q[9:]
    return PRIMITIVE.get(q, q)


def typedef_enum_decl(src, node):
    text = source_slice(src, node).strip()

    m = re.match(
        r"typedef\s+enum\s*(?:[A-Za-z_]\w*)?\s*\{(.*)\}\s*([A-Za-z_]\w*)\s*;?$",
        text,
        re.S,
    )

    if not m:
        return None

    body = m.group(1)
    name = m.group(2)

    lines = [f"enum {name} {{"]

    # Remove // comments first
    body = re.sub(r"//.*", "", body)

    for item in split_args(body):
        item = item.strip()
        if not item:
            continue

        lines.append(f"    {item},")

    lines.append("};")
    return "\n".join(lines)


def return_type(fn_q):
    # FunctionDecl qualType is "RET (ARGS)" or "RET*(ARGS)"
    i = fn_q.find("(")
    if i < 0:
        raise Error(f"cannot parse function type: {fn_q}")
    return conv_type(fn_q[:i].strip())


def source_slice(src, node):
    r = node.get("range", {})
    b = r.get("begin", {}).get("offset")
    e = r.get("end", {}).get("offset")
    if b is None or e is None:
        return ""
    tok = r.get("end", {}).get("tokLen", 1)
    return src[b : e + tok]


def callback_decl(src, node):
    text = source_slice(src, node).strip()
    m = re.search(
        r"typedef\s+(.+?)\(\s*\*\s*([A-Za-z_]\w*)\s*\)\s*\((.*)\)\s*;?$", text, re.S
    )
    if not m:
        return None
    ret, name, par = m.group(1).strip(), m.group(2), m.group(3).strip()
    args = []
    for i, p in enumerate(split_args(par)):
        if p == "...":
            args.append("...")
            continue
        if p == "void" or not p:
            continue
        # split trailing identifier from type; arrays are not expected in callbacks here
        mm = re.match(r"^(.*?)([A-Za-z_]\w*)$", p.strip())
        if mm:
            ty = mm.group(1).strip()
            nm = mm.group(2)
            # if type ends in struct/typedef identifier with no declarator name, fallback
            if not ty:
                ty = nm
                nm = f"arg{i}"
        else:
            ty = p
            nm = f"arg{i}"
        if ty == "va_list":
            args.append("...")
        else:
            args.append(f"{nm}: {conv_type(ty)}")
    out = f"type {name}: ->fn({', '.join(args)})"
    rt = conv_type(ret)
    if rt != "void":
        out += f": {rt}"
    return out + ";"


def enum_value(n):
    # AST usually has ConstantExpr -> IntegerLiteral with value
    def walk(x):
        if isinstance(x, dict):
            if "value" in x and x.get("kind") in (
                "ConstantExpr",
                "IntegerLiteral",
                "UnaryOperator",
            ):
                return x["value"]
            for c in x.get("inner", []):
                v = walk(c)
                if v is not None:
                    return v
        return None

    return walk(n)


def emit_record(n):
    name = n.get("name")
    if not name or not n.get("completeDefinition"):
        return None
    fields = [x for x in n.get("inner", []) if x.get("kind") == "FieldDecl"]
    lines = [f"struct {name} {{"]
    groups = []
    for f in fields:
        t = conv_type(f["type"]["qualType"])
        nm = abc_identifier(f.get("name", "_"))
        key = f.get("range", {}).get("begin", {}).get("offset")
        if groups and groups[-1][1] == t and groups[-1][2] == key:
            groups[-1][0].append(nm)
        else:
            groups.append(([nm], t, key))
    for names, t, _ in groups:
        lines.append(f"    {', '.join(names)}: {t};")
    lines.append("};")
    return "\n".join(lines)


def emit_enum(n, typedef_name=None):
    name = typedef_name or n.get("name")
    if not name:
        return None
    vals = [x for x in n.get("inner", []) if x.get("kind") == "EnumConstantDecl"]
    lines = [f"enum {name} {{"]
    for x in vals:
        v = enum_value(x)
        # Preserve explicit values only when present in source AST range text is hard; emitting all values is valid and deterministic.
        if v is not None:
            lines.append(f"    {x['name']} = {v},")
        else:
            lines.append(f"    {x['name']},")
    lines.append("};")
    return "\n".join(lines)


def emit_function(n):
    params = []
    for i, p in enumerate(
        x for x in n.get("inner", []) if x.get("kind") == "ParmVarDecl"
    ):
        name = p.get("name") or f"arg{i}"
        name = abc_identifier(name)
        params.append(f"{name}: {conv_type(p['type']['qualType'])}")
    if n.get("variadic"):
        params.append("...")
    out = f"extern fn {n['name']}({', '.join(params)})"
    rt = return_type(n["type"]["qualType"])
    if rt != "void":
        out += f": {rt}"
    return out + ";"


def collect_macros(src):
    result = []
    # join backslash continuations
    logical = re.sub(r"\\\n", "", src)
    for line in logical.splitlines():
        m = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*(?://.*)?$", line)
        if not m:
            continue
        name, val = m.group(1), m.group(2).strip()
        if name in (
            "RAYLIB_H",
            "RAYLIB_VERSION_MAJOR",
            "RAYLIB_VERSION_MINOR",
            "RAYLIB_VERSION_PATCH",
            "RAYLIB_VERSION",
        ) or name.startswith("RL_"):
            continue
        if "(" in name:
            continue
        # raylib useful object-like macros only
        if name in {
            "PI",
            "DEG2RAD",
            "RAD2DEG",
            "MOUSE_LEFT_BUTTON",
            "MOUSE_RIGHT_BUTTON",
            "MOUSE_MIDDLE_BUTTON",
            "MATERIAL_MAP_DIFFUSE",
            "MATERIAL_MAP_SPECULAR",
            "SHADER_LOC_MAP_DIFFUSE",
            "SHADER_LOC_MAP_SPECULAR",
            "GetMouseRay",
        } or re.match(
            r"^(LIGHTGRAY|GRAY|DARKGRAY|YELLOW|GOLD|ORANGE|PINK|RED|MAROON|GREEN|LIME|DARKGREEN|SKYBLUE|BLUE|DARKBLUE|PURPLE|VIOLET|DARKPURPLE|BEIGE|BROWN|DARKBROWN|WHITE|BLACK|BLANK|MAGENTA|RAYWHITE)$",
            name,
        ):
            val = re.sub(r"CLITERAL\s*\(\s*Color\s*\)\s*", "Color", val)
            val = re.sub(r"(?<=\d)f\b", "float", val)
            result.append(f"@define {name} {val}")
    return result


def main():
    ap = argparse.ArgumentParser(
        description="Translate a C header subset to an ABC header using Clang AST JSON."
    )
    ap.add_argument("header")
    ap.add_argument("-o", "--output")
    ap.add_argument("--clang", default="clang")
    ap.add_argument("--no-macros", action="store_true")
    ap.add_argument(
        "clang_args", nargs=argparse.REMAINDER, help="extra clang args after --"
    )
    a = ap.parse_args()
    path = Path(a.header).resolve()
    src = path.read_text()
    extra = a.clang_args
    if extra and extra[0] == "--":
        extra = extra[1:]
    cmd = [
        a.clang,
        "-x",
        "c",
        "-std=c11",
        "-fsyntax-only",
        "-Xclang",
        "-ast-dump=json",
        str(path),
        *extra,
    ]
    p = subprocess.run(cmd, text=True, capture_output=True)
    if p.returncode:
        sys.stderr.write(p.stderr)
        return p.returncode
    ast = json.loads(p.stdout)
    top = ast.get("inner", [])
    # First declaration known to originate in main file; subsequent nodes with offsets in main source are raylib nodes.
    start = next(
        (i for i, n in enumerate(top) if n.get("loc", {}).get("file") == str(path)),
        len(top),
    )
    top = top[start:]
    by_id = {n.get("id"): n for n in top}
    emitted_record_ids = set()
    emitted_enum_ids = set()
    chunks = []
    if not a.no_macros:
        ms = collect_macros(src)
        if ms:
            chunks.append("\n".join(ms))
    for n in top:
        k = n.get("kind")
        if k == "RecordDecl" and n.get("completeDefinition") and n.get("name"):
            # typedef struct Foo {...} Foo; is emitted here; skip duplicate typedef later
            s = emit_record(n)
            if s:
                chunks.append(s)
                emitted_record_ids.add(n.get("id"))
        elif k == "EnumDecl":
            # anonymous typedef enums are emitted at TypedefDecl so they get the typedef name
            if n.get("name"):
                s = emit_enum(n)
                if s:
                    chunks.append(s)
                    emitted_enum_ids.add(n.get("id"))

        elif k == "TypedefDecl":
            name = n.get("name")
            q = n.get("type", {}).get("qualType", "")

            if name in ("va_list",):
                continue

            enum_decl = typedef_enum_decl(src, n)
            if enum_decl:
                chunks.append(enum_decl)
                continue

            # Struct typedefs
            m = re.fullmatch(r"struct\s+([A-Za-z_]\w*)", q)
            if m:
                struct_name = m.group(1)

                if name == struct_name:
                    complete = any(
                        x.get("kind") == "RecordDecl"
                        and x.get("name") == struct_name
                        and x.get("completeDefinition")
                        for x in top
                    )

                    if complete:
                        continue

                    chunks.append(f"struct {struct_name};")
                    continue

                chunks.append(f"type {name}: {struct_name};")
                continue

            owned = None
            for x in n.get("inner", []):
                owned = x.get("ownedTagDecl") if isinstance(x, dict) else None
                if owned:
                    break

            if owned and owned.get("kind") == "EnumDecl":
                en = by_id.get(owned.get("id"))
                if en:
                    s = emit_enum(en, name)
                    if s:
                        chunks.append(s)
                        emitted_enum_ids.add(en.get("id"))
                    continue

            cb = callback_decl(src, n)
            if cb:
                chunks.append(cb)
                continue

            if q.startswith("enum "):
                continue

            chunks.append(f"type {name}: {conv_type(q)};")

        elif k == "FunctionDecl" and n.get("name"):
            chunks.append(emit_function(n))
    out = "\n\n".join(chunks) + "\n"
    if a.output:
        Path(a.output).write_text(out)
    else:
        sys.stdout.write(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
