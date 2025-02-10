{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  # Define build inputs (dependencies)
  buildInputs = [
    pkgs.gcc                     # Compiler
    pkgs.raylib                  # Raylib library
    pkgs.xorg.libX11             # X11 library
    pkgs.mesa.drivers            # OpenGL library (includes libGL)
    pkgs.zlib                    # Compression library (used by raylib)
    pkgs.gdb                     # GNU Debugger
    pkgs.valgrind                # Memory checker
  ];

  # Set environment variables
  shellHook = ''
    export CC=gcc
    # Add debug flags (-g) and optimization level (-O0 for better debugging)
    export CFLAGS="-Wall -g -O0 -I${pkgs.raylib}/include"
    export LIBS="-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"

    # Provide a helpful message when entering the shell
    echo "Development environment ready with debugging tools:"
    echo "  - GDB (GNU Debugger)"
    echo "  - Valgrind (Memory checker)"
    echo ""
    echo "Build commands:"
    echo "  make          - Regular build"
    echo "  make debug    - Build with debug symbols"
    echo ""
    echo "Debug commands:"
    echo "  gdb ./bin/tree3d     - Start GDB"
    echo "  valgrind ./bin/tree3d - Check for memory issues"
  '';
}