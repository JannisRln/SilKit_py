"""Sphinx documentation configuration for SilKit_py."""
from __future__ import annotations
from importlib.metadata import PackageNotFoundError, version as package_version

project = "SilKit_py"
author = "SilKit_py contributors"
try:
    release = package_version("SilKit_py")
except PackageNotFoundError:
    release = "1.0.5"
version = ".".join(release.split(".")[:2])
extensions = ["myst_parser"]
source_suffix = {".md": "markdown", ".rst": "restructuredtext"}
root_doc = "index"
exclude_patterns = ["_build", "index.rst", "python_example.rst"]
html_theme = "furo"
html_title = "SilKit_py Documentation"
myst_heading_anchors = 3
html_theme_options = {
    "source_repository": "https://github.com/JannisRln/SilKit_py/",
    "source_branch": "main",
    "source_directory": "docs/",
}
