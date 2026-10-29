Getting started
===============

Supported interpreters: CPython 3.12-3.14.

Install and check::

   python -m pip install SilKit_py
   python -c "import SilKit_py as silkit; print(silkit.__version__)"

Start a registry in a separate terminal::

   silkit-registry

Then follow the complete autonomous participant example in the README:
https://github.com/JannisRln/SilKit_py#quick-start

The example connects to silkit://localhost:8501. A live simulation
requires a registry and, in coordinated mode, a system controller.
