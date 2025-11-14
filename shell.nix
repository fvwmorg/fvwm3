{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  nativeBuildInputs = with pkgs.buildPackages; [
    asciidoctor
    fontconfig
    freetype
    fribidi
    go
    libevent
    libpng
    librsvg
    libsm
    libx11
    libxcursor
    libxext
    libxfixes
    libxft
    libxi
    libxkbcommon
    libxpm
    libxrandr
    libxrender
    libxt
    ncurses
    pkg-config
    readline
    sharutils
    xtrans
  ];
}
