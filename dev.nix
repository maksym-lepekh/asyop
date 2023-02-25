let
    pkgs = import(fetchTarball "https://github.com/NixOS/nixpkgs/archive/988cc958c57ce4350ec248d2d53087777f9e1949.tar.gz") {};

    asio = (pkgs.asio.override { boost = null; });
in
    pkgs.mkShell {
        packages = [
            # build tools
            pkgs.cmake
            pkgs.ninja
            pkgs.ccache
            pkgs.gcc
            pkgs.clang_15
            pkgs.pkg-config
            # other cli
            pkgs.ranger
            pkgs.tree
            # linked libs
            pkgs.catch2
            asio
        ];

        shellHook = ''
            export NIX_DEBUG=0;
            export NIX_CFLAGS_COMPILE="";
            export NIX_LDFLAGS=" -L${pkgs.llvmPackages_15.libcxxabi}/lib";
          '';
    }
