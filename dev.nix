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
            pkgs.clang_15
            pkgs.pkg-config
            # linked libs
            pkgs.catch2
            asio
            # other cli
            pkgs.ranger
        ];

        shellHook = ''
            export NIX_DEBUG=0;
            export NIX_CFLAGS_COMPILE="";
            export NIX_LDFLAGS="";
          '';
    }
