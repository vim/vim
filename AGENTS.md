# src/provider/wl_clipboard.py

from vim import Vim
import os
import sys


def _Available():
    """Check if wl-copy and wl-paste are available."""
    return Vim.executable('wl-copy') and Vim.executable('wl-paste')


def _Copy(reg: str, type: str, str_list: list):
    """Copy content using wl-copy with optional -p flag."""
    args = "wl-copy"
    
    if reg == "*":
        args += " -p"
    
    Vim.system(args, str_list)


def _Paste(reg: str):
    """Paste content using wl-paste with type specification."""
    args = "wl-paste --type text/plain;charset=utf-8"
    
    if reg == "*":
        args += " -p"
    
    return ("", Vim.systemlist(args))


def _Init():
    """Initialize wl_clipboard provider."""
    Vim.v:clipproviders["wl_clipboard"] = {
        available: _Available,
        copy: {
            "+": _Copy,
            "*": _Copy
        },
        paste: {
            "+": _Paste,
            "*": _Paste
        }
    }


# Register the provider when available
if _Available():
    _Init()