#!/usr/bin/env python3
"""
CppAdmin convention checker — CI gate.
Run: python3 tools/check_conventions.py <project_root>
Exit 0 = all OK. Exit 1 = violations found.

Rules enforced:
  1. std::getenv() in services/, controllers/, filters/ (except AppConfig.h) → FAIL
  2. Drogon ORM/DbClient used directly in controllers/ → FAIL
  3. Service .cc file without corresponding I*Service.h → FAIL
  4. Module .cc with route registration without ROUTE_REG → FAIL (warning)
  5. CSP template outputting variable without h() wrapper → WARN
  6. tableName not explicitly set in model .h files → WARN
"""

import sys
import os
import re
from pathlib import Path


VIOLATIONS: list[tuple[str, str, int]] = []  # (file, message, severity) severity 0=warn,1=fail
WARNINGS: list[tuple[str, str]] = []


def fail(filepath: str, msg: str) -> None:
    VIOLATIONS.append((filepath, msg, 1))


def warn(filepath: str, msg: str) -> None:
    VIOLATIONS.append((filepath, msg, 0))


def check_getenv_in_modules(root: Path) -> None:
    """Rule 1: std::getenv() forbidden in services/, controllers/, filters/ (not AppConfig.h)."""
    dirs = [root / "services", root / "controllers", root / "filters"]
    for d in dirs:
        if not d.exists():
            continue
        for f in d.rglob("*.cc"):
            text = f.read_text(errors="replace")
            if "std::getenv" in text or re.search(r'\bgetenv\s*\(', text):
                fail(str(f.relative_to(root)),
                     "std::getenv() in module — use AppConfig::instance() instead")
        for f in d.rglob("*.h"):
            text = f.read_text(errors="replace")
            if "std::getenv" in text or re.search(r'\bgetenv\s*\(', text):
                fail(str(f.relative_to(root)),
                     "std::getenv() in module header — use AppConfig::instance() instead")


def check_no_db_in_controllers(root: Path) -> None:
    """Rule 2: Controllers must not access DB directly (no Mapper/DbClient include in .cc)."""
    cdir = root / "controllers"
    if not cdir.exists():
        return
    db_patterns = [
        re.compile(r'drogon::orm::Mapper'),
        re.compile(r'app\(\)\.getDbClient'),
        re.compile(r'execSqlAsync'),
        re.compile(r'#include.*orm/Mapper'),
    ]
    for f in cdir.rglob("*.cc"):
        text = f.read_text(errors="replace")
        for pat in db_patterns:
            if pat.search(text):
                fail(str(f.relative_to(root)),
                     f"Controller accesses DB directly ({pat.pattern}) — delegate to service")
                break


def check_service_has_interface(root: Path) -> None:
    """Rule 3: Every *Service.cc must have a corresponding I*Service.h."""
    sdir = root / "services"
    if not sdir.exists():
        return
    for f in sdir.rglob("*Service.cc"):
        # e.g. UserService.cc → IUserService.h
        name = f.stem  # "UserService"
        interface_name = "I" + name + ".h"
        # Search for interface anywhere under services/
        matches = list(sdir.rglob(interface_name))
        if not matches:
            fail(str(f.relative_to(root)),
                 f"Service {name}.cc has no interface {interface_name} — add I{name}.h")


def check_route_reg(root: Path) -> None:
    """Rule 4: Route registration functions should call ROUTE_REG()."""
    # Check *Routes.cc files
    cdir = root / "controllers"
    if not cdir.exists():
        return
    for f in cdir.rglob("*Routes.cc"):
        text = f.read_text(errors="replace")
        # Skip stub file
        if "RouteStubs.cc" in f.name:
            continue
        if "ROUTE_REG" not in text and "RouteRegistry" not in text:
            warn(str(f.relative_to(root)),
                 "Route registration file does not call ROUTE_REG() — routes won't appear in RBAC")


def check_csp_escape(root: Path) -> None:
    """Rule 5 (warning): CSP files outputting variables should use h() wrapper."""
    vdir = root / "views"
    if not vdir.exists():
        return
    # Pattern: output->write(...) without h() on a non-trusted value
    # Simple heuristic: output->write( followed by getValueOf or a variable that isn't h(
    raw_output = re.compile(r'output->write\((?!h\(|"|\d|\w+\s*\?)[^)]*getValueOf')
    for f in vdir.rglob("*.csp"):
        text = f.read_text(errors="replace")
        for m in raw_output.finditer(text):
            line_no = text[:m.start()].count('\n') + 1
            warn(f"{f.relative_to(root)}:{line_no}",
                 "CSP output without h() wrapper — potential XSS. Use output->write(h(val))")


def check_model_table_name(root: Path) -> None:
    """Rule 6 (warning): Model .h files should have explicit tableName."""
    mdir = root / "models"
    if not mdir.exists():
        return
    for f in mdir.rglob("*.h"):
        text = f.read_text(errors="replace")
        # Skip model.json or non-model headers
        if "tableName" not in text and "class " in text:
            warn(str(f.relative_to(root)),
                 "Model header missing tableName — add: static const constexpr char *tableName = \"<table>\"")


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: check_conventions.py <project_root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1]).resolve()
    if not root.exists():
        print(f"Project root not found: {root}", file=sys.stderr)
        return 2

    check_getenv_in_modules(root)
    check_no_db_in_controllers(root)
    check_service_has_interface(root)
    check_route_reg(root)
    check_csp_escape(root)
    check_model_table_name(root)

    errors   = [(f, m) for f, m, s in VIOLATIONS if s == 1]
    warnings = [(f, m) for f, m, s in VIOLATIONS if s == 0]

    if warnings:
        print(f"\n⚠  Warnings ({len(warnings)}):")
        for filepath, msg in warnings:
            print(f"  WARN  {filepath}: {msg}")

    if errors:
        print(f"\n✗  Violations ({len(errors)}) — CI FAILED:")
        for filepath, msg in errors:
            print(f"  FAIL  {filepath}: {msg}")
        return 1

    total = len(warnings)
    print(f"✓  check_conventions passed — {total} warning(s), 0 violations.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
