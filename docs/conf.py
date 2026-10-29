"""Sphinx configuration for SilKit_py."""
from __future__ import annotations
from importlib.metadata import PackageNotFoundError, version as package_version
project = "SilKit_py"
author = "SilKit_py contributors"
try:
    release = package_version("SilKit_py")
except PackageNotFoundError:
    release = "1.0.5"
version = ".".join(release.split(".")[:2])
extensions = ["sphinx.ext.autodoc", "sphinx.ext.autosummary", "sphinx.ext.napoleon", "sphinx.ext.intersphinx"]
autosummary_generate = True
autodoc_typehints = "description"
intersphinx_mapping = {"python": ("https://docs.python.org/3", None)}
exclude_patterns = ["_build"]
html_theme = "alabaster"
